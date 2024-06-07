#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>

int main() {
	
	// initialising random
	srand(time(NULL));
	
	// init shmem
	char memn_ip[] = "shared-memory-ip";
	char memn_pn[] = "shared-memory-pn";
	int shm_ip, shm_pn;
	int shm_size = 10;
	int *addr_ip;
	int *addr_pn;
	
	if ((shm_ip = shm_open(memn_ip, O_CREAT | O_RDWR, 0666)) == -1 || (shm_pn = shm_open(memn_pn, O_CREAT | O_RDWR, 0666)) == -1) {
		perror("Error: cannot create shared memory segment");
		exit(1);
	}
	
	if (ftruncate(shm_ip, shm_size) == -1 || ftruncate(shm_pn, shm_size) == -1) {
		perror("Error: frtruncate");
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
		addr_ip = mmap(0, shm_size, PROT_WRITE, MAP_SHARED, shm_ip, 0);
		if (addr_ip == (int *) -1) {
			perror("Error: connecting to shmem adress");
			exit(1);
		}
		int i = 0;
		int *pointer;
		while (i < n) {
			sleep(rand() % 2 + 1);
			cost = 10 * (rand() % 10) + 100;
			printf("Ivanov:\tI've found smth #%d in the storage that costs %d\n", i+1, cost);
			pointer = &cost;
			memcpy(addr_ip, pointer, sizeof(pointer));
			sleep(rand() % 5 + 1);
			printf("Ivanov:\tI've just given smth #%d to Petrov\n", i+1);
			sem_post(ip_sem);
			++i;
		}
		close(shm_ip);
		exit(0);
	}
	
	if ((p_pid = fork()) == -1) {
		printf("Error: Couldn't start one of the child processes");
		exit(1);
	}
	
	if (p_pid == 0) {
		// do what p is supposed to do
		addr_ip = mmap(0, shm_size, PROT_READ, MAP_SHARED, shm_ip, 0);
		addr_pn = mmap(0, shm_size, PROT_WRITE, MAP_SHARED, shm_pn, 0);
		if (addr_ip == (int *) -1 || addr_pn == (int *) -1) {
			perror("Error: connecting to shmem adress");
			exit(1);
		}
		int i = 0;
		int *pointer;
		while (i < n) {
			sem_wait(ip_sem);
			cost = *addr_ip;
			sleep(rand() % 2 + 1);
			printf("Petrov:\tI've just got smth #%d from Ivanov that costs %d\n", i+1, cost);
			sleep(rand() % 3 + 1);
			printf("Petrov:\tI've just put smth #%d into the truck\n", i+1);
			pointer = &cost;
			memcpy(addr_pn, pointer, sizeof(pointer));
			sem_post(pn_sem);
			++i;
		}
		close(shm_ip);
		close(shm_pn);
		exit(0);
	}
	if ((n_pid = fork()) == -1) {
		printf("Error: Couldn't start one of the child processes");
		exit(1);
	}
	
	if (n_pid == 0) {
		// do what n is supposed to do
		addr_pn = mmap(0, shm_size, PROT_READ, MAP_SHARED, shm_pn, 0);
		if (addr_pn == (int *) -1) {
			perror("Error: connecting to shmem adress");
			exit(1);
		}
		int i = 0;
		int sum;
		while (i < n) {
			sem_wait(pn_sem);
			cost = *addr_pn;
			sum += cost;
			printf("Necheporuk:\tI've just seen Petrov put smth #%d into the truck\n", i+1);
			sleep(rand() % 2 + 1);
			printf("Necheporuk:\tI've just counted the value of the object #%d: %d\n", i+1, cost);
			++i;
		}
		printf("Necheporuk:\tTotal sum: %d", sum);
		close(shm_pn);
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
    
    close(shm_ip);
    close(shm_pn);
    if (shm_unlink(memn_ip) == -1 || shm_unlink(memn_pn) == -1) {
    	perror("Error: unlinking shmem");
    	exit(1);
    }
    
    exit(0);
}
