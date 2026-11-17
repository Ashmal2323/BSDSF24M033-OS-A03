#include "shell.h"
#include <readline/readline.h>
#include <readline/history.h>

/* Run one command (possibly a pipeline), in the foreground or background */
static void run_segment(char** seg, int bg) {
    command_t cmds[MAX_STAGES];
    int ncmds = 0;

    if (parse_pipeline(seg, cmds, &ncmds) != 0)
        return;

    /* A plain foreground command may be a built-in that must run
       in the shell process itself (cd, exit, ...) */
    int simple = (ncmds == 1 && cmds[0].infile == NULL && cmds[0].outfile == NULL);
    if (!bg && simple && handle_builtin(cmds[0].argv))
        return;

    /* Text shown by the jobs command */
    char label[MAX_LEN];
    label[0] = '\0';
    for (int i = 0; seg[i] != NULL; i++) {
        strncat(label, seg[i], sizeof(label) - strlen(label) - 1);
        if (seg[i + 1] != NULL)
            strncat(label, " ", sizeof(label) - strlen(label) - 1);
    }

    execute_pipeline(cmds, ncmds, bg, label);
}

/* Split the token list at ';' and '&' and run each part in order */
static void run_line(char** arglist) {
    char* seg[MAXARGS + 1];
    int n = 0;

    for (int i = 0; ; i++) {
        char* tok = arglist[i];
        int is_sep = (tok != NULL) && (strcmp(tok, ";") == 0 || strcmp(tok, "&") == 0);

        if (tok == NULL || is_sep) {
            int bg = (tok != NULL && tok[0] == '&');
            seg[n] = NULL;
            if (n > 0)
                run_segment(seg, bg);
            else if (bg)
                fprintf(stderr, "syntax error near unexpected token `&'\n");
            n = 0;
            if (tok == NULL)
                break;
        } else {
            seg[n++] = tok;
        }
    }
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
