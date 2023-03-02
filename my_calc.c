#include "my_parser.h"
#include "my_calc.h"
#include "builtins.h"
#include "errors.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <criterion/logging.h>
#include <math.h>

// START GRAMMAR

// LANG <- ALLBLOCKS* EOF
int readlang(struct parser *p, struct ast *a);

// ALLBLOCKS <- (COMMENT / FUNC / BLOCK / EXPR)
int readallblocks(struct parser *p, struct ast *a);

// FUNCCALL <- VAR"()"
int readfunccall(struct parser *p, struct ast *a);

// FUNCDEF <- 'funk ' VAR'(' (ARGUMENT (',' ARGUMENT)*)? ')' '{' (ALLBLOCKS)* '}' ';'?
int readfuncdef(struct parser *p, struct ast *a);

// BLOCK <- (IFBLOCK / WHILEBLOCK)
int readblock(struct parser *p, struct ast *a);

// IFELSEBLOCK <- IFBLOCK ELSEBLOCK? ';'?
int readifelseblock(struct parser *p, struct ast *a);

// IFBLOCK <- OPIF '(' COND (OPORAND COND)* ')' '{' (ALLBLOCKS)* '}'
int readifblock(struct parser *p, struct ast *a);

// ELIFBLOCK <- OPELIF '(' COND (OPORAND COND)* ')' '{' (ALLBLOCKS)* '}'
int readelifblock(struct parser *p, struct ast *a);

// ELSEBLOCK <- OPELSE '{' (ALLBLOCKS)* '}'
int readelseblock(struct parser *p, struct ast *a);

// WHILEBLOCK <- OPWHILE '(' COND (OPORAND COND)* ')' '{' (ALLBLOCKS)* '}' ';'?
int readwhileblock(struct parser *p, struct ast *a);

// EXPR <- (VAR OPEQ)? (FUNCCALL / CALC) ';'
int readexpr(struct parser *p, struct ast *a);

// CALC <- COMP (OPLOGIC COMP)*
int readcalc(struct parser *p, struct ast *a);

// COMP <- ADD (OPCOMP ADD)*
int readcomp(struct parser *p, struct ast *a);

// ADD <- MUL (OPADD MUL)*
int readadd(struct parser *p, struct ast *a);

// MUL <- POW (OPMUL POW)*
int readmul(struct parser *p, struct ast *a);

// POW <- PAR (OPEXP PAR)*
int readpow(struct parser *p, struct ast *a);

// PAR <- OPUNA (INT / VAR / '(' CALC ')')
int readpar(struct parser *p, struct ast *a);

// OPIF <- "if"
int readopif(struct parser *p, struct ast *a);

// OPELIF <- "elif"
int readopelif(struct parser *p, struct ast *a);

// OPELSE <- "else"
int readopelse(struct parser *p, struct ast *a);

// OPWHILE <- "while"
int readopwhile(struct parser *p, struct ast *a);

// OPBREAK <- "break"
int readopbreak(struct parser *p, struct ast *a);

// OPLOGIC <- ("||" / "&&")
int readoplogic(struct parser *p, struct ast *a);

// OPCOMP <- ("==" / "!=" / "<=" / '<' / ">=" / '>' )
int readopcomp(struct parser *p, struct ast *a);

// OPADD <- ('+' / '-')
int readopadd(struct parser *p, struct ast *a);

// OPMUL <- ('*' / '/' / '%' )
int readopmul(struct parser *p, struct ast *a);

// OPPOW <- '^'
int readoppow(struct parser *p, struct ast *a);

// OPUNA <- ('+' / '-' / '!')
int readopuna(struct parser *p, struct ast *a);

// OPEQ <- '='
int readopeq(struct parser *p);

// TYPE <- ("int" / "str")
int readtype(struct parser *p);

// VAR <- [a-zA-Z_][a-zA-Z_0-9]*
int readvar(struct parser *p);

// INT <- [0-9]+
int readint(struct parser *p);

// COMMENT <- "//" ".*

// END GRAMMAR

int clean_ast(struct ast *ast)
{
    if (ast->type == _intvar || ast->type == _call || ast->type == _funcdef)
    {
        free(ast->val.strval);
    }

    for (int i = 0; i < ast->size; i++)
    {
        clean_ast(ast->edges[i]);
        free(ast->edges[i]);
    }

    free(ast->edges);

    return 1;
}

int remove_last(struct ast *ast)
{
    if (!ast->size)
        return 0;

    clean_ast(ast->edges[ast->size - 1]);
    free(ast->edges[ast->size - 1]);
    ast->size -= 1;

    return 1;
}

struct ast *append_or_reuse_ast(struct ast *ast)
{
    if (!ast->type)
    {
        return ast;
    }
    else
    {
        struct ast *sub_ast = calloc(1, sizeof(struct ast));

        void *ptr = reallocarray(ast->edges, ast->size + 1, sizeof(struct ast *));

        if (!ptr)
        {
            clean_ast(sub_ast);
            free(sub_ast);
            return NULL;
        }

        ast->edges = (struct ast **)ptr;

        ast->edges[ast->size] = sub_ast;
        ast->size += 1;

        return sub_ast;
    }
}

struct ast *prepend_or_reuse_ast(struct ast *ast)
{
    if (!ast->type)
    {
        return ast;
    }
    else
    {
        struct ast *sub_ast = calloc(1, sizeof(struct ast));
        sub_ast->val = ast->val;
        sub_ast->type = ast->type;
        sub_ast->size = ast->size;
        sub_ast->edges = ast->edges;
        ast->edges = NULL;
        ast->type = 0;
        ast->size = 0;

        void *ptr = reallocarray(ast->edges, ast->size + 1, sizeof(struct ast *));

        if (!ptr)
            return NULL;
        ast->edges = (struct ast **)ptr;

        ast->edges[ast->size] = sub_ast;
        ast->size += 1;

        return ast;
    }
}

void clean_space(struct parser *p)
{
    while (readset(p, " \n\t"))
        ;
}

int readcomment(struct parser *p)
{
    int ret = 0;

    clean_space(p);
    if (readtext(p, "//"))
    {
        readuntil(p, '\n');
        ret = 1;
    }

    return ret;
}

int readvar(struct parser *p)
{
    int ret = 0;

    clean_space(p);
    begin_capture(p, "VAR");
    if (readid(p))
        ret = 1;
    end_capture(p, "VAR");

    return ret;
}

int readtype(struct parser *p)
{
    int ret = 0;

    clean_space(p);
    begin_capture(p, "TYPE");
    if (readtext(p, "int"))
        ret = 1;
    end_capture(p, "TYPE");

    return ret;
}

int readopeq(struct parser *p)
{
    int ret = 0;

    clean_space(p);
    begin_capture(p, "OPEQ");
    if (readchar(p, '='))
        ret = 1;
    end_capture(p, "OPEQ");

    return ret;
}

int readopuna(struct parser *p, struct ast *ast)
{
    int ret = 0;

    clean_space(p);
    begin_capture(p, "OPUNA");
    if (readchar(p, '+') || readchar(p, '-') || readchar(p, '!'))
        ret = 1;
    end_capture(p, "OPUNA");

    if (ret)
    {
        char *op = get_value(p, "OPUNA");
        struct ast *sub_ast = prepend_or_reuse_ast(ast);
        sub_ast->type = _opuna;

        if (op[0] == '+')
            sub_ast->val.strval = "+";
        else if (op[0] == '-')
            sub_ast->val.strval = "-";
        else if (op[0] == '!')
            sub_ast->val.strval = "!";

        free(op);
    }

    return ret;
}

int readoppow(struct parser *p, struct ast *ast)
{
    int ret = 0;

    clean_space(p);
    begin_capture(p, "OPPOW");
    if (readchar(p, '^'))
        ret = 1;
    end_capture(p, "OPPOW");

    if (ret)
    {
        char *op = get_value(p, "OPPOW");
        struct ast *sub_ast = prepend_or_reuse_ast(ast);
        sub_ast->type = _opmath;

        if (op[0] == '^')
            sub_ast->val.strval = "^";

        free(op);
    }

    return ret;
}

int readopmul(struct parser *p, struct ast *ast)
{
    int ret = 0;

    clean_space(p);
    begin_capture(p, "OPMUL");
    if (readchar(p, '*') || (readchar(p, '/') || readchar(p, '%')))
        ret = 1;
    end_capture(p, "OPMUL");

    if (ret)
    {
        char *op = get_value(p, "OPMUL");
        struct ast *sub_ast = prepend_or_reuse_ast(ast);
        sub_ast->type = _opmath;

        if (op[0] == '*')
            sub_ast->val.strval = "*";
        else if (op[0] == '/')
            sub_ast->val.strval = "/";
        else if (op[0] == '%')
            sub_ast->val.strval = "%%";

        free(op);
    }

    return ret;
}

int readopadd(struct parser *p, struct ast *ast)
{
    int ret = 0;

    clean_space(p);
    begin_capture(p, "OPADD");
    if (readchar(p, '+') || (readchar(p, '-')))
        ret = 1;
    end_capture(p, "OPADD");

    if (ret)
    {
        char *op = get_value(p, "OPADD");
        struct ast *sub_ast = prepend_or_reuse_ast(ast);
        sub_ast->type = _opmath;

        if (op[0] == '+')
            sub_ast->val.strval = "+";
        else if (op[0] == '-')
            sub_ast->val.strval = "-";

        free(op);
    }

    return ret;
}

int readopcomp(struct parser *p, struct ast *ast)
{
    int ret = 0;

    clean_space(p);
    begin_capture(p, "OPCOMP");
    if (readtext(p, "==") || readtext(p, "!=") || readtext(p, "<=") || readchar(p, '<') || readtext(p, ">=") || readchar(p, '>'))
        ret = 1;
    end_capture(p, "OPCOMP");

    if (ret)
    {
        char *op = get_value(p, "OPCOMP");
        struct ast *sub_ast = prepend_or_reuse_ast(ast);
        sub_ast->type = _opcomp;

        if (!strcmp(op, "=="))
            sub_ast->val.strval = "==";
        else if (!strcmp(op, "!="))
            sub_ast->val.strval = "!=";
        else if (!strcmp(op, "<="))
            sub_ast->val.strval = "<=";
        else if (!strcmp(op, "<"))
            sub_ast->val.strval = "<";
        else if (!strcmp(op, ">="))
            sub_ast->val.strval = ">=";
        else if (!strcmp(op, ">"))
            sub_ast->val.strval = ">";

        free(op);
    }

    return ret;
}

int readoplogic(struct parser *p, struct ast *ast)
{
    int ret = 0;

    clean_space(p);
    begin_capture(p, "OPLOGIC");
    if (readtext(p, "||") || readtext(p, "&&"))
        ret = 1;
    end_capture(p, "OPLOGIC");

    if (ret)
    {
        char *op = get_value(p, "OPLOGIC");
        struct ast *sub_ast = prepend_or_reuse_ast(ast);
        sub_ast->type = _oplogic;

        if (!strcmp(op, "||"))
            sub_ast->val.strval = "||";
        else if (!strcmp(op, "&&"))
            sub_ast->val.strval = "&&";

        free(op);
    }

    return ret;
}

int readopbreak(struct parser *p, struct ast *ast)
{
    int ret = 0;

    clean_space(p);
    if (readtext(p, "break"))
        ret = 1;

    if (ret)
    {
        struct ast *sub_ast = prepend_or_reuse_ast(ast);
        sub_ast->type = _opcontrol;
        sub_ast->val.strval = "break";
    }

    return ret;
}

int readopwhile(struct parser *p, struct ast *ast)
{
    int ret = 0;

    clean_space(p);
    if (readtext(p, "while"))
        ret = 1;

    if (ret)
    {
        struct ast *sub_ast = prepend_or_reuse_ast(ast);
        sub_ast->type = _opblock;
        sub_ast->val.strval = "while";
    }

    return ret;
}

int readopelse(struct parser *p, struct ast *ast)
{
    int ret = 0;

    clean_space(p);
    if (readtext(p, "else"))
        ret = 1;

    if (ret)
    {
        struct ast *sub_ast = prepend_or_reuse_ast(ast);
        sub_ast->type = _opblock;
        sub_ast->val.strval = "else";
    }

    return ret;
}

int readopelif(struct parser *p, struct ast *ast)
{
    int ret = 0;

    clean_space(p);
    if (readtext(p, "elif"))
        ret = 1;

    if (ret)
    {
        struct ast *sub_ast = prepend_or_reuse_ast(ast);
        sub_ast->type = _opblock;
        sub_ast->val.strval = "elif";
    }

    return ret;
}

int readopif(struct parser *p, struct ast *ast)
{
    int ret = 0;

    clean_space(p);
    if (readtext(p, "if"))
        ret = 1;

    if (ret)
    {
        struct ast *sub_ast = prepend_or_reuse_ast(ast);
        sub_ast->type = _opblock;
        sub_ast->val.strval = "if";
    }

    return ret;
}

int readpar(struct parser *p, struct ast *ast)
{
    int ret = 0;

    struct ast *sub_ast = append_or_reuse_ast(ast);

    clean_space(p);
    while (readopuna(p, sub_ast))
        ;

    if (begin_capture(p, "INT") && readint(p) && end_capture(p, "INT"))
    {
        char *tmp = get_value(p, "INT");

        struct ast *int_ast = append_or_reuse_ast(sub_ast);
        int_ast->type = _const;
        int_ast->val.intval = atoi(tmp);

        free(tmp);
        ret = 1;
    }
    else if (readfunccall(p, sub_ast))
        ret = 1;
    else if (readvar(p))
    {
        char *tmp = get_value(p, "VAR");

        sub_ast = append_or_reuse_ast(sub_ast);
        sub_ast->type = _intvar;
        sub_ast->val.strval = tmp;

        ret = 1;
    }
    else if (readchar(p, '(') && (sub_ast = append_or_reuse_ast(sub_ast)) && readcalc(p, sub_ast) && readchar(p, ')'))
    {
        ret = 1;
    }

    if (!ret && (sub_ast != ast))
        remove_last(ast);

    return ret;
}

int readpow(struct parser *p, struct ast *ast)
{
    int ret = 0;

    struct ast *sub_ast = append_or_reuse_ast(ast);

    if (readpar(p, sub_ast))
    {
        while (readoppow(p, sub_ast) && readpar(p, sub_ast))
            ;
        ret = 1;
    }

    if (!ret && (sub_ast != ast))
        remove_last(ast);

    return ret;
}

int readmul(struct parser *p, struct ast *ast)
{
    int ret = 0;

    struct ast *sub_ast = append_or_reuse_ast(ast);

    if (readpow(p, sub_ast))
    {
        while (readopmul(p, sub_ast) && readpow(p, sub_ast))
            ;
        ret = 1;
    }

    if (!ret && (sub_ast != ast))
        remove_last(ast);

    return ret;
}

int readadd(struct parser *p, struct ast *ast)
{
    int ret = 0;

    struct ast *sub_ast = append_or_reuse_ast(ast);

    if (readmul(p, sub_ast))
    {
        while (readopadd(p, sub_ast) && readmul(p, sub_ast))
            ;
        ret = 1;
    }

    if (!ret && (sub_ast != ast))
        remove_last(ast);

    return ret;
}

int readcomp(struct parser *p, struct ast *ast)
{
    int ret = 0;

    struct ast *sub_ast = append_or_reuse_ast(ast);

    if (readadd(p, sub_ast))
    {
        while (readopcomp(p, sub_ast) && readadd(p, sub_ast))
            ;
        ret = 1;
    }

    if (!ret && (sub_ast != ast))
        remove_last(ast);

    return ret;
}

int readcalc(struct parser *p, struct ast *ast)
{
    int ret = 0;

    struct ast *sub_ast = append_or_reuse_ast(ast);

    if (readcomp(p, sub_ast))
    {
        while (readoplogic(p, sub_ast) && readcomp(p, sub_ast))
            ;
        ret = 1;
    }

    if (!ret && (sub_ast != ast))
        remove_last(ast);

    return ret;
}

int readexpr(struct parser *p, struct ast *ast)
{
    int ret = 0;

    struct ast *sub_ast = append_or_reuse_ast(ast);

    int last_pos = p->current_pos;
    readtype(p);
    if (readvar(p))
    {
        if (readopeq(p))
        {
            char *var = get_value(p, "VAR");
            struct ast *var_ast = append_or_reuse_ast(sub_ast);
            var_ast->type = _intvar;
            var_ast->val.strval = var;

            struct ast *eq_ast = prepend_or_reuse_ast(var_ast);
            eq_ast->type = _opeq;
            eq_ast->val.strval = "=";
        }
        else
        {
            p->current_pos = last_pos;
        }
    }

    if (readcalc(p, sub_ast))
    {
        if (readchar(p, ';'))
        {
            ret = 1;
        }
    }

    if (!ret && (sub_ast != ast))
        remove_last(ast);

    return ret;
}

int readwhileblock(struct parser *p, struct ast *ast)
{
    int ret = 0;

    int tmp_pos = p->current_pos;
    struct ast *sub_ast = append_or_reuse_ast(ast);
    if (readopwhile(p, sub_ast))
    {

        struct ast *whilecond_ast = append_or_reuse_ast(sub_ast);

        clean_space(p);
        if (readchar(p, '('))
        {
            if (readcalc(p, whilecond_ast))
            {
                clean_space(p);
                if (readchar(p, ')'))
                {
                    clean_space(p);
                    if (readchar(p, '{'))
                    {
                        struct ast *whileblock_ast = append_or_reuse_ast(sub_ast);
                        whileblock_ast->type = _compound;

                        int tmp_pos = p->current_pos;
                        while (readallblocks(p, whileblock_ast))
                        {
                            tmp_pos = p->current_pos;
                        };
                        p->current_pos = tmp_pos;

                        clean_space(p);
                        if (readchar(p, '}'))
                        {
                            readchar(p, ';');
                            ret = 1;
                        }
                        else
                            p->err = "Missing 'while' closing bracket '}'";
                    }
                    else
                        p->err = "Missing 'while' opening bracket '{'";
                }
                else
                    p->err = "Missing 'while' closing parenthesis ')'";
            }
        }
        else
            p->err = "Missing 'while' opening parenthesis '('";
    }

    if (!ret)
    {
        remove_last(ast);
        p->current_pos = tmp_pos;
    }

    return ret;
}

int readelseblock(struct parser *p, struct ast *ast)
{
    int ret = 0;

    int tmp_pos = p->current_pos;
    struct ast *sub_ast = append_or_reuse_ast(ast);
    if (readopelse(p, sub_ast))
    {

        clean_space(p);
        if (readchar(p, '{'))
        {
            struct ast *elseblock_ast = append_or_reuse_ast(sub_ast);
            elseblock_ast->type = _compound;

            int tmp_pos = p->current_pos;
            while (readallblocks(p, elseblock_ast))
            {
                tmp_pos = p->current_pos;
            };
            p->current_pos = tmp_pos;

            clean_space(p);
            if (readchar(p, '}'))
            {
                ret = 1;
            }
            else
                p->err = "Missing 'else' closing bracket '}'";
        }
        else
            p->err = "Missing 'else' opening bracket '{'";
    }

    if (!ret)
    {
        remove_last(ast);
        p->current_pos = tmp_pos;
    }

    return ret;
}

int readelifblock(struct parser *p, struct ast *ast)
{
    int ret = 0;

    int tmp_pos = p->current_pos;
    struct ast *sub_ast = append_or_reuse_ast(ast);
    if (readopelif(p, sub_ast))
    {
        struct ast *elifcond_ast = append_or_reuse_ast(sub_ast);

        clean_space(p);
        if (readchar(p, '('))
        {
            if (readcalc(p, elifcond_ast))
            {
                clean_space(p);
                if (readchar(p, ')'))
                {

                    clean_space(p);
                    if (readchar(p, '{'))
                    {
                        struct ast *elifblock_ast = append_or_reuse_ast(sub_ast);
                        elifblock_ast->type = _compound;

                        int tmp_pos = p->current_pos;
                        while (readallblocks(p, elifblock_ast))
                        {
                            tmp_pos = p->current_pos;
                        };
                        p->current_pos = tmp_pos;

                        clean_space(p);
                        if (readchar(p, '}'))
                        {
                            ret = 1;
                        }
                        else
                            p->err = "Missing 'elif' closing bracket '}'";
                    }
                    else
                        p->err = "Missing 'elif' opening bracket '{'";
                }
                else
                    p->err = "Missing 'elif' closing parenthesis ')'";
            }
        }
        else
            p->err = "Missing 'elif' opening parenthesis '('";
    }

    if (!ret)
    {
        remove_last(ast);
        p->current_pos = tmp_pos;
    }

    return ret;
}

int readifblock(struct parser *p, struct ast *ast)
{
    int ret = 0;

    int tmp_pos = p->current_pos;
    struct ast *sub_ast = append_or_reuse_ast(ast);
    if (readopif(p, sub_ast))
    {

        struct ast *ifcond_ast = append_or_reuse_ast(sub_ast);

        clean_space(p);
        if (readchar(p, '('))
        {
            if (readcalc(p, ifcond_ast))
            {
                clean_space(p);
                if (readchar(p, ')'))
                {

                    clean_space(p);
                    if (readchar(p, '{'))
                    {
                        struct ast *ifblock_ast = append_or_reuse_ast(sub_ast);
                        ifblock_ast->type = _compound;

                        int tmp_pos = p->current_pos;
                        while (readallblocks(p, ifblock_ast))
                        {
                            tmp_pos = p->current_pos;
                        };
                        p->current_pos = tmp_pos;

                        clean_space(p);
                        if (readchar(p, '}'))
                        {
                            ret = 1;
                        }
                        else
                            p->err = "Missing 'if' closing bracket '}'";
                    }
                    else
                        p->err = "Missing 'if' opening bracket '{'";
                }
                else
                    p->err = "Missing 'if' closing parenthesis ')'";
            }
        }
        else
            p->err = "Missing 'if' opening parenthesis '('";
    }

    if (!ret)
    {
        remove_last(ast);
        p->current_pos = tmp_pos;
    }

    return ret;
}

int readifelseblock(struct parser *p, struct ast *ast)
{
    int ret = 0;

    int tmp_pos = p->current_pos;
    struct ast *sub_ast = append_or_reuse_ast(ast);
    sub_ast->type = _opblock;
    sub_ast->val.strval = "ifelse";

    if (readifblock(p, sub_ast))
    {
        while (readcomment(p))
            ;
        while (readelifblock(p, sub_ast))
            ;
        while (readcomment(p))
            ;
        readelseblock(p, sub_ast);
        ret = 1;
    }

    clean_space(p);
    readchar(p, ';');

    if (!ret)
    {
        sub_ast->type = _;
        sub_ast->val.strval = 0;
        if (sub_ast != ast)
        {
            remove_last(ast);
        }
        p->current_pos = tmp_pos;
    }

    return ret;
}

int readblock(struct parser *p, struct ast *a)
{
    int ret = 0;

    if (readifelseblock(p, a) || readwhileblock(p, a))
        ret = 1;

    return ret;
}

int readfuncdef(struct parser *p, struct ast *ast)
{
    int ret = 0;

    struct ast *sub_ast = append_or_reuse_ast(ast);

    int tmp_pos = p->current_pos;
    if (readtext(p, "funk "))
    {
        clean_space(p);
        if (readvar(p))
        {
            if (readchar(p, '('))
            {
                sub_ast->type = _funcdef;
                sub_ast->val.strval = get_value(p, "VAR");

                struct ast *args_ast = append_or_reuse_ast(sub_ast);
                args_ast->type = _args;

                clean_space(p);
                readtype(p);
                while (readvar(p))
                {
                    char *var = get_value(p, "VAR");
                    struct ast *var_ast = append_or_reuse_ast(args_ast);
                    var_ast->type = _intvar;
                    var_ast->val.strval = var;

                    clean_space(p);
                    if (!readchar(p, ','))
                        break;
                    readtype(p);
                }

                clean_space(p);
                if (readchar(p, ')'))
                {
                    clean_space(p);
                    if (readchar(p, '{'))
                    {
                        struct ast *func_ast = append_or_reuse_ast(sub_ast);
                        func_ast->type = _compound;

                        int tmp_pos = p->current_pos;
                        while (readallblocks(p, func_ast))
                        {
                            tmp_pos = p->current_pos;
                        };
                        p->current_pos = tmp_pos;

                        clean_space(p);
                        if (readchar(p, '}'))
                        {
                            readchar(p, ';');
                            ret = 1;
                        }
                    }
                }
            }
        }
    }

    if (!ret)
    {
        remove_last(ast);
        p->current_pos = tmp_pos;
    }

    return ret;
}

int readfunccall(struct parser *p, struct ast *ast)
{
    int ret = 0;

    struct ast *sub_ast = append_or_reuse_ast(ast);

    int tmp_pos = p->current_pos;
    if (readvar(p) && readchar(p, '('))
    {
        sub_ast->type = _call;
        sub_ast->val.strval = get_value(p, "VAR");

        clean_space(p);
        while (readcalc(p, sub_ast))
        {
            clean_space(p);
            if (!readchar(p, ','))
                break;
        }

        if (readchar(p, ')'))
            ret = 1;
        else
            p->err = "Missing function call closing parenthesis ')'";
    }

    if (!ret)
    {
        remove_last(ast);
        p->current_pos = tmp_pos;
    }

    return ret;
}

int readallblocks(struct parser *p, struct ast *ast)
{
    int ret = 0;

    if (readcomment(p) || readfuncdef(p, ast) || readblock(p, ast) || readexpr(p, ast))
        ret = 1;

    return ret;
}

int readlang(struct parser *p, struct ast *ast)
{
    int ret = 0;

    ast->type = _compound;

    while (readallblocks(p, ast))
        ;

    int tmp_pos = p->current_pos;
    int last_pos = p->last_pos;

    p->current_pos = tmp_pos;
    clean_space(p);
    p->last_pos = last_pos;

    if (!readeof(p))
    {
        safe_exit(p, ast);
    }
    else
        ret = 1;

    p->current_pos = 0;
    return ret;
}

int my_calc(struct parser *p, struct ast *ast)
{
    return readlang(p, ast);
}

struct def_list *getdef(struct scope *s, char *name)
{
    if (s->defs == NULL)
        return NULL;

    struct def_list *ptr = s->defs;
    while (ptr)
    {
        if (!strcmp(ptr->name, name))
            return ptr;
        ptr = ptr->next;
    }

    return NULL;
}

int create_or_reuse_dl(struct ast *a, struct scope *s, union Definition v)
{
    struct def_list *ptr = getdef(s, a->val.strval);

    if (ptr && !ptr->builtin)
    {
        ptr->val = v;
    }
    else if (!ptr)
    {
        struct def_list *dl = calloc(1, sizeof(struct def_list));
        char *name = calloc(1, sizeof(char) * (strlen(a->val.strval) + 1));
        strcpy(name, a->val.strval);
        dl->name = name;
        dl->val = v;
        dl->type = __;

        dl->next = s->defs;
        s->defs = dl;
    }

    return 1;
}

int check_cond(long a, long b, char *opc)
{
    if (!strcmp(opc, "=="))
    {
        if (a == b)
            return 1;
    }
    else if (!strcmp(opc, "!="))
    {
        if (a != b)
            return 1;
    }
    else if (!strcmp(opc, "<="))
    {
        if (a <= b)
            return 1;
    }
    else if (!strcmp(opc, "<"))
    {
        if (a < b)
            return 1;
    }
    else if (!strcmp(opc, ">="))
    {
        if (a >= b)
            return 1;
    }
    else if (!strcmp(opc, ">"))
    {
        if (a > b)
            return 1;
    }

    return 0;
}

struct scope *duplicate_scope(struct scope *s)
{
    struct scope *new_scope = calloc(1, sizeof(struct scope));

    struct def_list *ptr = s->defs;
    if (ptr != NULL)
    {
        struct def_list *last = NULL;
        while (ptr)
        {
            struct def_list *tmp = ptr->next;

            struct def_list *dl = calloc(1, sizeof(struct def_list));

            if (new_scope->defs == NULL)
                new_scope->defs = dl;

            char *name = calloc(1, sizeof(char) * (strlen(ptr->name) + 1));
            strcpy(name, ptr->name);
            dl->name = name;

            dl->val = ptr->val;
            dl->builtin = ptr->builtin;
            dl->type = ptr->type;

            if (last)
                last->next = dl;
            else
                dl->next = NULL;

            last = dl;

            ptr = tmp;
        }
    }

    return new_scope;
}

void clean_scope(struct scope *s)
{
    struct def_list *ptr = s->defs;
    if (ptr != NULL)
    {
        while (ptr)
        {
            struct def_list *tmp = ptr->next;

            if (ptr->name)
            {
                free(ptr->name);
            }
            free(ptr);
            ptr = tmp;
        }
    }
}

int recursive_eval(struct ast *ast, struct scope *s)
{
    if (ast == NULL)
        return 0;

    if (!ast->size && ast->type == _const)
    {
        s->current_val = ast->val.intval;
        return 1;
    }

    if (!ast->size && ast->type == _intvar)
    {
        struct def_list *ptr;

        if ((ptr = getdef(s, ast->val.strval)))
        {
            s->current_val = ptr->val.intval;
            return 1;
        }

        return 0;
    }

    if (ast->type == _funcdef)
    {
        union Definition val;
        val.astptr = ast;
        return create_or_reuse_dl(ast, s, val);
    }
    else if (ast->type == _call)
    {
        int ret = 0;
        struct def_list *ptr;

        if ((ptr = getdef(s, ast->val.strval)))
        {
            int i;
            int *args_res = calloc(0, sizeof(int));
            for (i = 0; i < ast->size; i++)
            {
                args_res = (int *)realloc(args_res, sizeof(int) * (i + 1));
                recursive_eval(ast->edges[i], s);
                args_res[i] = s->current_val;
            }

            if (ptr->builtin)
            {
                if (!strcmp(ptr->name, "println") && ast->size == 1)
                {
                    ret = _println(args_res[0]);
                }
                else if (!strcmp(ptr->name, "print") && ast->size == 1)
                {
                    ret = _print(args_res[0]);
                }
            }
            else
            {
                struct ast *func_ast = ptr->val.astptr;

                if (func_ast->size > 1)
                {
                    if (func_ast->edges[0]->size == ast->size)
                    {

                        struct scope *func_scope = duplicate_scope(s);

                        for (i = 0; i < func_ast->edges[0]->size; i++)
                        {
                            union Definition val;
                            val.intval = args_res[i];
                            create_or_reuse_dl(func_ast->edges[0]->edges[i], func_scope, val);
                        }

                        ret = recursive_eval(func_ast->edges[1], func_scope);

                        struct def_list *ptr = func_scope->defs;
                        if (ptr != NULL)
                        {
                            while (ptr)
                            {
                                struct def_list *tmp = ptr->next;

                                int arg = 0;
                                for (i = 0; i < func_ast->edges[0]->size; i++)
                                {
                                    if (!strcmp(func_ast->edges[0]->edges[i]->val.strval, ptr->name))
                                    {
                                        arg = 1;
                                    }
                                }

                                if (!arg)
                                {
                                    struct def_list *og;
                                    if ((og = getdef(s, ptr->name)))
                                    {
                                        og->val = ptr->val;
                                    }
                                }

                                ptr = tmp;
                            }
                        }

                        s->current_val = func_scope->current_val;
                        clean_scope(func_scope);
                        free(func_scope);
                    }
                }
            }

            free(args_res);
        }

        return ret;
    }

    if (ast->type == _opblock)
    {
        if (!strcmp(ast->val.strval, "ifelse") && ast->size > 0)
        {
            int ret = 0;

            int i;
            for (i = 0; i < ast->size; i++)
            {
                ret = recursive_eval(ast->edges[i], s);
                if (ret == 1)
                    break;
            }

            return ret;
        }
        else if ((!strcmp(ast->val.strval, "if") || !strcmp(ast->val.strval, "elif")) && ast->size > 1)
        {
            int ret = 0;

            recursive_eval(ast->edges[0], s);
            if (s->current_val)
                ret = recursive_eval(ast->edges[1], s);

            return ret;
        }
        else if (!strcmp(ast->val.strval, "else") && ast->size > 0)
        {
            return recursive_eval(ast->edges[0], s);
        }
        else if (!strcmp(ast->val.strval, "while") && ast->size > 1)
        {
            int ret = 0;

            recursive_eval(ast->edges[0], s);
            if (s->current_val)
            {
                while (true)
                {
                    ret = recursive_eval(ast->edges[1], s);

                    if (ret == 0)
                        break;

                    recursive_eval(ast->edges[0], s);
                    if (!s->current_val)
                        break;
                }
            }

            return ret;
        }
    }

    if (ast->type == _opuna)
    {
        int ret;

        ret = recursive_eval(ast->edges[0], s);
        int c = s->current_val;

        if (ret == 0)
            return ret;

        switch (ast->val.strval[0])
        {
        case '+':
            return 1;
        case '-':
            s->current_val = c * -1;
            return 1;
        case '!':
            if (c == 0)
            {
                s->current_val = 1;
            }
            else
            {
                s->current_val = 0;
            }
            return 1;
        }

        return ret;
    }

    if (ast->type == _opeq)
    {
        int ret;

        ret = recursive_eval(ast->edges[1], s);
        int r = s->current_val;

        if (!ret)
            return 0;

        union Definition val;
        val.intval = r;    
        ret = create_or_reuse_dl(ast->edges[0], s, val);

        return ret;
    }

    if (ast->type == _oplogic || ast->type == _opcomp || ast->type == _opmath)
    {
        int ret;

        ret = recursive_eval(ast->edges[0], s);
        int l = s->current_val;
        ret = recursive_eval(ast->edges[1], s);
        int r = s->current_val;

        if (ret == 0)
            return ret;

        if (ast->type == _oplogic)
        {
            if (!strcmp(ast->val.strval, "&&"))
            {
                if (l && r)
                    s->current_val = 1;
                else
                    s->current_val = 0;

                return 1;
            }
            else if (!strcmp(ast->val.strval, "||"))
            {
                if (l || r)
                    s->current_val = 1;
                else
                    s->current_val = 0;

                return 1;
            }
        }
        else if (ast->type == _opcomp)
        {
            s->current_val = check_cond(l, r, ast->val.strval);
            return 1;
        }
        else if (ast->type == _opmath)
        {
            switch (ast->val.strval[0])
            {
            case '+':
                s->current_val = l + r;
                return 1;
            case '-':
                s->current_val = l - r;
                return 1;
            case '*':
                s->current_val = l * r;
                return 1;
            case '/':
                s->current_val = l / r;
                return 1;
            case '%':
                s->current_val = l % r;
                return 1;
            case '^':
                s->current_val = (int)pow(l, r);
                return 1;
            default:
                return 0;
            }
        }
    }

    int ret = 0;
    if (ast->type == _compound)
    {
        int i;
        for (i = 0; i < ast->size; i++)
        {
            ret = recursive_eval(ast->edges[i], s);
        }
    }

    return ret;
}

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

int eval(struct ast *a, struct scope *s)
{
    int ret;

    s->defs = 0;
    register_builtins(s);
    ret = recursive_eval(a, s);
    clean_scope(s);
    return ret;
}
