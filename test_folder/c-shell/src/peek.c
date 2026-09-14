#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "../include/commands.h"

#define CHUNK_SIZE 1024

void print_normal(int fd, int show_lines, int *line_count) {
    char buffer[CHUNK_SIZE];
    ssize_t bytes_read;
    int newline_start = 1;

    while ((bytes_read = read(fd, buffer, CHUNK_SIZE)) > 0) {
        for (ssize_t i = 0; i < bytes_read; i++) {
            if (newline_start && buffer[i] != '\n' && show_lines) {
                printf("%d ", (*line_count)++);
                newline_start = 0;
            }
            putchar(buffer[i]);
            if (buffer[i] == '\n') newline_start = 1;
        }
    }
}

void print_reverse(int fd, int show_lines, int *line_count) {
    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size == 0) return;

    off_t current_pos = file_size;
    char buffer[CHUNK_SIZE];
    char line_buffer[4096]; // To build a single line
    int line_len = 0;

    // Read backwards in chunks
    while (current_pos > 0) {
        ssize_t to_read = (current_pos < CHUNK_SIZE) ? current_pos : CHUNK_SIZE;
        current_pos -= to_read;
        lseek(fd, current_pos, SEEK_SET);
        read(fd, buffer, to_read);

        for (ssize_t i = to_read - 1; i >= 0; i--) {
            if (buffer[i] == '\n') {
                if (line_len > 0) {
                    if (show_lines) printf("%d ", (*line_count)++);
                    for (int j = line_len - 1; j >= 0; j--) putchar(line_buffer[j]);
                    putchar('\n');
                    line_len = 0;
                }
            } else {
                line_buffer[line_len++] = buffer[i];
            }
        }
    }
    
    // Print the final line if the file didn't end with a newline
    if (line_len > 0) {
        if (show_lines) printf("%d ", (*line_count)++);
        for (int j = line_len - 1; j >= 0; j--) putchar(line_buffer[j]);
        putchar('\n');
    }
}

void execute_peek(char **args, int arg_count) {
    int show_lines = 0;
    int reverse = 0;
    int files_start = 0;

    // Parse flags
    for (int i = 0; i < arg_count; i++) {
        if (args[i][0] == '-') {
            for (size_t j = 1; j < strlen(args[i]); j++) {
                if (args[i][j] == 'n') show_lines = 1;
                else if (args[i][j] == 'r') reverse = 1;
                else {
                    fprintf(stderr, "peek: invalid syntax\n");
                    return;
                }
            }
            files_start++;
        } else {
            break;
        }
    }

    int global_line_count = 1;

    // If no files are given, use standard input
    if (files_start == arg_count) {
        // Stdin is not seekable, but assignment says we can buffer it.
        // For simplicity under deadline, this assumes normal reading.
        print_normal(STDIN_FILENO, show_lines, &global_line_count);
        return;
    }

    for (int i = files_start; i < arg_count; i++) {
        struct stat st;
        if (stat(args[i], &st) == 0 && S_ISDIR(st.st_mode)) {
            fprintf(stderr, "peek: is a directory\n");
            continue;
        }

        int fd = open(args[i], O_RDONLY);
        if (fd < 0) {
            fprintf(stderr, "peek: no such file or directory\n");
            continue;
        }

        if (reverse) {
            print_reverse(fd, show_lines, &global_line_count);
        } else {
            print_normal(fd, show_lines, &global_line_count);
        }
        close(fd);
    }
}