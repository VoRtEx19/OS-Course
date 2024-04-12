#include "common.h"

void sigfunc(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        printf("Writer: Caught signal, exiting...\n");
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
    while (counter < 10) {
        sem_wait(empty);
        sem_wait(mutex);
        buffer->store[buffer->have_reader] = rand() % 1000;
        printf("Writer: Writing %d\n", buffer->store[buffer->have_reader]);
        buffer->have_reader = (buffer->have_reader + 1) % BUF_SIZE;

        sem_post(mutex);
        sem_post(full);

        counter++;
    }

    printf("Writer: Transferred 10 numbers, exiting...\n");
    close_semaphores();
    unlink_sems_and_shmem();

    return 0;
}
