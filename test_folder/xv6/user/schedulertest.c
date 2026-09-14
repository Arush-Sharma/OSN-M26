#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main() {
    int n = 3; 
    for(int i = 0; i < n; i++) {
        int pid = fork();
        if(pid < 0) {
            printf("fork failed\n");
            exit(1);
        } else if(pid == 0) {
            // Child process: Burn CPU cycles
            for(volatile int j = 0; j < 100000000; j++) {
                // Do nothing, just consume time slices
            }
            printf("Process %d finished\n", getpid());
            exit(0);
        }
    }
    
    for(int i = 0; i < n; i++) {
        wait(0);
    }
    printf("All test processes finished.\n");
    exit(0);
}