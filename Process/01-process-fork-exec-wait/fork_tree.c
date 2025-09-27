
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

void D(pid_t parent) {
    printf("D: pid=%d, ppid=%d\n", getpid(), parent);

    pid_t D_pid = getpid();
    pid_t F_pid = fork();
    pid_t E_pid = 0;
    
    if (F_pid == 0) {
        printf("F: pid=%d, ppid=%d\n", getpid(), D_pid);
    }
    else if (F_pid > 0) {
        E_pid = fork();
        if (E_pid == 0) {
            printf("E: pid=%d, ppid=%d\n", getpid(), D_pid);
        }
        else if (E_pid > 0) {
            // in process D
        }
    }
}

void B(pid_t parent) {
    printf("B: pid=%d, ppid=%d\n", getpid(), parent);
    //pause(); // wait for signal
    pid_t B_pid = getpid();
    pid_t D_pid = fork();

    if (D_pid == 0) {
        D(B_pid);
    }
    else if (D_pid > 0) {
        // in process B
    }
}

void C(pid_t parent) {
    printf("C: pid=%d, ppid=%d\n", getpid(), parent);
    //pause(); // wait for signal
}

int main(void) {
    pid_t A_pid = getpid();
    pid_t B_pid = fork();
    pid_t C_pid = 0;

    if (B_pid == 0) { 
        B(A_pid); // in B
    }
    else if (B_pid > 0) {
        C_pid = fork();

        if (C_pid == 0) {
            C(A_pid); // in C
        }
        else if (C_pid > 0) {
            printf("I am process A. My PID is %d and I don't have a parent\n",A_pid);
        }
        else {
            // error 
        }
    }

    sleep(1);                    // pretend to do work
    //kill(B_pid, SIGTERM);        // terminate children
    //kill(C_pid, SIGTERM);

    return 0;
}