#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <time.h>
#include <dirent.h>
#include "../include/commands.h"
#include "../include/jobs.h"

static pid_t current_alarm_pgid = 0;

void timeout_handler(int sig) {
    if (current_alarm_pgid > 0) {
        kill(-current_alarm_pgid, SIGTERM);
        printf("\nresume: job timed out\n");
    }
}

void execute_activities() {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].is_active) {
            printf("[%d] pgid %d\n", jobs[i].job_id, jobs[i].pgid);
            printf("  %d %s %s\n", jobs[i].pgid, jobs[i].command, jobs[i].is_running ? "Running" : "Stopped");
        }
    }
}

void execute_ping(char **args, int arg_count) {
    if (arg_count != 2) {
        fprintf(stderr, "ping: invalid syntax\n");
        return;
    }
    
    int target = 0;
    int is_job = 0;
    if (args[0][0] == '%') {
        is_job = 1;
        target = atoi(args[0] + 1);
    } else {
        target = atoi(args[0]);
    }
    
    int sig = atoi(args[1]);
    if (sig < 0) {
        fprintf(stderr, "ping: invalid syntax\n");
        return;
    }

    pid_t target_pid = -1;
    if (is_job) {
        Job *j = find_job_by_id(target);
        if (!j) { fprintf(stderr, "ping: no such process found\n"); return; }
        target_pid = -j->pgid; // Send to process group
    } else {
        target_pid = target;
    }

    if (kill(target_pid, sig % 64) == -1) {
        fprintf(stderr, "ping: no such process found\n");
    } else {
        printf("Sent signal %d to %s\n", sig, args[0]);
    }
}

void execute_resume(char **args, int arg_count) {
    if (arg_count < 2 || args[0][0] != '%') {
        fprintf(stderr, "resume: invalid syntax\n");
        return;
    }
    
    int job_id = atoi(args[0] + 1);
    Job *j = find_job_by_id(job_id);
    if (!j) { fprintf(stderr, "resume: no such job\n"); return; }

    int is_fg = (strcmp(args[1], "fg") == 0);
    int is_bg = (strcmp(args[1], "bg") == 0);
    if (!is_fg && !is_bg) { fprintf(stderr, "resume: invalid syntax\n"); return; }

    int timeout = 0;
    if (is_fg && arg_count == 4 && strcmp(args[2], "--timeout") == 0) {
        timeout = atoi(args[3]);
    }

    kill(-j->pgid, SIGCONT);
    j->is_running = 1;

    if (is_fg) {
        printf("%s\n", j->command);
        tcsetpgrp(shell_terminal, j->pgid);
        
        if (timeout > 0) {
            current_alarm_pgid = j->pgid;
            signal(SIGALRM, timeout_handler);
            alarm(timeout);
        }

        int status;
        waitpid(-j->pgid, &status, WUNTRACED);
        
        if (timeout > 0) {
            alarm(0); // Cancel alarm if it finishes early
            current_alarm_pgid = 0;
        }

        if (WIFSTOPPED(status)) {
            j->is_running = 0;
            printf("\n[%d] + Stopped    %s\n", j->job_id, j->command);
        } else {
            j->is_active = 0; // It finished or was killed by the alarm
        }
        
        tcsetpgrp(shell_terminal, shell_pgid);
    } else {
        printf("[%d] + Running    %s\n", j->job_id, j->command);
    }
}

void execute_spy(char **args, int arg_count) {
    pid_t target = (arg_count > 0) ? atoi(args[0]) : getpid();
    char path[256];
    char target_path[1024];
    
    printf("PID    FD    TYPE   PATH\n");
    
    // CWD
    snprintf(path, sizeof(path), "/proc/%d/cwd", target);
    if (readlink(path, target_path, sizeof(target_path)) > 0) {
        printf("%d    cwd    DIR    %s\n", target, target_path);
    } else {
        fprintf(stderr, "spy: no such process\n");
        return;
    }

    // FDs
    snprintf(path, sizeof(path), "/proc/%d/fd", target);
    struct dirent *entry;
    DIR *dp = opendir(path);
    if (dp) {
        while ((entry = readdir(dp)) != NULL) {
            if (entry->d_name[0] == '.') continue;
            char full_fd_path[512];
            snprintf(full_fd_path, sizeof(full_fd_path), "%s/%s", path, entry->d_name);
            memset(target_path, 0, sizeof(target_path));
            if (readlink(full_fd_path, target_path, sizeof(target_path)) > 0) {
                printf("%d    %s      REG    %s\n", target, entry->d_name, target_path);
            }
        }
        closedir(dp);
    }
}

void execute_snoop(char **args, int arg_count) {
    if (arg_count == 0) return;
    
    pid_t pid = fork();
    if (pid == 0) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execvp(args[0], args);
        fprintf(stderr, "snoop: command not found\n");
        exit(1);
    } else {
        int status;
        waitpid(pid, &status, 0); // Wait for exec
        int syscall_count = 0;
        
        while (1) {
            ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
            waitpid(pid, &status, 0);
            if (WIFEXITED(status)) break;
            syscall_count++;
        }
        printf("Process exited. Syscall state transitions captured: %d\n", syscall_count/2);
    }
}