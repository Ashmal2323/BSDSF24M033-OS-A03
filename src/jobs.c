#include "shell.h"

#define MAX_JOBS 64

typedef struct {
    pid_t pid;
    char  cmd[MAX_LEN];
    int   active;
} job_t;

static job_t jobs[MAX_JOBS];

/* Remember a background job. Returns its job number (1-based) or 0 if full. */
int add_job(pid_t pid, const char* cmd) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!jobs[i].active) {
            jobs[i].pid = pid;
            jobs[i].active = 1;
            strncpy(jobs[i].cmd, cmd, MAX_LEN - 1);
            jobs[i].cmd[MAX_LEN - 1] = '\0';
            return i + 1;
        }
    }
    fprintf(stderr, "job table full\n");
    return 0;
}

/* Collect finished children without blocking (this removes zombies). */
void reap_jobs(void) {
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < MAX_JOBS; i++) {
            if (jobs[i].active && jobs[i].pid == pid) {
                printf("[%d]+ Done    %s\n", i + 1, jobs[i].cmd);
                jobs[i].active = 0;
                break;
            }
        }
    }
}

/* The 'jobs' built-in */
void print_jobs(void) {
    int any = 0;
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].active) {
            printf("[%d] %d Running    %s\n", i + 1, (int)jobs[i].pid, jobs[i].cmd);
            any = 1;
        }
    }
    if (!any)
        printf("No background jobs.\n");
}
