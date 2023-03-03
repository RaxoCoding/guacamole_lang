#ifndef _BUILTINS_H
#define _BUILTINS_H
#include "my_calc.h"

// Register all built-ins to scope
void register_builtins(struct scope *s);

// Print with 2 surrounding spaces
int _print(int val);

// Print with new lines
int _println(int val);

// DONUT
int _donut();

#endif /* _BUILTINS_H */
