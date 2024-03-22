#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>


int sender_pid;
int number;
int bit;
int state; // 1 for getting, 0 for passive


void handle(int n_sig) {
    if (state == 0) {
        state = 1;
       
        if (n_sig == SIGUSR2) {
            printf("%d\n", number);
            exit(0);
        }

        kill(sender_pid, SIGUSR1);
    } else {
        state = 0;

        // set current bit to 1 (SIGUSR1) or 0 (SIGUSR2)
        if (n_sig == SIGUSR1) {
            number |= 1 << bit;
        }
        ++bit;

        kill(sender_pid, SIGUSR1);
    }
}


int main() {
    printf("this pid: %d\n", getpid());
    printf("input sender pid: ");
    scanf("%d", &sender_pid);

    (void)signal(SIGUSR1, handle);
    (void)signal(SIGUSR2, handle);

    bit = 0;
    number = 0;
    state = 0;

    while (1);
    return 0;
}
