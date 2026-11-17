#include "shell.h"

char* read_cmd(char* prompt, FILE* fp) {
    printf("%s", prompt);
    char* cmdline = (char*) malloc(sizeof(char) * MAX_LEN);
    int c, pos = 0;

    while ((c = getc(fp)) != EOF) {
        if (c == '\n') break;
        cmdline[pos++] = c;
    }

    if (c == EOF && pos == 0) {
        free(cmdline);
        return NULL; // Handle Ctrl+D
    }

    cmdline[pos] = '\0';
    return cmdline;
}

char** tokenize(char* cmdline) {
    // Edge case: empty command line
    if (cmdline == NULL || cmdline[0] == '\0' || cmdline[0] == '\n') {
        return NULL;
    }

    char** arglist = (char**)malloc(sizeof(char*) * (MAXARGS + 1));
    for (int i = 0; i < MAXARGS + 1; i++) {
        arglist[i] = (char*)malloc(sizeof(char) * ARGLEN);
        memset(arglist[i], 0, ARGLEN);
    }

    char* cp = cmdline;
    int argnum = 0;

    while (*cp != '\0' && argnum < MAXARGS) {
        while (*cp == ' ' || *cp == '\t') cp++;      // Skip whitespace
        if (*cp == '\0') break;

        /* Operators are tokens of their own: < > | */
        if (*cp == '<' || *cp == '>' || *cp == '|') {
            arglist[argnum][0] = *cp;
            arglist[argnum][1] = '\0';
            argnum++;
            cp++;
            continue;
        }

        /* Ordinary word; quotes group characters and are stripped */
        int len = 0;
        char quote = 0;
        while (*cp != '\0') {
            if (quote) {
                if (*cp == quote) { quote = 0; cp++; continue; }
            } else {
                if (*cp == '"' || *cp == '\'') { quote = *cp; cp++; continue; }
                if (*cp == ' ' || *cp == '\t' || *cp == '<' || *cp == '>' || *cp == '|')
                    break;
            }
            if (len < ARGLEN - 1)
                arglist[argnum][len++] = *cp;
            cp++;
        }
        arglist[argnum][len] = '\0';
        argnum++;
    }

    if (argnum == 0) { // No arguments were parsed
        for (int i = 0; i < MAXARGS + 1; i++) free(arglist[i]);
        free(arglist);
        return NULL;
    }

    /* Free the unused slots so nothing leaks, then terminate the list */
    for (int i = argnum; i < MAXARGS + 1; i++) free(arglist[i]);
    arglist[argnum] = NULL;
    return arglist;
}

/* Returns 1 if arglist[0] was a built-in (and has been handled), 0 otherwise */
int handle_builtin(char** arglist) {
    if (arglist == NULL || arglist[0] == NULL)
        return 0;

    if (strcmp(arglist[0], "exit") == 0) {
        printf("Shell exited.\n");
        exit(0);
    }

    if (strcmp(arglist[0], "cd") == 0) {
        const char* target = arglist[1];
        if (target == NULL)
            target = getenv("HOME");          /* plain "cd" goes home */
        if (target == NULL || chdir(target) != 0)
            fprintf(stderr, "cd: %s: %s\n",
                    arglist[1] ? arglist[1] : "HOME", strerror(errno));
        return 1;
    }

    if (strcmp(arglist[0], "help") == 0) {
        printf("Built-in commands:\n");
        printf("  cd <directory>  - change the current working directory\n");
        printf("  exit            - terminate the shell\n");
        printf("  help            - display this help message\n");
        printf("  jobs            - list background jobs (not yet implemented)\n");
        printf("  history         - show command history (use !n to re-run command n)\n");
        return 1;
    }

    if (strcmp(arglist[0], "jobs") == 0) {
        printf("Job control not yet implemented.\n");
        return 1;
    }

    if (strcmp(arglist[0], "history") == 0) {
        print_history();
        return 1;
    }

    return 0;
}
