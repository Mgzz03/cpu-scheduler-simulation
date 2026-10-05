#include "headers.h"
#include <signal.h>

volatile sig_atomic_t paused = 0;

void handle_sigstop(int signum) {
    paused = 1;
    // SIGSTOP cannot be caught, but this is for debugging if needed
}

void handle_sigcont(int signum) {
    paused = 0;
    printf("Process resumed at time %d\n", getClk());
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <RunTime> <ArrTime>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    signal(SIGCONT, handle_sigcont);
    // Note: SIGSTOP cannot be caught, but we set a handler for debugging
    signal(SIGSTOP, handle_sigstop);

    initClk();
    int remainingtime = atoi(argv[1]);
    int arrtime = atoi(argv[2]); // Unused but included for compatibility
    int currentC = getClk();

    while (remainingtime > 0) {
        if (currentC < getClk() && !paused) {
            printf("Remaining Time %d\n", remainingtime);
            remainingtime--;
            currentC = getClk();
        }
    }

    printf("Process Terminated at time %d\n", getClk());
    kill(getppid(), SIGINT);
    destroyClk(false);
    exit(0);
}
