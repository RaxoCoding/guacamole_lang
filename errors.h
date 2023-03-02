#ifndef _ERRORS_H
#define _ERRORS_H
#include "my_parser.h"
#include "my_calc.h"

// Exit safely and print err message
void safe_exit(struct parser *p, struct ast *a);

#endif /* _ERRORS_H */
