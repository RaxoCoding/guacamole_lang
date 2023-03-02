#ifndef _MY_CALC_H
#define _MY_CALC_H
#include "my_parser.h"

union Definition
{
    int intval;
    struct ast *astptr;
};

struct def_list
{
    char *name;
    union Definition val;
    int builtin;
    struct def_list *next;
};

struct scope
{
    struct def_list *defs;
    long int current_val;
};

union Constant
{
    int intval;
    char *strval;
};

struct ast
{
    enum
    {
        _,
        _compound,
        _const,
        _var,
        _func,
        _args,
        _call,
        _opeq,
        _opmath,
        _opuna,
        _oplogic,
        _opcomp,
        _opblock,
        _opstack,
    } type;
    union Constant val;
    int size;
    struct ast **edges;
};

int my_calc(struct parser *p, struct ast *a);
int clean_ast(struct ast *ast);
int eval(struct ast *a, struct scope *s);

#endif /* _MY_CALC_H */
