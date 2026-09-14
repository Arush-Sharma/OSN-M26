#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include "../include/jobs.h"

Job jobs[MAX_JOBS];
int next_job_id = 1;
pid_t shell_pgid;
struct termios shell_tmodes;
int shell_terminal;

void init_shell() {
    shell_terminal = STDIN_FILENO;
    // Loop until we are in the foreground
    while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp())) {
        kill(-shell_pgid, SIGTTIN);
    }
    
    // Ignore interactive and job-control signals
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGCHLD, sigchld_handler);

    // Put ourselves in our own process group
    shell_pgid = getpid();
    if (setpgid(shell_pgid, shell_pgid) < 0) {
        perror("Couldn't put the shell in its own process group");
        exit(1);
    }

    // Grab control of the terminal
    tcsetpgrp(shell_terminal, shell_pgid);
    tcgetattr(shell_terminal, &shell_tmodes);

    for (int i = 0; i < MAX_JOBS; i++) {
        jobs[i].is_active = 0;
    }
}

void add_job(pid_t pgid, const char *cmd, int is_running) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!jobs[i].is_active) {
            jobs[i].job_id = next_job_id++;
            jobs[i].pgid = pgid;
            strncpy(jobs[i].command, cmd, 1023);
            jobs[i].is_running = is_running;
            jobs[i].is_active = 1;
            printf("[%d] %d\n", jobs[i].job_id, pgid);
            return;
        }
    }
}

void update_jobs_status() {
    int status;
    pid_t pid;
    // WNOHANG ensures we don't block. WUNTRACED checks for stopped jobs.
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        for (int i = 0; i < MAX_JOBS; i++) {
            if (jobs[i].is_active && jobs[i].pgid == pid) { // Simplified: matching PGID to PID for first process
                if (WIFEXITED(status) || WIFSIGNALED(status)) {
                    printf("\n%s with pid %d exited %s\n", 
                        jobs[i].command, pid, WIFEXITED(status) ? "normally" : "abnormally");
                    jobs[i].is_active = 0;
                } else if (WIFSTOPPED(status)) {
                    jobs[i].is_running = 0;
                }
                break;
            }
        }
    }
}

void sigchld_handler(int sig) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        for (int i = 0; i < MAX_JOBS; i++) {
            if (jobs[i].is_active && jobs[i].pgid == pid) { 
                if (WIFEXITED(status) || WIFSIGNALED(status)) {
                    printf("\n%s with pid %d exited %s\n", 
                        jobs[i].command, pid, WIFEXITED(status) ? "normally" : "abnormally");
                    jobs[i].is_active = 0;
                } else if (WIFSTOPPED(status)) {
                    jobs[i].is_running = 0;
                }
                break;
            }
        }
    }
}

Job* find_job_by_id(int job_id) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].is_active && jobs[i].job_id == job_id) return &jobs[i];
    }
    return NULL;
}