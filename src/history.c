#include "shell.h"

static char* history[HISTORY_SIZE];
static int hist_count = 0;

/* Store a copy of cmd. If full, drop the oldest entry. */
void add_to_history(const char* cmd) {
    if (hist_count == HISTORY_SIZE) {
        free(history[0]);
        for (int i = 1; i < HISTORY_SIZE; i++)
            history[i - 1] = history[i];
        hist_count--;
    }
    history[hist_count++] = strdup(cmd);
}

/* Print all stored commands, numbered from 1 */
void print_history(void) {
    for (int i = 0; i < hist_count; i++)
        printf("%d  %s\n", i + 1, history[i]);
}

/* Return the n-th command (1-based) or NULL if out of range */
char* get_history(int n) {
    if (n < 1 || n > hist_count)
        return NULL;
    return history[n - 1];
}
