#include "builtins.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int create_builtin(struct scope *s, char *name)
{
    struct def_list *dl = calloc(1, sizeof(struct def_list));

    char *tmp = calloc(1, sizeof(char) * (strlen(name) + 1));
    strcpy(tmp, name);

    dl->name = tmp;
    dl->builtin = 1;
    dl->type = _func;
    dl->next = s->defs;
    s->defs = dl;

    return 1;
}

void register_builtins(struct scope *s)
{
    create_builtin(s, "print");
    create_builtin(s, "println");
}

int _print(int val) {
    return printf("%d ", val);
}

int _println(int val) {
    return printf("%d\n", val);
}