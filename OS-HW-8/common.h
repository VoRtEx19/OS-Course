#include <time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <signal.h>

#define BUF_SIZE 10     // buffer size

// struct for storing buffer and other data in shmem
typedef struct {
    int store[BUF_SIZE];  // buffer
    int have_reader;      // bool (true if there is a reader)
    int reader_pids[2];   // reader ids
    int writer_pid;       // writer id
} shared_memory;

// Имена объектов
extern const char* shar_object;
extern const char *full_sem_name;
extern const char *empty_sem_name;
extern const char *mutex_sem_name;
extern const char *admin_sem_name;

extern int buf_id;
extern shared_memory *buffer;
extern sem_t *full;
extern sem_t *empty;
extern sem_t *mutex;
extern sem_t *admin;

void init(void);
void close_semaphores(void);
void unlink_sems_and_shmem(void);
