#ifndef _MY_CALC_H
#define _MY_CALC_H
#include "my_parser.h"

union Definition
{
    int intval;
    struct ast *astptr;
};

typedef enum
{
    __,
    _int,
    _func,
} dltype;

struct def_list
{
    char *name;
    union Definition val;
    dltype type;
    int builtin;
    struct def_list *next;
};

struct scope
{
    struct def_list *defs;
    long int current_val;
};

struct error_scope
{
    int begin;
    int end;
    char *err;
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
        _args,
        _funccall,
        _opeq,
        _const,
        _opuna,
        _var,
        _opmath,
        _opcomp,
        _funcdef,
        _block,
        _loop,
        _oplogic,
        _compound,
        _opcontrol,
    } type;
    union Constant val;
    int size;
    struct ast **edges;
    int begin;
    int end;
};

int my_calc(struct parser *p, struct ast *a, struct error_scope *err_s);
int clean_ast(struct ast *ast);
int eval(struct ast *a, struct scope *s);

#endif /* _MY_CALC_H */
