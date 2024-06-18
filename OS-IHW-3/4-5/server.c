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

#define MAXPENDING 5
#define BUFFER_SIZE 32

const char *shared_object = "/posix-shared-object";
const char *sem_shared_object = "/posix-sem-shared-object";
int pipe_fd[2];

int server_socket;
int children_counter;

struct Task {
	int id;
	int status;
	int index;
};

int createServerSocket(in_addr_t sin_addr, int port) {
    int server_socket;
    struct sockaddr_in server_address;

    if ((server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
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

    if ((client_socket =
             accept(server_socket, (struct sockaddr *)&client_address, &address_length)) < 0) {
        perror("Unable to accept client connection");
        exit(-1);
    }

    printf("Handling client %s:%d\n", inet_ntoa(client_address.sin_addr), client_address.sin_port);

    return client_socket;
}

void handleSemaphores(sem_t *semaphores, struct Task task) {
	if (task.id == 1 && task.status == 1) {
		sem_post(semaphores + 0);
	}
	if (task.id == 2 && task.status == 0) {
		sem_wait(semaphores + 0);
	}
	if (task.id == 2 && task.status == 1) {
		sem_post(semaphores + 1);
	}
	if (task.id == 3 && task.status == 0) {
		sem_wait(semaphores + 1);
	}
}

void handle(int client_socket, sem_t *semaphores, int number, int* storage) {
    char buffer[BUFFER_SIZE];
    int bytes_received;

    if (send(client_socket, (char *)(&number), sizeof(number), 0) != sizeof(number)) {
        perror("send() bad");
        exit(-1);
    }

    struct Task task;

    if ((bytes_received = recv(client_socket, buffer, sizeof(struct Task), 0)) < 0) {
        perror("recv() bad");
        exit(-1);
    }
    task = *((struct Task *)buffer);
    int result = 1;

    while (task.status != 2) {
        handleSemaphores(semaphores, task);
        result = storage[task.index];

        if (send(client_socket, &result, sizeof(int), 0) != sizeof(int)) {
            perror("send() bad");
            exit(-1);
        }

        if ((bytes_received = recv(client_socket, buffer, sizeof(struct Task), 0)) < 0) {
            perror("recv() bad");
            exit(-1);
        }
        task = *((struct Task *)buffer);
    }
    result = 1;

    if (send(client_socket, &result, sizeof(int), 0) != sizeof(int)) {
        perror("send() bad");
        exit(-1);
    }

    close(client_socket);
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
    
    for (int i = 0; i < sem_count; ++i) {
    	if (sem_init(semaphores + i, 1, 0) < 0) {
            perror("sem_init: can not create semaphore");
            exit(-1);
        };

        int val;
        sem_getvalue(semaphores + i, &val);
        if (val != 0) {
            printf("Ooops, one of semaphores can't set initial value to 0. Please, restart server.\n");
            shm_unlink(shared_object);
            exit(-1);
        }
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
    printf("Items in storage: ");
    for (int i = 0; i < number; ++i) {
    	storage[i] = (rand() % 10) * 100 + 1000;
    	printf("%d ", storage[i]);
    }
    printf("\n");

    return storage;
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
    printf("Server stopped\n");
    waitChildProcessess();
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

    //runWriter();

    signal(SIGINT, sigint_handler);

    int number = atoi(argv[3]);
    if (number > 10 || number < 2) {
        perror("Number of objects in storage should be in range [2, 10]");
        exit(-1);
    }
    
    int * storage = initializeStorage(number);

    sem_t *semaphores = createSemaphoresSharedMemory(2);

    server_socket = createServerSocket(server_address, server_port);
    while (1) {
        int client_socket = acceptClientConnection(server_socket);

        pid_t child_id;
        if ((child_id = fork()) < 0) {
            perror("Unable to create child for handling new connection");
            exit(-1);
        } else if (child_id == 0) {
            signal(SIGINT, SIG_DFL);
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
