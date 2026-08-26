#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_RESET   "\x1b[0m"

int main(){
    char input[1030];
    char* args[516];
    char hostname[50];
    char cwd[5000];

    while (1)
    {
        char* username = getenv("USER");
        if (!username)
        {
            username= "anonymous";
        }
        
        
    }
    
}