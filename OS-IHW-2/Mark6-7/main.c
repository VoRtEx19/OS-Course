#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>

#define SHMID_IP 0x1111
#define SHMID_PN 0x2222
#define PERMS 0666

int main() {
	
	// initialising random
	srand(time(NULL));
	
	// init shmem
	int shmid_ip, shmid_pn;
	int* msg_ip;
	int* msg_pn;
	
	if ((shmid_ip = shmget(SHMID_IP, sizeof(int), PERMS | IPC_CREAT)) < 0 || (shmid_pn = shmget(SHMID_PN, sizeof(int), PERMS | IPC_CREAT)) < 0) {
		perror("Error: could not create shared memory");
		exit(1);
	}
	
	// initialising named semaphores
	const char* ip_sem_name = "/ivanov-petrov-semaphore";
	const char* pn_sem_name = "/petrov-necheporuk-semaphore";
	sem_t* ip_sem;
	sem_t* pn_sem;
	
	if ((ip_sem = sem_open(ip_sem_name, O_CREAT, 0666, 0)) == 0 || (pn_sem = sem_open(pn_sem_name, O_CREAT, 0666, 0)) == 0) {
		perror("Error: Couldn't open a semaphore");
		exit(1);
	}
	
	// child processes for praporschiks:
	// i for ivanov, p for petrov, n for necheporuk, v for vendetta
	pid_t i_pid, p_pid, n_pid;
	
	int n = 5;
	int cost;
	
	if ((i_pid = fork()) == -1) {
		printf("Error: Couldn't start one of the child processes");
		exit(1);
	}
	
	if (i_pid == 0) {
		// do what i is supposed to
		if ((msg_ip = (int*) shmat (shmid_ip, 0, 0)) == NULL) {
			perror("Error: couldn't attach to shmem");
			exit(1);
		}
		int i = 0;
		int *pointer;
		while (i < n) {
			sleep(rand() % 2 + 1);
			cost = 10 * (rand() % 10) + 100;
			printf("Ivanov:\tI've found smth #%d in the storage that costs %d\n", i+1, cost);
			pointer = &cost;
			memcpy(msg_ip, pointer, sizeof(pointer));
			sleep(rand() % 5 + 1);
			printf("Ivanov:\tI've just given smth #%d to Petrov\n", i+1);
			sem_post(ip_sem);
			++i;
		}
		shmdt(msg_p);
		exit(0);
	}
	
	if ((p_pid = fork()) == -1) {
		printf("Error: Couldn't start one of the child processes");
		exit(1);
	}
	
	if (p_pid == 0) {
		// do what p is supposed to do
		if ((msg_ip = (int*) shmat (shmid_ip, 0, 0)) == NULL || (msg_pn == (int *) shmat (shmid_pn, 0, 0)) == NULL) {
			perror("Error: couldn't attach to shmem");
			exit(1);
		}
		int i = 0;
		int *pointer;
		while (i < n) {
			sem_wait(ip_sem);
			cost = *msg_ip;
			sleep(rand() % 2 + 1);
			printf("Petrov:\tI've just got smth #%d from Ivanov that costs %d\n", i+1, cost);
			sleep(rand() % 3 + 1);
			printf("Petrov:\tI've just put smth #%d into the truck\n", i+1);
			pointer = &cost;
			memcpy(msg_pn, pointer, sizeof(pointer));
			sem_post(pn_sem);
			++i;
		}
		shmdt(msg_ip);
		shmdt(msg_pn);
		exit(0);
	}
	if ((n_pid = fork()) == -1) {
		printf("Error: Couldn't start one of the child processes");
		exit(1);
	}
	
	if (n_pid == 0) {
		// do what n is supposed to do
		if ((msg_pn = (int*) shmat (shmid_pn, 0, 0)) == NULL) {
			perror("Error: couldn't attach to shmem");
			exit(1);
		}
		int i = 0;
		int sum;
		while (i < n) {
			sem_wait(pn_sem);
			cost = *msg_pn;
			sum += cost;
			printf("Necheporuk:\tI've just seen Petrov put smth #%d into the truck\n", i+1);
			sleep(rand() % 2 + 1);
			printf("Necheporuk:\tI've just counted the value of the object #%d: %d\n", i+1, cost);
			++i;
		}
		printf("Necheporuk:\tTotal sum: %d", sum);
		shmdt(msg_pn);
		exit(0);
	}
	
	// Wait for all child processes to finish
    waitpid(i_pid, NULL, 0);
    waitpid(p_pid, NULL, 0);
    waitpid(n_pid, NULL, 0);
    
    // close and unlink semaphores
    if (sem_close(ip_sem) == -1 || sem_close(pn_sem) == -1) {
    	printf("Error: Cannot close sempahores");
    	exit(1);
    }
    if (sem_unlink(ip_sem_name) == -1 || sem_unlink(pn_sem_name) == -1) {
    	printf("Error: Cannot unlink semaphores");
    	exit(1);
    }
    
    //finishing up shmem
    shmdt(msg_ip);
    shmdt(msg_pn);
    if (shmctl(shmid_ip, IPC_RMID, (struct shmid_ds *) 0) < 0 || shmctl(shmid_pn, IPC_RMID, (struct shmid_ds *) 0) < 0){
    	perror("Error: could not turn off shmem");
    	exit(1);
    }
    
    
    exit(0);
}
