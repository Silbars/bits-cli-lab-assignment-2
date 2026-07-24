/*
 * process_manager.c
 *
 * Demonstrates safe management of child processes for a web-server-style
 * workload:
 *   1. Creates child processes with fork().
 *   2. Monitors their execution.
 *   3. Prevents zombie processes (reaps them via SIGCHLD handler / waitpid).
 *   4. Terminates unresponsive children using signals (SIGTERM, then SIGKILL).
 *
 * Compile: gcc -o process_manager process_manager.c
 * Run:     ./process_manager
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

#define NUM_CHILDREN 3
#define GRACE_SECONDS 3

static pid_t children[NUM_CHILDREN];

/* SIGCHLD handler: reap any child that has changed state.
 * WNOHANG makes waitpid non-blocking so we clear every finished child
 * without hanging, which is what prevents zombies from accumulating. */
static void reap_children(int signo)
{
    (void)signo;
    int saved_errno = errno;
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        /* async-signal-safe: use write(), not printf() */
        char msg[64];
        int n = snprintf(msg, sizeof(msg), "[parent] reaped child %d\n", pid);
        write(STDOUT_FILENO, msg, n);
    }
    errno = saved_errno;
}

/* Simulated worker. One child (index 0) "hangs" to model an
 * unresponsive process the parent must forcibly terminate. */
static void child_work(int idx)
{
    if (idx == 0) {
        /* Unresponsive child: ignore SIGTERM and loop forever. */
        signal(SIGTERM, SIG_IGN);
        while (1) pause();
    } else {
        sleep(1);              /* normal short-lived work */
        _exit(0);
    }
}

int main(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = reap_children;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;   /* restart syscalls; only real exits */
    sigaction(SIGCHLD, &sa, NULL);

    for (int i = 0; i < NUM_CHILDREN; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            child_work(i);      /* never returns */
        } else {
            children[i] = pid;
            printf("[parent] started child %d (pid %d)\n", i, pid);
        }
    }

    /* Monitor: give children a grace period to finish on their own. */
    sleep(GRACE_SECONDS);

    /* Any child still alive is treated as unresponsive.
     * kill(pid, 0) probes existence without sending a real signal. */
    for (int i = 0; i < NUM_CHILDREN; i++) {
        if (kill(children[i], 0) == 0) {
            printf("[parent] child %d (pid %d) unresponsive -> SIGTERM\n",
                   i, children[i]);
            kill(children[i], SIGTERM);
            sleep(1);
            if (kill(children[i], 0) == 0) {
                printf("[parent] child %d ignored SIGTERM -> SIGKILL\n", i);
                kill(children[i], SIGKILL);   /* cannot be caught or ignored */
            }
        }
    }

    /* Final blocking reap so nothing is left as a zombie. */
    int status;
    while (waitpid(-1, &status, 0) > 0) { /* reaped in handler too */ }

    printf("[parent] all children handled, exiting.\n");
    return 0;
}
