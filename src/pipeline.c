#include "shell.h"
#include <fcntl.h>

/* Split the token list into pipeline stages.
   Returns 0 on success, -1 on a syntax error. */
int parse_pipeline(char** arglist, command_t cmds[], int* ncmds) {
    int n = 0;      /* index of the current stage */
    int argc = 0;   /* words collected for the current stage */

    memset(&cmds[0], 0, sizeof(command_t));

    for (int i = 0; arglist[i] != NULL; i++) {
        char* tok = arglist[i];

        if (strcmp(tok, "|") == 0) {
            if (argc == 0) {
                fprintf(stderr, "syntax error near unexpected token `|'\n");
                return -1;
            }
            cmds[n].argv[argc] = NULL;
            n++;
            if (n >= MAX_STAGES) {
                fprintf(stderr, "too many commands in pipeline\n");
                return -1;
            }
            memset(&cmds[n], 0, sizeof(command_t));
            argc = 0;
        }
        else if (strcmp(tok, "<") == 0 || strcmp(tok, ">") == 0) {
            char* file = arglist[i + 1];
            if (file == NULL || strcmp(file, "<") == 0 ||
                strcmp(file, ">") == 0 || strcmp(file, "|") == 0) {
                fprintf(stderr, "syntax error: missing filename after `%s'\n", tok);
                return -1;
            }
            if (tok[0] == '<')
                cmds[n].infile = file;
            else
                cmds[n].outfile = file;
            i++;                       /* skip the filename token */
        }
        else {
            cmds[n].argv[argc++] = tok;
        }
    }

    if (argc == 0) {
        fprintf(stderr, "syntax error: missing command\n");
        return -1;
    }
    cmds[n].argv[argc] = NULL;
    *ncmds = n + 1;
    return 0;
}

/* Open path and make it the child's target_fd (0 or 1) */
static void redirect(const char* path, int flags, int target_fd) {
    int fd = open(path, flags, 0644);
    if (fd < 0) {
        perror(path);
        exit(1);
    }
    dup2(fd, target_fd);
    close(fd);                          /* original fd no longer needed */
}

/* Run all stages, connected by pipes. Returns the exit status of the last one. */
int execute_pipeline(command_t cmds[], int ncmds) {
    pid_t pids[MAX_STAGES];
    int started = 0;
    int prev_read = -1;                 /* read end of the previous pipe */

    fflush(stdout);                     /* avoid duplicated buffered output after fork */

    for (int i = 0; i < ncmds; i++) {
        int fd[2] = {-1, -1};

        if (i < ncmds - 1 && pipe(fd) < 0) {
            perror("pipe");
            break;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            if (fd[0] != -1) close(fd[0]);
            if (fd[1] != -1) close(fd[1]);
            break;
        }

        if (pid == 0) {
            /* ---- child ---- */
            if (prev_read != -1) {                  /* read from previous stage */
                dup2(prev_read, STDIN_FILENO);
                close(prev_read);
            }
            if (fd[1] != -1) {                      /* write to next stage */
                dup2(fd[1], STDOUT_FILENO);
                close(fd[0]);
                close(fd[1]);
            }
            /* Explicit < and > override the pipe, as in bash */
            if (cmds[i].infile)
                redirect(cmds[i].infile, O_RDONLY, STDIN_FILENO);
            if (cmds[i].outfile)
                redirect(cmds[i].outfile, O_WRONLY | O_CREAT | O_TRUNC, STDOUT_FILENO);

            if (handle_builtin(cmds[i].argv))       /* e.g. history | grep ls */
                exit(0);

            execvp(cmds[i].argv[0], cmds[i].argv);
            fprintf(stderr, "%s: command not found\n", cmds[i].argv[0]);
            exit(127);
        }

        /* ---- parent ---- */
        pids[started++] = pid;
        if (prev_read != -1) close(prev_read);      /* done with the old read end */
        if (fd[1] != -1) close(fd[1]);              /* parent never writes */
        prev_read = fd[0];                          /* keep for the next stage */
    }

    if (prev_read != -1) close(prev_read);          /* only left open after an error */

    int last_status = 1;
    for (int i = 0; i < started; i++) {
        int st;
        waitpid(pids[i], &st, 0);
        last_status = WIFEXITED(st) ? WEXITSTATUS(st) : 128 + WTERMSIG(st);
    }
    return last_status;
}
