#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "../include/builtin.h"

// In-memory previous directory (does not persist across sessions per Q12)
static char prev_dir[1024] = "";

typedef struct {
    char path[1024];
    int score;
} FrecencyEntry;

// Helper: Get the path to the hidden frecency file
void get_frecency_file(char *buffer) {
    snprintf(buffer, 1024, "%s/.cshell_frecency", home_dir);
}

// Helper: Update the score of a directory
void record_hop(const char *path) {
    char file_path[1024];
    get_frecency_file(file_path);
    
    FrecencyEntry entries[500];
    int count = 0;
    
    // 1. Read existing scores
    FILE *f = fopen(file_path, "r");
    if (f) {
        while (fscanf(f, "%1023s %d", entries[count].path, &entries[count].score) == 2) {
            count++;
        }
        fclose(f);
    }
    
    // 2. Update existing or add new
    int found = 0;
    for (int i = 0; i < count; i++) {
        if (strcmp(entries[i].path, path) == 0) {
            entries[i].score++;
            found = 1;
            break;
        }
    }
    if (!found && count < 500) {
        strcpy(entries[count].path, path);
        entries[count].score = 1;
        count++;
    }
    
    // 3. Save back to file
    f = fopen(file_path, "w");
    if (f) {
        for (int i = 0; i < count; i++) {
            fprintf(f, "%s %d\n", entries[i].path, entries[i].score);
        }
        fclose(f);
    }
}

// Helper: Find the highest scoring path containing the substring
char* find_frecency_match(const char *search_term) {
    char file_path[1024];
    get_frecency_file(file_path);
    
    static char best_match[1024];
    int highest_score = -1;
    
    FILE *f = fopen(file_path, "r");
    if (!f) return NULL;
    
    char path[1024];
    int score;
    while (fscanf(f, "%1023s %d", path, &score) == 2) {
        // Look for substring (Q38)
        if (strstr(path, search_term) != NULL) {
            if (score > highest_score) {
                highest_score = score;
                strcpy(best_match, path);
            }
        }
    }
    fclose(f);
    
    if (highest_score == -1) return NULL;
    return best_match;
}

int execute_builtin(Token *tokens, int token_count) {
    if (token_count == 0 || tokens[0].type != TOKEN_WORD) {
        return 0;
    }

    char *cmd = tokens[0].value;

    if (strcmp(cmd, "exit") == 0) {
        exit(0);
    }

    if (strcmp(cmd, "pwd") == 0) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) printf("%s\n", cwd);
        else perror("pwd");
        return 1;
    }

    // --- NEW HOP IMPLEMENTATION ---
    if (strcmp(cmd, "hop") == 0) {
        // If no arguments, fake it as 'hop ~'
        if (token_count == 1) {
            strcpy(tokens[1].value, "~");
            token_count = 2;
        }

        // Process sequentially (Q10)
        for (int i = 1; i < token_count; i++) {
            char *target = tokens[i].value;
            char current_cwd[1024];
            getcwd(current_cwd, sizeof(current_cwd));

            if (strcmp(target, ".") == 0) {
                continue; // No-op, no frecency recorded (Q23)
            }

            char next_dir[1024];
            
            // Handle "-"
            if (strcmp(target, "-") == 0) {
                if (strlen(prev_dir) == 0) {
                    fprintf(stderr, "hop: no previous directory\n");
                    return 1; 
                }
                strcpy(next_dir, prev_dir);
            } 
            // Handle "~" expansion (e.g., "~" or "~/Documents")
            else if (target[0] == '~') {
                snprintf(next_dir, sizeof(next_dir), "%s%s", home_dir, target + 1);
            } 
            else {
                strcpy(next_dir, target);
            }

            // Attempt to jump normally
            if (chdir(next_dir) != 0) {
                // If it fails, attempt Frecency Substring Jump
                char *frecent = find_frecency_match(target);
                if (frecent != NULL && chdir(frecent) == 0) {
                    // Success via frecency!
                } else {
                    fprintf(stderr, "hop: no such directory\n");
                    return 1; // Stop processing further arguments (Q40)
                }
            }

            // Success: Update state and frecency
            strcpy(prev_dir, current_cwd);
            
            char new_cwd[1024];
            getcwd(new_cwd, sizeof(new_cwd));
            record_hop(new_cwd);
            
            // Print absolute path of where we landed
            printf("%s\n", new_cwd);
        }
        return 1;
    }

    return 0; 
}