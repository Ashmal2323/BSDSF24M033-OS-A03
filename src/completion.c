#include "shell.h"
#include <readline/readline.h>
#include <readline/history.h>

static const char* builtins[] = {"cd", "exit", "help", "history", "jobs", NULL};

/* Called repeatedly by readline; returns one matching built-in per call */
static char* builtin_generator(const char* text, int state) {
    static int idx, len;
    if (state == 0) {
        idx = 0;
        len = strlen(text);
    }
    while (builtins[idx] != NULL) {
        const char* name = builtins[idx++];
        if (strncmp(name, text, len) == 0)
            return strdup(name);
    }
    return NULL;
}

/* First word on the line: complete built-in names.
   Otherwise (or if nothing matched) return NULL so readline
   falls back to its default filename completion. */
static char** shell_completion(const char* text, int start, int end) {
    (void)end;
    if (start == 0)
        return rl_completion_matches(text, builtin_generator);
    return NULL;
}

void init_readline(void) {
    rl_attempted_completion_function = shell_completion;
}
