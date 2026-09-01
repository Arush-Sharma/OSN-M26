#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "../include/builtin.h"

int execute_builtin(Token *tokens, int token_count) {
    if (token_count == 0 || tokens[0].type != TOKEN_WORD) {
        return 0; // Not a built-in
    }

    char *cmd = tokens[0].value;

    if (strcmp(cmd, "exit") == 0) {
        exit(0);
    }

    if (strcmp(cmd, "cd") == 0) {
        char *path = (token_count > 1) ? tokens[1].value : getenv("HOME");
        if (chdir(path) != 0) {
            perror("cd");
        }
        return 1;
    }

    if (strcmp(cmd, "pwd") == 0) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("pwd");
        }
        return 1;
    }

    return 0; // Command was not a built-in
}