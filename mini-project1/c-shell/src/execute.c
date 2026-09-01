#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "../include/lexer.h"
#include "../include/builtin.h"

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
int execute_pipeline(Token *tokens, int token_count) {
    int prev_read_fd = -1; // To hold the read end of the previous pipe
    int cmd_start = 0;     // Starting index of the current command slice
    
    pid_t pids[128];       // Keep track of all child processes
    int num_cmds = 0;

    for (int i = 0; i <= token_count; i++) {
        // A command ends when we hit a pipe OR the end of the tokens
        if (i == token_count || strcmp(tokens[i].value, "|") == 0) {
            
            int fd[2];
            // Only create a new pipe if this IS NOT the last command
            if (i < token_count) {
                if (pipe(fd) < 0) {
                    perror("pipe failed");
                    return 1;
                }
            }

            pid_t pid = fork();
            if (pid < 0) {
                perror("fork failed");
                return 1;
            } 
            else if (pid == 0) {
                // --- CHILD PROCESS ---
                
                // If there is a previous pipe, read from it instead of STDIN
                if (prev_read_fd != -1) {
                    dup2(prev_read_fd, STDIN_FILENO);
                    close(prev_read_fd);
                }

                // If this isn't the last command, write to the new pipe instead of STDOUT
                if (i < token_count) {
                    dup2(fd[1], STDOUT_FILENO);
                    close(fd[1]);
                    close(fd[0]); // The child doesn't read from its own output pipe
                }

                execute_single_command(tokens, cmd_start, i);
            } 
            else {
                // --- PARENT PROCESS ---
                pids[num_cmds++] = pid;

                // Close the old read end (the child is using it now)
                if (prev_read_fd != -1) {
                    close(prev_read_fd);
                }

                // Close the write end of the NEW pipe (CRITICAL!)
                // If the parent leaves this open, the next child will hang forever.
                if (i < token_count) {
                    close(fd[1]);
                    prev_read_fd = fd[0]; // Save read end for the NEXT loop iteration
                }
            }
            
            // Move the start index past the "|" for the next command
            cmd_start = i + 1;
        }
    }

    // Wait for all commands in the pipeline to finish
    for (int j = 0; j < num_cmds; j++) {
        waitpid(pids[j], NULL, 0);
    }

    return 1;
}