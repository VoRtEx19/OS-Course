#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>


int state; // 1 for sending, 0 for waiting
int receiver_pid;
int number;
int bit;


void handler(int nsig) {
    if (bit < 8 * sizeof(number)) {
        if (state == 1) {
            state = 0;
            int mask = 1 << bit;
            ++bit;
            kill(receiver_pid, (number & mask) ? SIGUSR1 : SIGUSR2);
        } else {
            state = 1;
            kill(receiver_pid, SIGUSR1);
        }
    } else {
        // send SIGUSR2 - finish
        kill(receiver_pid, SIGUSR2);
        exit(0);
    }
}


int main() {
    printf("this pid: %d\n", getpid());
    printf("input reciever pid: ");
    scanf("%d", &receiver_pid);
    printf("input number: ");
    scanf("%d", &number);

    (void)signal(SIGUSR1, handler);

    bit = 0;
    state = 1;

    kill(receiver_pid, SIGUSR1);

    while (1);
    return 0;
}
