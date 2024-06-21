#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <time.h>
#include <pthread.h>
#include <poll.h>

#define BUFFER_SIZE 256
#define EMPTY_PLOT_COEFFICIENT 2

const char *shared_object = "/posix-shared-object";
const char *sem_shared_object = "/posix-sem-shared-object";
int pipe_fd[2];
int iv_pipe_fd[2];
int pe_pipe_fd[2];
int ne_pipe_fd[2];
int results_pipe_fd[2];

int server_socket;
int children_counter = 0;

pthread_t praporschiks_threads[3];
pthread_t task_reader_thread;
pthread_t result_sender_thread;

struct Task {
    int id;
    int index;
    int status;
};

struct Request {
    struct Task task;
    struct sockaddr_in client_address;
};

struct TaskResult {
    char buffer[1024];
    int size;
    struct sockaddr_in client_address;
};

enum event_type { MAP, ACTION, META_INFO, SERVER_INFO };

struct Event {
    char timestamp[26];
    char buffer[1024];
    enum event_type type;
};

struct Args {
    int socket;
    sem_t *sem;
};

struct HandleArgs {
    int server_socket;
    sem_t *semaphores;
    int *storage;
    int number;
    int id;
    int *pipe_fd;
};

struct Response {
    int status;
    struct sockaddr_in client_address;
};

int createServerSocket(in_addr_t sin_addr, int port) {
    int server_socket;
    struct sockaddr_in server_address;

    if ((server_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("Unable to create server socket");
        exit(-1);
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Unable to bind address");
        exit(-1);
    }

    return server_socket;
}

void printStorage(int *storage, int number) {
	printf("Storage:");
    for (int i = 0; i < number; ++i) {
    	printf(" %d", storage[i]);
    }
    printf("\n");

    fflush(stdout);
}

void setEventWithCurrentTime(struct Event *event) {
    time_t timer;
    struct tm *tm_info;
    timer = time(NULL);
    tm_info = localtime(&timer);
    strftime(event->timestamp, sizeof(event->timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
}

void writeEventToPipe(struct Event *event) {
    if (write(pipe_fd[1], event, sizeof(*event)) < 0) {
        perror("Can't write to pipe");
        exit(-1);
    }
}

void handle(sem_t *semaphores, int *storage, int number, struct Task task) {
    struct Event event;
    setEventWithCurrentTime(&event);
    event.type = ACTION;
    if (task.id == 1) {
    	if (task.status == 0) {
    		sprintf(event.buffer, "Ivanov took item #%d from the storage\n", task.index + 1);
    	}
    	if (task.status == 1) {
    		sprintf(event.buffer, "Ivanov gave item #%d over to Petrov\n", task.index + 1);
    		sem_post(semaphores + 0);
		}
    }
    if (task.id == 2) {
    	if (task.status == 0) {
    		sem_wait(semaphores + 0);
    		sprintf(event.buffer, "Petrov took item #%d from Ivanov\n", task.index + 1);
    	}
    	if (task.status == 1) {
    		sprintf(event.buffer, "Petrov put item #%d into the truck\n", task.index + 1);
    		sem_post(semaphores + 1);
		}
    }
    if (task.id == 3) {
    	if (task.status == 0) {
    		sem_wait(semaphores + 1);
    		sprintf(event.buffer, "Necheporuk saw Petrov put item #%d into the truck\n", task.index + 1);
    	}
    	if (task.status == 1) {
    		sprintf(event.buffer, "Necheporuk estimated item #%d to be worth %d\n", task.index + 1, storage[task.index]);
		}
    }
    writeEventToPipe(&event);

}

int writeToBuffer(void *object, char *buffer, int size) {
    int i = 0;
    for (; i < size; ++i) {
        buffer[i] = *((char *)(object) + i);
    }

    return i;
}

void *praporschikHandler(void *args) {
    struct HandleArgs params = *((struct HandleArgs *)args);
    while (1) {
        struct Request request;

        if (read(params.pipe_fd[0], &request, sizeof(request)) < 0) {
            perror("Can't read from pipe");
        }

        int status = 1;
        struct TaskResult result;
        if (request.task.status == 2) {
            result.size = writeToBuffer(&params.number, result.buffer, sizeof(params.number));
            result.client_address = request.client_address;
            write(results_pipe_fd[1], &result, sizeof(result));
        } else if (request.task.status < 2) {
            handle(params.semaphores, params.storage, params.number, request.task);
            result.size = writeToBuffer(&status, result.buffer, sizeof(status));
            result.client_address = request.client_address;
            write(results_pipe_fd[1], &result, sizeof(result));
        } else if (request.task.status == 3) {
            struct Event finish_event;
            setEventWithCurrentTime(&finish_event);
            finish_event.type = ACTION;
            sprintf(finish_event.buffer, "Praporschik %d finish work\n", request.task.id);
            writeEventToPipe(&finish_event);
            result.size = writeToBuffer(&status, result.buffer, sizeof(status));
            result.client_address = request.client_address;
            write(results_pipe_fd[1], &result, sizeof(result));
        }
    }
}

void runPraporschik(struct HandleArgs *args) {
    pthread_create(praporschiks_threads + args->id - 1, NULL, praporschikHandler, args);
}

int *getStorage(int number) {
    int *storage;
    int shmid;

    if ((shmid = shm_open(shared_object, O_CREAT | O_RDWR, 0666)) < 0) {
        perror("Can't connect to shared memory");
        exit(-1);
    } else {
        if (ftruncate(shmid, number * sizeof(int)) < 0) {
            perror("Can't resize shared memory");
            exit(-1);
        }
        if ((storage = mmap(0, number * sizeof(int), PROT_WRITE | PROT_READ, MAP_SHARED, shmid,
                          0)) < 0) {
            printf("Can't connect to shared memory\n");
            exit(-1);
        };
        printf("Open shared Memory\n");
    }
    
    srand(time(NULL));
    for (int i = 0; i < number; ++i) {
    	storage[i] = (rand() % 10) * 100 + 1000;
    }

    return storage;
}

sem_t *createSemaphoresSharedMemory(int sem_count) {
    int sem_main_shmid;
    sem_t *semaphores;

    if ((sem_main_shmid = shm_open(sem_shared_object, O_CREAT | O_RDWR, 0666)) < 0) {
        perror("Can't connect to shared memory");
        exit(-1);
    } else {
        if (ftruncate(sem_main_shmid, sem_count * sizeof(sem_t)) < 0) {
            perror("Can't rezie shm");
            exit(-1);
        }
        if ((semaphores = mmap(0, sem_count * sizeof(sem_t), PROT_WRITE | PROT_READ, MAP_SHARED,
                               sem_main_shmid, 0)) < 0) {
            printf("Can\'t connect to shared memory for semaphores\n");
            exit(-1);
        };
        printf("Open shared Memory for semaphores\n");
    }
    
	int val;
    for (int i = 0; i < sem_count; ++i) {
    	if (sem_init(semaphores + i, 1, 0) < 0) {
            perror("sem_init: can not create semaphore");
            exit(-1);
        };
        sem_getvalue(semaphores + i, &val);
        if (val != 0) {
            printf("Ooops, one of semaphores can't set initial value to 0. Please, restart server.\n");
            shm_unlink(shared_object);
            exit(-1);
        }
    }
    
    if (sem_init(semaphores + sem_count - 1, 1, 1) < 0) {
        perror("sem_init: can not create semaphore");
        exit(-1);
    };
	sem_getvalue(semaphores + sem_count - 1, &val);
	if (val != 1) {
		printf("Ooops, one of semaphores can't set initial value to 1. Please, restart server.\n");
		shm_unlink(shared_object);
		exit(-1);
	}

    return semaphores;
}

void writeInfoToConsole() {
    while (1) {
        struct Event event;
        if (read(pipe_fd[0], &event, sizeof(event)) < 0) {
            perror("Can't read from pipe");
            exit(-1);
        }
        if (event.type == MAP) {
            printf("%s | %s\n", event.timestamp, event.buffer);
        }
    }
}

pid_t runWriter() {
    pid_t child_id;
    if ((child_id = fork()) < 0) {
        perror("Unable to create child for handling write to log");
        exit(-1);
    } else if (child_id == 0) {
        writeInfoToConsole();
        exit(0);
    }

    return child_id;
}

void *readTasks(void *args) {
    struct Args params = *((struct Args *)args);

    while (1) {
        struct pollfd fd;
        fd.fd = params.socket;
        fd.events = POLLIN;

        int result = poll(&fd, 1, -1);
        if (result < 0) {
            perror("Can't poll socket");
            exit(-1);
        }

        struct Task task;
        struct sockaddr_in client_address;
        socklen_t client_length = sizeof(client_address);

        sem_wait(params.sem);
        int received_size = 0;
        if ((received_size = recvfrom(params.socket, &task, sizeof(task), 0, (struct sockaddr *)&client_address, &client_length)) != sizeof(task)) {
            perror("Can't receive task");
            sem_post(params.sem);
            continue;
        }
        sem_post(params.sem);

        struct Request request;
        request.task = task;
        request.client_address = client_address;

        int pipe_write_fd = task.id == 1 ? iv_pipe_fd[1] : (task.id == 2 ? pe_pipe_fd[1] : ne_pipe_fd[1]);

        if (write(pipe_write_fd, &request, sizeof(request)) < 0) {
            perror("Can't write to pipe");
        }
    }
}

void *resultSender(void *args) {
    int server_socket = *((int *)args);
    while (1) {
        struct TaskResult result;
        read(results_pipe_fd[0], &result, sizeof(result));
        sendto(server_socket, result.buffer, result.size, 0, (struct sockaddr *)&result.client_address, sizeof(result.client_address));
    }
}

void runTaskReader(struct Args *args) {
    if (pthread_create(&task_reader_thread, NULL, readTasks, (void *)args) < 0) {
        perror("Can't run task reader");
        exit(-1);
    }
}

void runResultSender(int *server_socket) {
    if (pthread_create(&result_sender_thread, NULL, resultSender, (void *)server_socket) < 0) {
        perror("Can't run result sender");
        exit(-1);
    }
}

void sigint_handler(int signum) {
    printf("Server stopped\n");
    shm_unlink(shared_object);
    shm_unlink(sem_shared_object);
    close(server_socket);
    exit(0);
}

int main(int argc, char *argv[]) {

    if (argc != 4) {
        fprintf(stderr, "Usage:  %s <Server Address> <Server Port> <Number of objects>\n", argv[0]);
        exit(1);
    }

    in_addr_t server_address;
    if ((server_address = inet_addr(argv[1])) < 0) {
        perror("Invalid server address");
        exit(-1);
    }

    int server_port = atoi(argv[2]);
    if (server_port < 0) {
        perror("Invalid server port");
        exit(-1);
    }

    if (pipe(pipe_fd) < 0) {
        perror("Can't open pipe");
        exit(-1);
    }

    if (pipe(iv_pipe_fd) < 0) {
        perror("Can't open task pipe");
        exit(-1);
    }

    if (pipe(pe_pipe_fd) < 0) {
        perror("Can't open task pipe");
        exit(-1);
    }
    
    if (pipe(ne_pipe_fd) < 0) {
        perror("Can't open task pipe");
        exit(-1);
    }

    if (pipe(results_pipe_fd) < 0) {
        perror("Can't open task pipe");
        exit(-1);
    }

    runWriter();

    signal(SIGINT, sigint_handler);

    int number = atoi(argv[3]);
    if (number > 15 || number < 5) {
        perror("Number of items should be in range [5, 15]");
        exit(-1);
    }

    int sem_count = 2 + 1;

    int *storage = getStorage(number);

    sem_t *semaphores = createSemaphoresSharedMemory(sem_count);

    server_socket = createServerSocket(server_address, server_port);
    printStorage(storage, number);

    struct Args args;
    args.socket = server_socket;
    args.sem = semaphores + sem_count - 1;

    struct HandleArgs ivanovArgs;
    ivanovArgs.server_socket = server_socket;
    ivanovArgs.storage = NULL;
    ivanovArgs.semaphores = semaphores;
    ivanovArgs.number = number;
    ivanovArgs.id = 1;
    ivanovArgs.pipe_fd = iv_pipe_fd;

    struct HandleArgs petrovArgs;
    petrovArgs.server_socket = server_socket;
    petrovArgs.storage = NULL;
    petrovArgs.semaphores = semaphores;
    petrovArgs.number = number;
    petrovArgs.id = 2;
    petrovArgs.pipe_fd = pe_pipe_fd;
    
    struct HandleArgs necheporukArgs;
    necheporukArgs.server_socket = server_socket;
    necheporukArgs.storage = storage;
    necheporukArgs.semaphores = semaphores;
    necheporukArgs.number = number;
    necheporukArgs.id = 3;
    necheporukArgs.pipe_fd = ne_pipe_fd;

    runTaskReader(&args);
    runResultSender(&server_socket);
    runPraporschik(&ivanovArgs);
    runPraporschik(&petrovArgs);
    runPraporschik(&necheporukArgs);

    pthread_join(task_reader_thread, NULL);
    pthread_join(result_sender_thread, NULL);

    return 0;
}
