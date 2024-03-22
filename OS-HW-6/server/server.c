#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>

#define SHM_SIZE 1024
#define SHM_ID      1    // id of shared memory
#define PERMS		0666 // access permission mode

typedef struct SharedData {
	int number;
	int flag;
} shared_data;

int main() {
    // get shared memory aid
    int shmid = shmget(SHM_ID, SHM_SIZE, PERMS);
    if (shmid == -1) {
        perror("shmget failed");
        exit(1);
    }
	
    struct SharedData *sharedData = NULL;

    int prev = -1;
    // accept shared data from client
    while (1) {
        if ((sharedData = (struct SharedData *)shmat(shmid, NULL, 0)) == NULL) {
            perror("shmat failed");
            exit(1);
        }

        if (sharedData->flag) {
            if (prev != sharedData->number) {
                printf("Received number: %d\n", sharedData->number);
            }
            prev = sharedData->number;
        }
        sharedData->flag = 0;
        
        if (sharedData->number == -1) {
            printf("Input flow stopped. Ending program.");
            break;
        }
        sharedData->number = -2;
        sleep(1);
    }

    // delete shared memory segment
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl(IPC_RMID) failed");
        exit(1);
    }

    return 0;
}
