#include "common.h"

const char* shar_object = "/posix-shar-object";
int buf_id;
shared_memory *buffer;
const char *full_sem_name = "/full-semaphore";
const char *empty_sem_name = "/empty-semaphore";
const char *mutex_sem_name = "/mutex-semaphore";
const char *admin_sem_name = "/admin-semaphore";
sem_t *full;
sem_t *empty;
sem_t *mutex;
sem_t *admin;

void init(void) {
    if ((admin = sem_open(admin_sem_name, O_CREAT, 0666, 0)) == SEM_FAILED) {
        perror("sem_open: unable to create admin semaphore");
        exit(EXIT_FAILURE);
    }

    if ((mutex = sem_open(mutex_sem_name, O_CREAT, 0666, 1)) == SEM_FAILED) {
        perror("sem_open: unable to create mutex semaphore");
        exit(EXIT_FAILURE);
    }

    if ((empty = sem_open(empty_sem_name, O_CREAT, 0666, BUF_SIZE)) == SEM_FAILED) {
        perror("sem_open: unable to create free semaphore");
        exit(EXIT_FAILURE);
    }

    if ((full = sem_open(full_sem_name, O_CREAT, 0666, 0)) == SEM_FAILED) {
        perror("sem_open: unable to create full semaphore");
        exit(EXIT_FAILURE);
    }

    // created or open shared memory
    buf_id = shm_open(shar_object, O_CREAT | O_RDWR, 0666);
	
    if (buf_id == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
	
    ftruncate(buf_id, sizeof(shared_memory));
	
    buffer = mmap(NULL, sizeof(shared_memory), PROT_READ | PROT_WRITE, MAP_SHARED, buf_id, 0);
    
	if (buffer == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
}

void close_semaphores(void) {
    sem_close(empty);
    sem_close(full);
    sem_close(admin);
    sem_close(mutex);
}

void unlink_sems_and_shmem(void) {
    sem_unlink(mutex_sem_name);
    sem_unlink(empty_sem_name);
    sem_unlink(full_sem_name);
    sem_unlink(admin_sem_name);

    shm_unlink(shar_object);
}
