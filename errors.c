#include "errors.h"
#include "my_parser.h"
#include "my_calc.h"
#include "errors.h"
#include <stdio.h>
#include <stdlib.h>

void safe_exit(struct parser *p, struct ast *a)
{

    char *errline = get_line_error(p);
    struct position pos;
    count_lines(p, &pos);
    fprintf(stderr, "Error line: %d, col: %d\n", pos.line, pos.col);
    fprintf(stderr, "%s\n", errline);
    for (int i = 0; i < pos.col - 1; i += 1)
        fprintf(stderr, " ");
    fprintf(stderr, "^\n");
    free(errline);
    if(p->err) {
        fprintf(stderr, "%s\n", p->err);
    }

    clean_ast(a);
    clean_parser(p);
    exit(0);
}