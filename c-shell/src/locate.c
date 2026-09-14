#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/commands.h"

void execute_locate(char **args, int arg_count) {
    if (arg_count == 0) {
        fprintf(stderr, "locate: invalid syntax\n");
        return;
    }

    for (int i = 0; i < arg_count; i++) {
        int found = 0;
        char *cmd = args[i];
        char cwd[1024];
        char full_path[1024];

        // 1. Check Current Working Directory
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            snprintf(full_path, sizeof(full_path), "%s/%s", cwd, cmd);
            if (access(full_path, X_OK) == 0) {
                printf("%s\n", full_path);
                found = 1;
            }
        }

        // 2. Check PATH environment variable
        char *path_env = getenv("PATH");
        if (path_env) {
            char *path_copy = strdup(path_env);
            char *dir = strtok(path_copy, ":");
            while (dir != NULL) {
                snprintf(full_path, sizeof(full_path), "%s/%s", dir, cmd);
                // Check if file exists and is executable
                if (access(full_path, X_OK) == 0) {
                    printf("%s\n", full_path);
                    found = 1;
                }
                dir = strtok(NULL, ":");
            }
            free(path_copy);
        }

        if (!found) {
            fprintf(stderr, "locate: command not found (%s)\n", cmd);
        }
    }
}