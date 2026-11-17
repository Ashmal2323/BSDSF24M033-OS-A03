#include "shell.h"
#include <ctype.h>

/* Shell variables are kept in a singly linked list of name/value pairs */
typedef struct var {
    char* name;
    char* value;
    struct var* next;
} var_t;

static var_t* head = NULL;

/* True if tok looks like NAME=value, where NAME is a valid identifier */
int is_assignment(const char* tok) {
    const char* eq = strchr(tok, '=');
    if (eq == NULL || eq == tok)
        return 0;
    if (!(isalpha((unsigned char)tok[0]) || tok[0] == '_'))
        return 0;
    for (const char* p = tok; p < eq; p++)
        if (!(isalnum((unsigned char)*p) || *p == '_'))
            return 0;
    return 1;
}

/* Create the variable, or overwrite its value if it already exists */
void set_var(const char* name, const char* value) {
    for (var_t* v = head; v != NULL; v = v->next) {
        if (strcmp(v->name, name) == 0) {
            free(v->value);
            v->value = strdup(value);
            return;
        }
    }
    var_t* node = malloc(sizeof(var_t));
    node->name = strdup(name);
    node->value = strdup(value);
    node->next = NULL;

    if (head == NULL) {
        head = node;
    } else {
        var_t* tail = head;
        while (tail->next != NULL)
            tail = tail->next;
        tail->next = node;               /* keep definition order */
    }
}

/* Look in shell variables first, then in the environment ($HOME, $USER...).
   Returns NULL if the variable does not exist. */
const char* get_var(const char* name) {
    for (var_t* v = head; v != NULL; v = v->next)
        if (strcmp(v->name, name) == 0)
            return v->value;
    return getenv(name);
}

/* Handle one NAME=value token (a value like $OTHER is expanded) */
void assign_var(const char* tok) {
    const char* eq = strchr(tok, '=');
    char* name = strndup(tok, eq - tok);
    const char* value = eq + 1;

    if (value[0] == '$' && value[1] != '\0') {
        const char* v = get_var(value + 1);
        value = v ? v : "";
    }
    set_var(name, value);
    free(name);
}

/* The 'set' built-in */
void print_vars(void) {
    for (var_t* v = head; v != NULL; v = v->next)
        printf("%s=%s\n", v->name, v->value);
}
