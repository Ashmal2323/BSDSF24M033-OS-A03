#include "shell.h"
#include <readline/readline.h>
#include <readline/history.h>

int main() {
    char* cmdline;
    char** arglist;

    init_readline();

    /* readline() prints the prompt, handles editing, Tab and arrow keys.
       It returns NULL on Ctrl+D. */
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

            if (!handle_builtin(arglist)) {
                execute(arglist);
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
