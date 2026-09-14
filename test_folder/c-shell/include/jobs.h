#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>
#include <termios.h>

#define MAX_JOBS 128

typedef struct {
    int job_id;
    pid_t pgid;
    char command[1024];
    int is_running; // 1 for Running, 0 for Stopped
    int is_active;  // 1 if tracked, 0 if empty slot
} Job;

extern Job jobs[MAX_JOBS];
extern int next_job_id;
extern pid_t shell_pgid;
extern struct termios shell_tmodes;
extern int shell_terminal;

void init_shell();
void add_job(pid_t pgid, const char *cmd, int is_running);
void remove_job(pid_t pgid);
void update_jobs_status();
Job* find_job_by_id(int job_id);
Job* find_job_by_pgid(pid_t pgid);

#endif