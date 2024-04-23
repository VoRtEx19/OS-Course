#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <dirent.h>

int main(int argc, char *argv[]) {
	pid_t chpid;
	int status;
	/* setting up starting values */
	uint64_t curr = 1;
	/* argument received from command prompt */
	uint64_t n = 0;
    	if (argc == 1) {
        	printf("[using n = %d (default)]\n", n);
    	} else {
        	n = atoi(argv[1]);
        	printf("[using n = %d]\n", n);
    	}
	if ((chpid = fork()) == -1) {
		printf(" I can't have a baby (fork error) ");
		return 0;
	} else if (chpid == 0) {
		/* counting factorial */
		for (uint64_t i = 2; i <= n; ++i) {
			curr = curr * i;
			if (curr > UINT64_MAX / n) {
				printf("Factorial process (child): PID=%d, PPID=%d, Child=%d\nOverflow occured, last normal result = %d on iteration %d", (int) getpid(), (int) getppid(), (int) chpid, curr, i);
				exit(0);
			}
		}
		printf("Factorial process (child): PID=%d, PPID=%d, Child=%d\nFactorial: %d\n", (int) getpid(), (int) getppid(), (int) chpid, curr);
	} else {
		printf("Fibonacci process (parent): PID=%d, PPID=%d, Child=%d\n", getpid(), getppid(), chpid);
		/* counting fibonacci */
		uint64_t prev = 1;
		for (uint64_t i = 1; i <= n; ++i) {
			curr = prev + curr;
			prev = curr - prev;
			if (curr > UINT64_MAX - prev) {
				printf("Fibonacci process (child): PID=%d, PPID=%d, Child=%d\nOverflow occured, last normal result = %d on iteration %d", (int) getpid(), (int) getppid(), (int) chpid, curr, i);
				exit(0);
			}
		}
		printf("Fibonacci process (child): PID=%d, PPID=%d, Child=%d\nFibonacci: %d\n", (int) getpid(), (int) getppid(), (int) chpid, curr);
		waitpid(chpid, &status, 0);
        	/* Print directory contents (Using here to avoid repetition) */
        	DIR* dir;
    		struct dirent *ent;
    		char current_dir[256];
    		getcwd(current_dir, sizeof(current_dir));
    		
    		if ((dir = opendir(current_dir)) != NULL) {
        		printf("\nDirectory contents:\n");
        		while ((ent = readdir(dir)) != NULL) {
            			printf("%s\n", ent->d_name);
        		}
        		closedir(dir);
    		} else {
        		perror("Error opening directory");
    		}
	}
}
