#include "common.h"

// SIGINT and SIGTERM signal handler
void sigfunc(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        printf("Reader: Caught signal, exiting...\n");
        // close semaphores and unlink all
        close_semaphores();
        unlink_sems_and_shmem();
        exit(EXIT_SUCCESS);
    }
}

int main() {
    signal(SIGINT, sigfunc);
    signal(SIGTERM, sigfunc);

    int counter = 0;

    init();

    // read 10 number from buffer
    while (counter < 10) {
        sem_wait(full);
        sem_wait(mutex);

        int data = buffer->store[buffer->have_reader];
        printf("Reader: Reading %d\n", data);

        sem_post(mutex);
        sem_post(empty);

        counter++;
    }

    printf("Reader: Received 10 numbers, exiting...\n");
    close_semaphores();
    unlink_sems_and_shmem();

    return 0;
}
