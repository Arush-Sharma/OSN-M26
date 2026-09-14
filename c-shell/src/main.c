#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/lexer.h"
#include "../include/builtin.h"
#include "../include/commands.h"
#include "../include/jobs.h"

char home_dir[1024];

int execute_pipeline(Token *tokens, int token_count, int is_bg);

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
    getcwd(home_dir, sizeof(home_dir));
    init_shell(); // E2: Initialize process groups and terminal control
    
    char input[1024];
    Token tokens[256]; 

    while (1) {
        // Print async background job status before prompt
        update_jobs_status();
        
        print_prompt();

        if (fgets(input, sizeof(input), stdin) == NULL) {
            // E2: Handle Ctrl-D (EOF) gracefully
            printf("\nExiting...\n");
            break; 
        }

        input[strcspn(input, "\n")] = 0;
        if (strlen(input) == 0) continue;

        int total_tokens = tokenize_input(input, tokens, 256);
        
        // Parts D1 & D2: Split execution by ';' and '&'
        int slice_start = 0;
        for (int i = 0; i <= total_tokens; i++) {
            if (i == total_tokens || strcmp(tokens[i].value, ";") == 0 || strcmp(tokens[i].value, "&") == 0) {
                
                int slice_length = i - slice_start;
                int is_bg = (i < total_tokens && strcmp(tokens[i].value, "&") == 0);

                if (slice_length > 0) {
                    Token *slice = &tokens[slice_start];
                    
                    // Exotic Builtin Routing (E & F)
                    if (strcmp(slice[0].value, "activities") == 0) { execute_activities(); }
                    else if (strcmp(slice[0].value, "ping") == 0) { 
                        char *p_args[10];
                        for(int k=1; k<slice_length; k++) p_args[k-1] = slice[k].value;
                        execute_ping(p_args, slice_length-1);
                    }
                    else if (strcmp(slice[0].value, "resume") == 0) {
                        char *r_args[10];
                        for(int k=1; k<slice_length; k++) r_args[k-1] = slice[k].value;
                        execute_resume(r_args, slice_length-1);
                    }
                    else if (strcmp(slice[0].value, "spy") == 0) {
                        char *s_args[1];
                        s_args[0] = (slice_length > 1) ? slice[1].value : NULL;
                        execute_spy(s_args, slice_length > 1 ? 1 : 0);
                    }
                    else if (strcmp(slice[0].value, "snoop") == 0) {
                        char *sn_args[10];
                        for(int k=1; k<slice_length; k++) sn_args[k-1] = slice[k].value;
                        sn_args[slice_length-1] = NULL;
                        execute_snoop(sn_args, slice_length-1);
                    }
                    // Standard execution
                    else if (!execute_builtin(slice, slice_length)) {
                        execute_pipeline(slice, slice_length, is_bg);
                    }
                }
                slice_start = i + 1;
            }
        }
    }

    return 0;
}