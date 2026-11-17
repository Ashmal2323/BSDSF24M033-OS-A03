#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_LEN 512
#define MAXARGS 64
#define ARGLEN 256
#define PROMPT "FCIT> "
#define HISTORY_SIZE 20
#define MAX_STAGES 16

/* One stage of a pipeline: a command plus optional redirections */
typedef struct {
    char* argv[MAXARGS + 1];
    char* infile;     /* file after '<', or NULL */
    char* outfile;    /* file after '>', or NULL */
} command_t;

// Function prototypes
char* read_cmd(char* prompt, FILE* fp);
char** tokenize(char* cmdline);
int execute(char** arglist);
int handle_builtin(char** arglist);
void init_readline(void);

// History
void add_to_history(const char* cmd);
void print_history(void);
char* get_history(int n);

// Pipeline / redirection
int parse_pipeline(char** arglist, command_t cmds[], int* ncmds);
int execute_pipeline(command_t cmds[], int ncmds, int background, const char* label);

// Background jobs
int add_job(pid_t pid, const char* cmd);
void reap_jobs(void);
void print_jobs(void);

#endif // SHELL_H
