#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <time.h>

#define SHM_SIZE 1024
#define SHM_ID      1    // id of shared memory
#define PERMS       IPC_CREAT | 0666 // access permission mode

typedef struct SharedData {
	int number;
	int flag;
} shared_data;

int main() {
    // creating a shared memory segment
    int shmid = shmget(SHM_ID, SHM_SIZE, PERMS);
    if (shmid == -1) {
        perror("shmget error");
        exit(1);
    }

    // attaching shmem segment to processor's address space
    struct SharedData *shared_data = (struct SharedData *)shmat(shmid, NULL, 0);
    if (shared_data == NULL) {
        perror("shmat failed");
        exit(1);
    }

    // random number generator
    srand(time(NULL));
    int limit = 5;
    while (1) {
        if (shared_data->number != -2) {
            sleep(1);
            continue;
        }
        shared_data->number = rand() % 100;
        if (limit-- == 0) {
            shared_data->number = -1;
            printf("That's enough");
            break;
        }
        shared_data->flag = 1;
        printf("Generated number: %d\n", shared_data->number);
        sleep(1);
    }

    // detach from processor's address space
    if (shmdt(shared_data) == -1) {
        perror("shmdt failed");
        exit(1);
    }

    return 0;
}
