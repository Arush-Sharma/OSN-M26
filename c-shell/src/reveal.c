#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include "../include/commands.h"

// Expose home_dir from main.c for ~ resolution
extern char home_dir[1024]; 

void do_reveal(const char *path, int show_hidden, int tree) {
    struct dirent **namelist;
    int n = scandir(path, &namelist, NULL, alphasort);
    if (n < 0) {
        fprintf(stderr, "reveal: no such directory\n");
        return;
    }

    for (int i = 0; i < n; i++) {
        char *name = namelist[i]->d_name;
        
        // Handle hidden files
        if (!show_hidden && name[0] == '.') {
            free(namelist[i]);
            continue;
        }
        
        // Skip . and .. for recursive calls to prevent infinite loops
        if (tree && (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)) {
            free(namelist[i]);
            continue;
        }

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, name);

        struct stat st;
        if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) {
            if (tree) {
                printf("%s/\n", name);
                do_reveal(full_path, show_hidden, tree);
            } else {
                printf("%s\n", name);
            }
        } else {
            printf("%s\n", name);
        }
        free(namelist[i]);
    }
    free(namelist);
}

void execute_reveal(char **args, int arg_count) {
    int show_hidden = 0;
    int tree = 0;
    char target_path[1024] = "."; // Default to current directory

    // Parse flags and path
    for (int i = 0; i < arg_count; i++) {
        if (args[i][0] == '-') {
            for (size_t j = 1; j < strlen(args[i]); j++) {
                if (args[i][j] == 'a') show_hidden = 1;
                else if (args[i][j] == 't') tree = 1;
                else {
                    fprintf(stderr, "reveal: invalid syntax\n");
                    return;
                }
            }
        } else {
            // Path resolution
            if (args[i][0] == '~') {
                snprintf(target_path, sizeof(target_path), "%s%s", home_dir, args[i] + 1);
            } else {
                strncpy(target_path, args[i], sizeof(target_path));
            }
        }
    }

    do_reveal(target_path, show_hidden, tree);
}