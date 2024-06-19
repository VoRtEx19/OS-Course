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

#define MAXPENDING 5
#define BUFFER_SIZE 256

const char *shared_object = "/posix-shared-object";
const char *sem_shared_object = "/posix-sem-shared-object";
const char *observers_shared_object = "/posix-observers-shared-object";

int pipe_fd[2];

struct Observer *observers;
pthread_t writer_thread;
pthread_t registartor_thread;

int server_socket;
int observer_socket;
int personal_client_socket;
int children_counter = 0;

struct Task {
    int id;
    int status;
    int index;
};

enum event_type { ACTION, SERVER_INFO };

struct Event {
    char timestamp[26];
    char buffer[1024];
    enum event_type type;
};

struct Observer {
    int socket;
    int is_new;
    int is_active;
};

struct Args {
    int socket;
    sem_t *sem;
};

void writeEventToPipe(struct Event *event) {
    if (write(pipe_fd[1], event, sizeof(*event)) < 0) {
        perror("Can't write to pipe");
        exit(-1);
    }
}

void setEventWithCurrentTime(struct Event *event) {
    time_t timer;
    struct tm *tm_info;
    timer = time(NULL);
    tm_info = localtime(&timer);
    strftime(event->timestamp, sizeof(event->timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
}

int createServerSocket(in_addr_t sin_addr, int port) {
    int server_socket;
    struct sockaddr_in server_address;

    if ((server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("Unable to create server socket");
        exit(-1);
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = sin_addr;
    server_address.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Unable to bind address");
        exit(-1);
    }

    if (listen(server_socket, MAXPENDING) < 0) {
        perror("Unable to listen server socket");
        exit(-1);
    }

    return server_socket;
}

int acceptClientConnection(int server_socket) {
    int client_socket;
    struct sockaddr_in client_address;
    unsigned int address_length;

    address_length = sizeof(client_address);

    if ((client_socket = accept(server_socket, (struct sockaddr *)&client_address, &address_length)) < 0) {
        perror("Unable to accept client connection");
        exit(-1);
    }

    struct Event event;
    setEventWithCurrentTime(&event);
    event.type = SERVER_INFO;
    sprintf(event.buffer, "Handling client %s:%d\n", inet_ntoa(client_address.sin_addr), client_address.sin_port);
    writeEventToPipe(&event);

    return client_socket;
}

void publishLostConnectionMessage(int id) {
    struct Event finish_event;
    setEventWithCurrentTime(&finish_event);
    finish_event.type = SERVER_INFO;
    sprintf(finish_event.buffer, "Lost connection with praporschik %d\n", id);
    writeEventToPipe(&finish_event);
}

void introduceNewConnection(int id) {
    struct Event event;
    setEventWithCurrentTime(&event);
    event.type = SERVER_INFO;
    sprintf(event.buffer, "New connection from praporschik %d\n", id);
    writeEventToPipe(&event);
}

struct Observer *getObserversMemory() {
    struct Observer *observers;
    int shmid;

    if ((shmid = shm_open(observers_shared_object, O_CREAT | O_RDWR, 0666)) < 0) {
        perror("Can't connect to shared memory");
        exit(-1);
    } else {
        if (ftruncate(shmid, 100 * sizeof(struct Observer)) < 0) {
            perror("Can't resize shared memory");
            exit(-1);
        }
        if ((observers = mmap(0, 100 * sizeof(struct Observer), PROT_WRITE | PROT_READ, MAP_SHARED, shmid, 0)) < 0) {
            printf("Can't connect to shared memory\n");
            exit(-1);
        };
        printf("Open shared Memory with observers\n");
    }

    return observers;
}

void *registerObservers(void *args) {
    struct Args data = *((struct Args *)args);
    while (1) {
        int client_socket = acceptClientConnection(data.socket);

        struct Event finish_event;
        setEventWithCurrentTime(&finish_event);
        finish_event.type = SERVER_INFO;
        sprintf(finish_event.buffer, "Observer connected\n");
        writeEventToPipe(&finish_event);

        struct Observer observer;
        observer.is_new = 1;
        observer.socket = client_socket;
        observer.is_active = 1;

        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        sem_wait(data.sem);
        for (int i = 0; i < 100; ++i) {
            if (observers[i].is_active == 0) {
                observers[i] = observer;
                break;
            }
        }
        sem_post(data.sem);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    }
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
        if ((semaphores = mmap(0, sem_count * sizeof(sem_t), PROT_WRITE | PROT_READ, MAP_SHARED, sem_main_shmid, 0)) < 0) {
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

int * initializeStorage(int number) {
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
        if ((storage = mmap(0, number * sizeof(int), PROT_WRITE | PROT_READ, MAP_SHARED, shmid, 0)) < 0) {
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

void *writeInfoToConsole(void *args) {
    sem_t *sem = (sem_t *)args;
    while (1) {
        struct Event event;
        if (read(pipe_fd[0], &event, sizeof(event)) < 0) {
            perror("Can't read from pipe");
            exit(-1);
        }
        if (event.type == SERVER_INFO) {
            printf("%s | %s\n", event.timestamp, event.buffer);
        }
        char buffer[sizeof(event.timestamp) + sizeof(event.buffer) + 3];
        int size = sprintf(buffer, "%s | %s\n", event.timestamp, event.buffer);
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        sem_wait(sem);
        for (int i = 0; i < 100; ++i) {
            if (observers[i].is_active == 1) {
                if (send(observers[i].socket, buffer, size, MSG_NOSIGNAL) <= 0) {
                    observers[i].is_active = 0;
                    close(observers[i].socket);
                    printf("Observer disconnected\n");
                }
            }
        }
        sem_post(sem);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    }
}

void runWriter(sem_t *sem) {
    pthread_create(&writer_thread, NULL, writeInfoToConsole, (void *)sem);
}

void runObserverRegistrator(struct Args *args) {
    struct Observer observer;
    observer.is_active = 0;
    for (int i = 0; i < 100; ++i) {
        observers[i] = observer;
    }

    pthread_create(&registartor_thread, NULL, registerObservers, (void *)args);
}

void waitChildProcessess() {
    while (children_counter > 0) {
        int child_id = waitpid((pid_t)-1, NULL, 0);
        if (child_id < 0) {
            perror("Unable to wait child proccess");
            exit(-1);
        } else {
            children_counter--;
        }
    }
}

void sigint_handler(int signum) {
    waitChildProcessess();
    pthread_cancel(registartor_thread);
    pthread_cancel(writer_thread);
    struct Observer *observers_mem = getObserversMemory();
    for (int i = 0; i < 100; ++i) {
        int status = 0;
        if (observers_mem[i].is_active == 1) {
            send(observers_mem[i].socket, &status, sizeof(int), MSG_NOSIGNAL);
            close(observers_mem[i].socket);
        }
    }
    shm_unlink(shared_object);
    shm_unlink(sem_shared_object);
    shm_unlink(observers_shared_object);
    close(server_socket);
    close(observer_socket);
    printf("Server stopped\n");
    exit(0);
}

void child_sigint_handler(int signum) {
    close(personal_client_socket);
    exit(0);
}


void handleSemaphores(sem_t *semaphores, int *storage, struct Task task) {

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

void handle(int client_socket, sem_t *semaphores, int number, int * storage) {
    char buffer[BUFFER_SIZE];
    int bytes_received;

    if (send(client_socket, &number, sizeof(int), 0) != sizeof(int)) {
        perror("send() bad");
        exit(-1);
    }

    struct Task task;
    const int result = 1;

    if ((bytes_received = recv(client_socket, buffer, sizeof(struct Task), MSG_NOSIGNAL)) != sizeof(struct Task)) {
        publishLostConnectionMessage(task.id);
        close(client_socket);
        exit(0);
    }
    task = *((struct Task *)buffer);

    while (task.status != 2) {
        handleSemaphores(semaphores, storage, task);

        if (send(client_socket, &result, sizeof(int), MSG_NOSIGNAL) != sizeof(int)) {
            publishLostConnectionMessage(task.id);
            close(client_socket);
            exit(0);
        }

        if ((bytes_received = recv(client_socket, buffer, sizeof(struct Task), MSG_NOSIGNAL)) != sizeof(struct Task)) {
            publishLostConnectionMessage(task.id);
            close(client_socket);
            exit(0);
        }
        task = *((struct Task *)buffer);
    }

    struct Event finish_event;
    setEventWithCurrentTime(&finish_event);
    finish_event.type = ACTION;
    sprintf(finish_event.buffer, "Praporschik %d finished work\n", task.id);
    writeEventToPipe(&finish_event);

    if (send(client_socket, &result, sizeof(int), MSG_NOSIGNAL) != sizeof(int)) {
        publishLostConnectionMessage(task.id);
        close(client_socket);
        exit(0);
    }

    close(client_socket);
}

int main(int argc, char *argv[]) {

    if (argc != 5) {
        fprintf(stderr, "Usage:  %s <Server Address> <Server Port> <Observer Port> <Square side size>\n", argv[0]);
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

    int observer_port = atoi(argv[3]);
    if (observer_port < 0) {
        perror("Invalid observer port");
        exit(-1);
    }

    if (pipe(pipe_fd) < 0) {
        perror("Can't open pipe");
        exit(-1);
    }

    int number = atoi(argv[4]);
    if (number > 15 || number < 5) {
        perror("Number of objects in storage should be in range [5, 15]");
        exit(-1);
    }
    
    int * storage = initializeStorage(number);

    // +1 под семафор для observers
    int sem_count = 2 + 1;
    sem_t *semaphores = createSemaphoresSharedMemory(sem_count);
    observers = getObserversMemory();

    server_socket = createServerSocket(server_address, server_port);
    observer_socket = createServerSocket(server_address, observer_port);

    runWriter(semaphores + sem_count - 1);

    struct Args args;
    args.socket = observer_socket;
    args.sem = semaphores + sem_count - 1;
    runObserverRegistrator(&args);

    signal(SIGINT, sigint_handler);

    while (1) {
        int client_socket = acceptClientConnection(server_socket);

        pid_t child_id;
        if ((child_id = fork()) < 0) {
            perror("Unable to create child for handling new connection");
            exit(-1);
        } else if (child_id == 0) {
            personal_client_socket = client_socket;
            signal(SIGINT, child_sigint_handler);
            close(server_socket);
            handle(client_socket, semaphores, number, storage);
            exit(0);
        }

        printf("with child process: %d\n", (int)child_id);
        close(client_socket);
        children_counter++;
    }

    return 0;
}
