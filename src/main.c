#include "shell.h"

int main() {
    char* cmdline;
    char** arglist;

    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {

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
            add_to_history(cmdline);         /* only non-empty commands are stored */

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
