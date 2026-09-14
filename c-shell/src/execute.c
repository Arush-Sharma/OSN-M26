#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "../include/lexer.h"
#include "../include/builtin.h"
#include "../include/jobs.h"

// The path resolver from earlier
char* resolve_path(char *command) {
    if (strchr(command, '/')) return command;
    char *path_env = getenv("PATH");
    if (!path_env) return NULL;
    char *path_copy = strdup(path_env);
    char *dir = strtok(path_copy, ":");
    static char full_path[1024];

    while (dir != NULL) {
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, command);
        if (access(full_path, X_OK) == 0) {
            free(path_copy);
            return full_path;
        }
        dir = strtok(NULL, ":");
    }
    free(path_copy);
    return NULL;
}

// Handles redirection and execution for a SINGLE command in the pipeline
// NOTE: This runs entirely inside a child process. If it succeeds, it never returns.
void execute_single_command(Token *tokens, int start_idx, int end_idx) {
    char *args[128];
    int arg_count = 0;
    
    char *input_file = NULL;
    char *output_file = NULL;
    int append_output = 0;

    // 1. Parse just this slice of tokens for <, >, >>
    for (int i = start_idx; i < end_idx; i++) {
        if (strcmp(tokens[i].value, "<") == 0 && i + 1 < end_idx) {
            input_file = tokens[i + 1].value;
            i++; 
        } 
        else if (strcmp(tokens[i].value, ">") == 0 && i + 1 < end_idx) {
            output_file = tokens[i + 1].value;
            append_output = 0;
            i++;
        } 
        else if (strcmp(tokens[i].value, ">>") == 0 && i + 1 < end_idx) {
            output_file = tokens[i + 1].value;
            append_output = 1;
            i++;
        } 
        else {
            args[arg_count++] = tokens[i].value;
        }
    }
    args[arg_count] = NULL;

    if (arg_count == 0) exit(0);

    // 2. Set up File Redirection FIRST
    // This ensures built-ins like `pwd` can be redirected to a file
    if (input_file) {
        int fd_in = open(input_file, O_RDONLY);
        if (fd_in < 0) { 
            fprintf(stderr, "cshell: no such file or directory\n"); 
            exit(1); 
        }
        dup2(fd_in, STDIN_FILENO);
        close(fd_in);
    }

    if (output_file) {
        int flags = O_WRONLY | O_CREAT | (append_output ? O_APPEND : O_TRUNC);
        int fd_out = open(output_file, flags, 0644); 
        if (fd_out < 0) { perror("output redirect failed"); exit(1); }
        dup2(fd_out, STDOUT_FILENO);
        close(fd_out);
    }

    // 3. THE THIRD FIX: Check if it's a built-in BEFORE calling resolve_path
    Token slice_tokens[128];
    for (int i = 0; i < arg_count; i++) {
        slice_tokens[i].type = TOKEN_WORD;
        strcpy(slice_tokens[i].value, args[i]);
    }

    if (execute_builtin(slice_tokens, arg_count)) {
        // Built-in executed successfully in the child pipeline process.
        exit(0); 
    }

    // 4. If not a built-in, it must be an external command. Resolve it and execv.
    char *executable = resolve_path(args[0]);
    if (!executable) {
        fprintf(stderr, "%s: command not found\n", args[0]);
        exit(1);
    }

    execv(executable, args);
    perror("execv failed");
    exit(1);
}

// Orchestrates the pipes and forks
int execute_pipeline(Token *tokens, int token_count, int is_bg) {
    int prev_read_fd = -1; 
    int cmd_start = 0;     
    pid_t pids[128];       
    int num_cmds = 0;
    pid_t pipeline_pgid = 0;

    // Build command string for tracking
    char full_cmd[1024] = "";
    for(int i = 0; i < token_count; i++) {
        strcat(full_cmd, tokens[i].value);
        strcat(full_cmd, " ");
    }

    for (int i = 0; i <= token_count; i++) {
        if (i == token_count || strcmp(tokens[i].value, "|") == 0) {
            int fd[2];
            if (i < token_count) pipe(fd);

            pid_t pid = fork();
            if (pid == 0) {
                // --- CHILD PROCESS ---
                if (num_cmds == 0) pipeline_pgid = getpid();
                setpgid(0, pipeline_pgid);
                
                // If foreground, give child terminal
                if (!is_bg) tcsetpgrp(shell_terminal, pipeline_pgid);

                // Reset signals for the child executing the command
                signal(SIGINT, SIG_DFL);
                signal(SIGQUIT, SIG_DFL);
                signal(SIGTSTP, SIG_DFL);
                signal(SIGTTIN, SIG_DFL);
                signal(SIGTTOU, SIG_DFL);

                if (prev_read_fd != -1) { dup2(prev_read_fd, STDIN_FILENO); close(prev_read_fd); }
                if (i < token_count) { dup2(fd[1], STDOUT_FILENO); close(fd[1]); close(fd[0]); }
                execute_single_command(tokens, cmd_start, i);
            } 
            else {
                // --- PARENT PROCESS ---
                if (num_cmds == 0) pipeline_pgid = pid;
                setpgid(pid, pipeline_pgid); // Avoid race condition

                pids[num_cmds++] = pid;
                if (prev_read_fd != -1) close(prev_read_fd);
                if (i < token_count) { close(fd[1]); prev_read_fd = fd[0]; }
            }
            cmd_start = i + 1;
        }
    }

    if (is_bg) {
        add_job(pipeline_pgid, full_cmd, 1);
    } else {
        // Foreground: give terminal to pipeline, wait, reclaim terminal
        tcsetpgrp(shell_terminal, pipeline_pgid);
        
        for (int j = 0; j < num_cmds; j++) {
            int status;
            waitpid(pids[j], &status, WUNTRACED);
            if (WIFSTOPPED(status)) {
                // If user hits Ctrl-Z, mark job stopped
                add_job(pipeline_pgid, full_cmd, 0);
                Job* j_ptr = find_job_by_pgid(pipeline_pgid); // helper you'd need or just rely on add_job return
                printf("\nStopped %s\n", full_cmd);
                break; // stop waiting for pipeline
            }
        }
        tcsetpgrp(shell_terminal, shell_pgid); // Reclaim terminal
    }
    return 1;
}