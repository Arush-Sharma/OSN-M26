#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/lexer.h"
#include "../include/builtin.h"

char home_dir[1024];

void print_prompt() {
    char cwd[1024];
    char hostname[256];
    char *username = getenv("USER");

    if (username == NULL) username = "user";
    gethostname(hostname, sizeof(hostname));
    getcwd(cwd, sizeof(cwd));

    char display_cwd[1024];
    if (strncmp(cwd, home_dir, strlen(home_dir)) == 0) {
        snprintf(display_cwd, sizeof(display_cwd), "~%s", cwd + strlen(home_dir));
    } else {
        strncpy(display_cwd, cwd, sizeof(display_cwd));
    }

    printf("<%s@%s:%s> ", username, hostname, display_cwd);
    fflush(stdout); 
}

int main() {
    // A1: Save starting directory for the ~ replacement
    getcwd(home_dir, sizeof(home_dir));
    
    char input[1024];
    Token tokens[128]; 

    while (1) {
        print_prompt();

        // A2: Consume Input
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");
            break; 
        }

        input[strcspn(input, "\n")] = 0;

        if (strlen(input) == 0) {
            continue;
        }

        // A3: Tokenize
        int token_count = tokenize_input(input, tokens, 128);
        
        // Part B: Try to execute as a built-in command
        if (token_count > 0) {
            if (!execute_builtin(tokens, token_count)) {
                execute_pipeline(tokens, token_count);
            }
        }
    }

    return 0;
}