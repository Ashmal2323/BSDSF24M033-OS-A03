#include "shell.h"
#include <readline/readline.h>
#include <readline/history.h>

int main() {
    char* cmdline;
    char** arglist;

    init_readline();

    while ((cmdline = readline(PROMPT)) != NULL) {

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

            command_t cmds[MAX_STAGES];
            int ncmds = 0;

            if (parse_pipeline(arglist, cmds, &ncmds) == 0) {
                /* A plain command (no pipe, no redirection) may be a built-in
                   that must run in the shell process itself (cd, exit, ...) */
                int simple = (ncmds == 1 && cmds[0].infile == NULL
                                         && cmds[0].outfile == NULL);
                if (!(simple && handle_builtin(cmds[0].argv)))
                    execute_pipeline(cmds, ncmds);
            }

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
