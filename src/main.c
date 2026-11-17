#include "shell.h"
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_BLOCK 64

/* Run one command (possibly a pipeline). Returns its exit status. */
static int run_segment(char** seg, int bg) {
    command_t cmds[MAX_STAGES];
    int ncmds = 0;

    if (parse_pipeline(seg, cmds, &ncmds) != 0)
        return 1;

    /* A plain foreground command may be a built-in that must run
       in the shell process itself (cd, exit, ...) */
    int simple = (ncmds == 1 && cmds[0].infile == NULL && cmds[0].outfile == NULL);
    if (!bg && simple && handle_builtin(cmds[0].argv))
        return 0;

    /* Text shown by the jobs command */
    char label[MAX_LEN];
    label[0] = '\0';
    for (int i = 0; seg[i] != NULL; i++) {
        strncat(label, seg[i], sizeof(label) - strlen(label) - 1);
        if (seg[i + 1] != NULL)
            strncat(label, " ", sizeof(label) - strlen(label) - 1);
    }

    return execute_pipeline(cmds, ncmds, bg, label);
}

/* Split the token list at ';' and '&', run each part in order.
   Returns the exit status of the last command run. */
static int run_line(char** arglist) {
    char* seg[MAXARGS + 1];
    int n = 0;
    int status = 0;

    for (int i = 0; ; i++) {
        char* tok = arglist[i];
        int is_sep = (tok != NULL) && (strcmp(tok, ";") == 0 || strcmp(tok, "&") == 0);

        if (tok == NULL || is_sep) {
            int bg = (tok != NULL && tok[0] == '&');
            seg[n] = NULL;
            if (n > 0)
                status = run_segment(seg, bg);
            else if (bg) {
                fprintf(stderr, "syntax error near unexpected token `&'\n");
                status = 1;
            }
            n = 0;
            if (tok == NULL)
                break;
        } else {
            seg[n++] = tok;
        }
    }
    return status;
}

/* Tokenize and run one line of text. Returns its exit status. */
static int run_text(const char* text) {
    char** arglist = tokenize((char*)text);
    if (arglist == NULL)
        return 0;
    int status = run_line(arglist);
    for (int i = 0; arglist[i] != NULL; i++)
        free(arglist[i]);
    free(arglist);
    return status;
}

/* ---------- if / then / else / fi ---------- */

static int is_blank(const char* s) {
    while (*s == ' ' || *s == '\t') s++;
    return *s == '\0';
}

/* True if the line contains only the keyword kw (spaces allowed around it) */
static int is_kw(const char* line, const char* kw) {
    while (*line == ' ' || *line == '\t') line++;
    size_t n = strlen(kw);
    if (strncmp(line, kw, n) != 0)
        return 0;
    return is_blank(line + n);
}

static void free_block(char** block, int n) {
    for (int i = 0; i < n; i++)
        free(block[i]);
}

/* cmdline is the first line: "if <command>". Reads the following lines
   until "fi", then runs the then-block or the else-block. */
static void handle_if(const char* cmdline) {
    const char* cond = cmdline;
    while (*cond == ' ' || *cond == '\t') cond++;
    cond += 2;                                   /* skip the word "if" */

    if (is_blank(cond)) {
        fprintf(stderr, "syntax error: missing condition after 'if'\n");
        return;
    }

    char* then_lines[MAX_BLOCK];
    char* else_lines[MAX_BLOCK];
    int nthen = 0, nelse = 0;
    int state = 0;                               /* 0: expect then, 1: then-block, 2: else-block */
    int complete = 0;
    char* line;

    while ((line = readline("> ")) != NULL) {
        if (is_blank(line)) {
            free(line);
            continue;
        }

        if (state == 0) {
            if (is_kw(line, "then")) {
                state = 1;
                free(line);
                continue;
            }
            fprintf(stderr, "syntax error: expected 'then'\n");
            free(line);
            break;
        }

        if (is_kw(line, "fi")) {
            free(line);
            complete = 1;
            break;
        }

        if (state == 1 && is_kw(line, "else")) {
            state = 2;
            free(line);
            continue;
        }

        if (state == 1 && nthen < MAX_BLOCK)
            then_lines[nthen++] = line;
        else if (state == 2 && nelse < MAX_BLOCK)
            else_lines[nelse++] = line;
        else
            free(line);                          /* block too long: drop the extra line */
    }

    if (!complete) {
        fprintf(stderr, "syntax error: if block not closed with 'fi'\n");
    } else {
        int status = run_text(cond);             /* exit status of the if-command */
        char** block = (status == 0) ? then_lines : else_lines;
        int n = (status == 0) ? nthen : nelse;
        for (int i = 0; i < n; i++)
            run_text(block[i]);
    }

    free_block(then_lines, nthen);
    free_block(else_lines, nelse);
}

int main() {
    char* cmdline;
    char** arglist;

    init_readline();

    for (;;) {
        reap_jobs();                         /* collect finished background jobs (no zombies) */

        if ((cmdline = readline(PROMPT)) == NULL)
            break;                           /* Ctrl+D */

        /* Handle !n BEFORE tokenizing and BEFORE adding to history */
        if (cmdline[0] == '!') {
            char* end;
            long n = strtol(cmdline + 1, &end, 10);
            char* hist_cmd = (end == cmdline + 1) ? NULL : get_history((int)n);

            if (hist_cmd == NULL) {
                fprintf(stderr, "%s: event not found\n", cmdline);
                free(cmdline);
                continue;
            }
            free(cmdline);
            cmdline = strdup(hist_cmd);      /* replace !n with the real command */
            printf("%s\n", cmdline);         /* show what is being re-run */
        }

        if ((arglist = tokenize(cmdline)) != NULL) {
            add_history(cmdline);            /* readline's history (arrow keys) */
            add_to_history(cmdline);         /* our own history (history, !n) */

            if (strcmp(arglist[0], "if") == 0)
                handle_if(cmdline);          /* multi-line if block */
            else
                run_line(arglist);

            // Free the memory allocated by tokenize()
            for (int i = 0; arglist[i] != NULL; i++) {
                free(arglist[i]);
            }
            free(arglist);
        }
        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
