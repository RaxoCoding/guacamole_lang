#include "my_parser.h"
#include "my_calc.h"
#include "builtins.h"
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

// CONTROL <- (OPBREAK / (OPRETURN CALC)) ';'
int readcontrol(struct parser *p, struct ast *a);

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

// OPCONTINUE <- "continue"
int readopcontinue(struct parser *p, struct ast *a);

// OPRETURN <- "return"
int readopreturn(struct parser *p, struct ast *a);

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

// VAR <- [a-zA-Z_][a-zA-Z_0-9]*
int readvar(struct parser *p);

// INT <- [0-9]+
int readint(struct parser *p);

// COMMENT <- "//" ".*

// END GRAMMAR

int clean_ast(struct ast *ast)
{
    if (ast->type == _var || ast->type == _funccall || ast->type == _funcdef)
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

struct ast *append_or_reuse_ast(struct ast *ast, struct parser *p)
{
    if (!ast->type)
    {
        ast->begin = p->current_pos;
        return ast;
    }
    else
    {
        struct ast *sub_ast = calloc(1, sizeof(struct ast));
        sub_ast->begin = p->current_pos;

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

struct ast *prepend_or_reuse_ast(struct ast *ast, struct parser *p)
{
    if (!ast->type)
    {
        ast->begin = p->current_pos;
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
        ast->begin = p->current_pos;

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
    int begin = p->current_pos;
    if (readchar(p, '+') || readchar(p, '-') || readchar(p, '!'))
        ret = 1;
    end_capture(p, "OPUNA");

    if (ret)
    {
        char *op = get_value(p, "OPUNA");
        struct ast *sub_ast = prepend_or_reuse_ast(ast, p);
        sub_ast->type = _opuna;
        sub_ast->begin = begin;
        sub_ast->end = p->current_pos;

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
    int begin = p->current_pos;
    if (readchar(p, '^'))
        ret = 1;
    end_capture(p, "OPPOW");

    if (ret)
    {
        char *op = get_value(p, "OPPOW");
        struct ast *sub_ast = prepend_or_reuse_ast(ast, p);
        sub_ast->type = _opmath;
        sub_ast->begin = begin;
        sub_ast->end = p->current_pos;

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
    int begin = p->current_pos;
    if (readchar(p, '*') || (readchar(p, '/') || readchar(p, '%')))
        ret = 1;
    end_capture(p, "OPMUL");

    if (ret)
    {
        char *op = get_value(p, "OPMUL");
        struct ast *sub_ast = prepend_or_reuse_ast(ast, p);
        sub_ast->type = _opmath;
        sub_ast->begin = begin;
        sub_ast->end = p->current_pos;

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
    int begin = p->current_pos;
    if (readchar(p, '+') || (readchar(p, '-')))
        ret = 1;
    end_capture(p, "OPADD");

    if (ret)
    {
        char *op = get_value(p, "OPADD");
        struct ast *sub_ast = prepend_or_reuse_ast(ast, p);
        sub_ast->type = _opmath;
        sub_ast->begin = begin;
        sub_ast->end = p->current_pos;

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
    int begin = p->current_pos;
    if (readtext(p, "==") || readtext(p, "!=") || readtext(p, "<=") || readchar(p, '<') || readtext(p, ">=") || readchar(p, '>'))
        ret = 1;
    end_capture(p, "OPCOMP");

    if (ret)
    {
        char *op = get_value(p, "OPCOMP");
        struct ast *sub_ast = prepend_or_reuse_ast(ast, p);
        sub_ast->type = _opcomp;
        sub_ast->begin = begin;
        sub_ast->end = p->current_pos;

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
    int begin = p->current_pos;
    if (readtext(p, "||") || readtext(p, "&&"))
        ret = 1;
    end_capture(p, "OPLOGIC");

    if (ret)
    {
        char *op = get_value(p, "OPLOGIC");
        struct ast *sub_ast = prepend_or_reuse_ast(ast, p);
        sub_ast->type = _oplogic;
        sub_ast->begin = begin;
        sub_ast->end = p->current_pos;

        if (!strcmp(op, "||"))
            sub_ast->val.strval = "||";
        else if (!strcmp(op, "&&"))
            sub_ast->val.strval = "&&";

        free(op);
    }

    return ret;
}

int readopreturn(struct parser *p, struct ast *ast)
{
    int ret = 0;

    clean_space(p);
    int begin = p->current_pos;
    if (readtext(p, "return"))
        ret = 1;

    if (ret)
    {
        struct ast *sub_ast = append_or_reuse_ast(ast, p);
        sub_ast->type = _opcontrol;
        sub_ast->val.strval = "return";
        sub_ast->begin = begin;
        sub_ast->end = p->current_pos;
    }

    return ret;
}

int readopcontinue(struct parser *p, struct ast *ast)
{
    int ret = 0;

    clean_space(p);
    int begin = p->current_pos;
    if (readtext(p, "continue"))
        ret = 1;

    if (ret)
    {
        struct ast *sub_ast = append_or_reuse_ast(ast, p);
        sub_ast->type = _opcontrol;
        sub_ast->val.strval = "continue";
        sub_ast->begin = begin;
        sub_ast->end = p->current_pos;
    }

    return ret;
}

int readopbreak(struct parser *p, struct ast *ast)
{
    int ret = 0;

    clean_space(p);
    int begin = p->current_pos;
    if (readtext(p, "break"))
        ret = 1;

    if (ret)
    {
        struct ast *sub_ast = append_or_reuse_ast(ast, p);
        sub_ast->type = _opcontrol;
        sub_ast->val.strval = "break";
        sub_ast->begin = begin;
        sub_ast->end = p->current_pos;
    }

    return ret;
}

int readopwhile(struct parser *p, struct ast *ast)
{
    int ret = 0;

    clean_space(p);
    int begin = p->current_pos;
    if (readtext(p, "while"))
        ret = 1;

    if (ret)
    {
        struct ast *sub_ast = prepend_or_reuse_ast(ast, p);
        sub_ast->type = _loop;
        sub_ast->val.strval = "while";
        sub_ast->begin = begin;
        sub_ast->end = p->current_pos;
    }

    return ret;
}

int readopelse(struct parser *p, struct ast *ast)
{
    int ret = 0;

    clean_space(p);
    int begin = p->current_pos;
    if (readtext(p, "else"))
        ret = 1;

    if (ret)
    {
        struct ast *sub_ast = prepend_or_reuse_ast(ast, p);
        sub_ast->type = _block;
        sub_ast->val.strval = "else";
        sub_ast->begin = begin;
        sub_ast->end = p->current_pos;
    }

    return ret;
}

int readopelif(struct parser *p, struct ast *ast)
{
    int ret = 0;

    clean_space(p);
    int begin = p->current_pos;
    if (readtext(p, "elif"))
        ret = 1;

    if (ret)
    {
        struct ast *sub_ast = prepend_or_reuse_ast(ast, p);
        sub_ast->type = _block;
        sub_ast->val.strval = "elif";
        sub_ast->begin = begin;
        sub_ast->end = p->current_pos;
    }

    return ret;
}

int readopif(struct parser *p, struct ast *ast)
{
    int ret = 0;

    clean_space(p);
    int begin = p->current_pos;
    if (readtext(p, "if"))
        ret = 1;

    if (ret)
    {
        struct ast *sub_ast = prepend_or_reuse_ast(ast, p);
        sub_ast->type = _block;
        sub_ast->val.strval = "if";
        sub_ast->begin = begin;
        sub_ast->end = p->current_pos;
    }

    return ret;
}

int readpar(struct parser *p, struct ast *ast)
{
    int ret = 0;

    struct ast *sub_ast = append_or_reuse_ast(ast, p);

    clean_space(p);
    while (readopuna(p, sub_ast))
        ;

    struct ast *par_ast = append_or_reuse_ast(sub_ast, p);
    if (begin_capture(p, "INT") && readint(p) && end_capture(p, "INT"))
    {
        char *tmp = get_value(p, "INT");

        par_ast->type = _const;
        par_ast->val.intval = atoi(tmp);
        par_ast->end = p->current_pos;

        free(tmp);
        ret = 1;
    }
    else if (readfunccall(p, par_ast))
        ret = 1;
    else if (readvar(p))
    {
        char *tmp = get_value(p, "VAR");

        if (!strcmp(tmp, "return") || !strcmp(tmp, "while") || !strcmp(tmp, "break") || !strcmp(tmp, "funk") || !strcmp(tmp, "if") || !strcmp(tmp, "elif") || !strcmp(tmp, "else"))
        {
            free(tmp);
        }
        else
        {
            par_ast->type = _var;
            par_ast->val.strval = tmp;
            par_ast->end = p->current_pos;

            ret = 1;
        }
    }
    else if (readchar(p, '(') && readcalc(p, par_ast) && readchar(p, ')'))
        ret = 1;

    if (!ret && (sub_ast != ast))
        remove_last(ast);

    return ret;
}

int readpow(struct parser *p, struct ast *ast)
{
    int ret = 0;

    struct ast *sub_ast = append_or_reuse_ast(ast, p);

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

    struct ast *sub_ast = append_or_reuse_ast(ast, p);

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

    struct ast *sub_ast = append_or_reuse_ast(ast, p);

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

    struct ast *sub_ast = append_or_reuse_ast(ast, p);

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

    struct ast *sub_ast = append_or_reuse_ast(ast, p);

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

    struct ast *sub_ast = append_or_reuse_ast(ast, p);

    int last_pos = p->current_pos;

    int var_begin = p->current_pos;
    if (readvar(p))
    {
        int eq_begin = p->current_pos;
        if (readopeq(p))
        {
            struct ast *var_ast = append_or_reuse_ast(sub_ast, p);
            char *var = get_value(p, "VAR");
            var_ast->type = _var;
            var_ast->val.strval = var;
            var_ast->begin = var_begin;
            var_ast->end = eq_begin;

            struct ast *eq_ast = prepend_or_reuse_ast(var_ast, p);
            eq_ast->type = _opeq;
            eq_ast->val.strval = "=";
            eq_ast->begin = eq_begin;
            eq_ast->end = p->current_pos;
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
    struct ast *sub_ast = append_or_reuse_ast(ast, p);
    if (readopwhile(p, sub_ast))
    {

        struct ast *whilecond_ast = append_or_reuse_ast(sub_ast, p);

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
                        struct ast *whileblock_ast = append_or_reuse_ast(sub_ast, p);
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
    struct ast *sub_ast = append_or_reuse_ast(ast, p);
    if (readopelse(p, sub_ast))
    {

        clean_space(p);
        if (readchar(p, '{'))
        {
            struct ast *elseblock_ast = append_or_reuse_ast(sub_ast, p);
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
    struct ast *sub_ast = append_or_reuse_ast(ast, p);
    if (readopelif(p, sub_ast))
    {
        struct ast *elifcond_ast = append_or_reuse_ast(sub_ast, p);

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
                        struct ast *elifblock_ast = append_or_reuse_ast(sub_ast, p);
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
    struct ast *sub_ast = append_or_reuse_ast(ast, p);
    if (readopif(p, sub_ast))
    {

        struct ast *ifcond_ast = append_or_reuse_ast(sub_ast, p);

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
                        struct ast *ifblock_ast = append_or_reuse_ast(sub_ast, p);
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
    struct ast *sub_ast = append_or_reuse_ast(ast, p);
    sub_ast->type = _block;
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

    sub_ast->end = p->current_pos;

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

int readcontrol(struct parser *p, struct ast *ast)
{
    int ret = 0;
    int tmp_pos = p->current_pos;

    struct ast *sub_ast = append_or_reuse_ast(ast, p);

    if ((readopbreak(p, sub_ast) || readopcontinue(p, sub_ast)) && readchar(p, ';'))
    {
        ret = 1;
    }
    else if (readopreturn(p, sub_ast) && readcalc(p, sub_ast) && readchar(p, ';'))
        ret = 1;

    if (!ret)
    {
        remove_last(ast);
        p->current_pos = tmp_pos;
    }

    return ret;
}

int readfuncdef(struct parser *p, struct ast *ast)
{
    int ret = 0;

    struct ast *sub_ast = append_or_reuse_ast(ast, p);

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

                struct ast *args_ast = append_or_reuse_ast(sub_ast, p);
                args_ast->type = _args;

                clean_space(p);
                while (readvar(p))
                {
                    char *var = get_value(p, "VAR");
                    struct ast *var_ast = append_or_reuse_ast(args_ast, p);
                    var_ast->type = _var;
                    var_ast->val.strval = var;

                    clean_space(p);
                    if (!readchar(p, ','))
                        break;
                }

                args_ast->end = p->current_pos;

                clean_space(p);
                if (readchar(p, ')'))
                {
                    clean_space(p);
                    if (readchar(p, '{'))
                    {
                        struct ast *func_ast = append_or_reuse_ast(sub_ast, p);
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

    sub_ast->end = p->current_pos;

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

    struct ast *sub_ast = append_or_reuse_ast(ast, p);

    int tmp_pos = p->current_pos;
    if (readvar(p) && readchar(p, '('))
    {
        sub_ast->type = _funccall;
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

    sub_ast->end = p->current_pos;

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

    if (readcomment(p) || readcontrol(p, ast) || readfuncdef(p, ast) || readblock(p, ast) || readexpr(p, ast))
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
        ret = 0;
    }
    else
        ret = 1;

    p->current_pos = 0;
    return ret;
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

int throw_err(struct ast *ast, struct error_scope *err_s, char *msg)
{
    if (err_s->begin == -1)
    {
        err_s->begin = ast->begin;
        err_s->end = ast->end;
        err_s->err = msg;
    }

    return 0;
}

int check_ast(struct ast *ast, struct scope *s, struct visitor_scope *vis_s, struct error_scope *err_s)
{
    if (ast == NULL)
        return 0;

    if (ast->type == _const)
    {
        if (ast->size > 0)
            return throw_err(ast, err_s, "_const should have no edges!");

        return 1;
    }

    if (ast->type == _var)
    {
        if (ast->size > 0)
            return throw_err(ast, err_s, "_var should have no edges!");
        if (!getdef(s, ast->val.strval) && vis_s->state != _invardef)
            return throw_err(ast, err_s, "_var should be defined before being used!");

        if (vis_s->state == _invardef)
        {
            union Definition val;
            create_or_reuse_dl(ast, s, val);
        }

        return 1;
    }

    if (ast->type == _opcontrol)
    {
        if ((!strcmp(ast->val.strval, "break")))
        {
            if (ast->size != 0)
                return throw_err(ast, err_s, "break should not have any edges!");
            if (vis_s->state != _inwhile)
                return throw_err(ast, err_s, "cannot break outside of a loop!");
            return 1;
        }

        if ((!strcmp(ast->val.strval, "continue")))
        {
            if (ast->size != 0)
                return throw_err(ast, err_s, "continue should not have any edges!");
            if (vis_s->state != _inwhile)
                return throw_err(ast, err_s, "cannot continue outside of a loop!");
            return 1;
        }

        if ((!strcmp(ast->val.strval, "return")))
        {
            if (ast->size != 1)
                return throw_err(ast, err_s, "return should have 1 edge!");
            if (vis_s->state != _infunc)
                return throw_err(ast, err_s, "cannot return outside of a funk!");
            return 1;
        }
    }

    if (ast->type == _funcdef)
    {
        if (ast->size != 2)
            return throw_err(ast, err_s, "_funcdef should have 2 edges!");
        if (ast->edges[0]->type != _args)
            return throw_err(ast, err_s, "_funcdef edge[0] should be of type _args!");
        if (ast->edges[1]->type != _compound)
            return throw_err(ast, err_s, "_funcdef edge[1] should be of type _compound!");

        union Definition val;
        val.astptr = ast;
        create_or_reuse_dl(ast, s, val);

        struct scope *func_s = duplicate_scope(s);

        int _ogstate = vis_s->state;
        vis_s->state = _invardef;
        if (!check_ast(ast->edges[0], func_s, vis_s, err_s))
        {
            clean_scope(func_s);
            return 0;
        }
        vis_s->state = 0;

        vis_s->state = _infunc;
        if (!check_ast(ast->edges[1], func_s, vis_s, err_s))
        {
            clean_scope(func_s);
            return 0;
        }
        vis_s->state = _ogstate;

        clean_scope(func_s);
        free(func_s);
        return 1;
    }

    if (ast->type == _funccall)
    {
        struct def_list *func;
        if (!(func = getdef(s, ast->val.strval)))
            return throw_err(ast, err_s, "_funccall should be after function is defined!");
        if (!func->builtin && ast->size != func->val.astptr->edges[0]->size)
            return throw_err(ast, err_s, "_funccall should have the same # of args as the _funcdef!");

        for (int i = 0; i < ast->size; i++)
        {
            if (!check_ast(ast->edges[i], s, vis_s, err_s))
                return 0;
        }

        return 1;
    }

    if (ast->type == _block)
    {
        if ((!strcmp(ast->val.strval, "ifelse")))
        {
            if (ast->size < 1)
                return throw_err(ast, err_s, "ifelse should have atleast 1 edge!");

            for (int i = 0; i < ast->size; i++)
            {
                if (!check_ast(ast->edges[i], s, vis_s, err_s))
                    return 0;
            }

            return 1;
        }

        if ((!strcmp(ast->val.strval, "if") || !strcmp(ast->val.strval, "elif")))
        {
            if (ast->size != 2)
                return throw_err(ast, err_s, "if/elif should have 2 edges!");
            if (ast->edges[1]->type != _compound)
                return throw_err(ast, err_s, "if/elif edge[1] should be of type _compound!");
            if (!check_ast(ast->edges[0], s, vis_s, err_s) || !check_ast(ast->edges[1], s, vis_s, err_s))
                return 0;

            return 1;
        }

        if (!strcmp(ast->val.strval, "else"))
        {
            if (ast->size != 1)
                return throw_err(ast, err_s, "else sould have 1 edge!");
            if (ast->edges[0]->type != _compound)
                return throw_err(ast, err_s, "else edge[0] should be of type _compound!");
            if (!check_ast(ast->edges[0], s, vis_s, err_s))
                return 0;

            return 1;
        }
    }

    if (ast->type == _loop)
    {
        if (ast->size != 2)
            return throw_err(ast, err_s, "_loop should have 2 edges!");
        if (ast->edges[1]->type != _compound)
            return throw_err(ast, err_s, "_loop edges[1] should be of type _compound!");

        int _ogstate = vis_s->state;
        vis_s->state = _inwhile;
        if (!check_ast(ast->edges[0], s, vis_s, err_s) || !check_ast(ast->edges[1], s, vis_s, err_s))
            return 0;
        vis_s->state = _ogstate;

        return 1;
    }

    if (ast->type == _opuna)
    {
        if (ast->size != 1)
            return throw_err(ast, err_s, "_opuna should have 1 edge!");
        if (!check_ast(ast->edges[0], s, vis_s, err_s))
            return 0;

        return 1;
    }

    if (ast->type == _opeq)
    {
        if (ast->size != 2)
            return throw_err(ast, err_s, "_opeq should have 2 edges!");

        if (ast->edges[0]->type != _var)
            return throw_err(ast, err_s, "_opeq edge[0] should be of type _var!");

        int _ogstate = vis_s->state;
        vis_s->state = _invardef;
        if (!check_ast(ast->edges[0], s, vis_s, err_s))
            return 0;
        vis_s->state = _ogstate;

        if (!check_ast(ast->edges[1], s, vis_s, err_s))
            return 0;

        return 1;
    }

    if (ast->type == _oplogic || ast->type == _opcomp || ast->type == _opmath)
    {
        if (ast->size < 2)
            return throw_err(ast, err_s, "_oplogic/_opcomp/_opmath should have 2 edges!");
        if (!check_ast(ast->edges[0], s, vis_s, err_s) || !check_ast(ast->edges[1], s, vis_s, err_s))
            return 0;

        return 1;
    }

    if (ast->type == _compound || ast->type == _args)
    {
        for (int i = 0; i < ast->size; i++)
        {
            if (!check_ast(ast->edges[i], s, vis_s, err_s))
                return 0;
        }

        return 1;
    }

    return 0;
}

int my_calc(struct parser *p, struct ast *ast, struct error_scope *err_s)
{
    int ret = 0;
    struct scope s;
    s.defs = 0;
    err_s->begin = -1;

    ret = readlang(p, ast);
    if (ret != 0)
    {
        register_builtins(&s);
        struct visitor_scope vs;
        vs.state = 0;
        ret = check_ast(ast, &s, &vs, err_s);
    }

    clean_scope(&s);

    return ret;
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

int recursive_eval(struct ast *ast, struct scope *s, struct control_scope *ctrl_s)
{
    if (ast == NULL)
        return 0;

    if (!ast->size && ast->type == _const)
    {
        s->current_val = ast->val.intval;
        return 1;
    }

    if (!ast->size && ast->type == _var)
    {
        struct def_list *ptr;

        if ((ptr = getdef(s, ast->val.strval)))
        {
            s->current_val = ptr->val.intval;
            return 1;
        }

        return 0;
    }

    if (ast->type == _opcontrol)
    {
        if (!strcmp(ast->val.strval, "break"))
        {
            ctrl_s->breakcnt += 1;
            return 1;
        }

        if (!strcmp(ast->val.strval, "continue"))
        {
            ctrl_s->continuecnt += 1;
            return 1;
        }

        if (!strcmp(ast->val.strval, "return"))
        {
            int ret = 0;

            ctrl_s->returncnt += 1;

            ret = recursive_eval(ast->edges[0], s, ctrl_s);

            return ret;
        }
    }

    if (ast->type == _funcdef)
    {
        union Definition val;
        val.astptr = ast;
        return create_or_reuse_dl(ast, s, val);
    }
    else if (ast->type == _funccall)
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
                recursive_eval(ast->edges[i], s, ctrl_s);
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

                        int i;
                        for (i = 0; i < func_ast->edges[1]->size; i++)
                        {
                            ret = recursive_eval(func_ast->edges[1]->edges[i], func_scope, ctrl_s);
                            if (!ret)
                                break;

                            if (ctrl_s->returncnt)
                            {
                                ctrl_s->returncnt -= 1;
                            }
                        }

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

    if (ast->type == _block)
    {
        if (!strcmp(ast->val.strval, "ifelse") && ast->size > 0)
        {
            int ret = 0;

            int i;
            for (i = 0; i < ast->size; i++)
            {
                ret = recursive_eval(ast->edges[i], s, ctrl_s);
                if (ret == 1)
                    break;
            }

            return 1;
        }
        else if ((!strcmp(ast->val.strval, "if") || !strcmp(ast->val.strval, "elif")) && ast->size > 1)
        {
            int ret = 0;

            recursive_eval(ast->edges[0], s, ctrl_s);
            if (s->current_val)
            {
                ret = recursive_eval(ast->edges[1], s, ctrl_s);
            }

            return ret;
        }
        else if (!strcmp(ast->val.strval, "else") && ast->size > 0)
        {
            return recursive_eval(ast->edges[0], s, ctrl_s);
        }
    }

    if (ast->type == _loop)
    {
        if (!strcmp(ast->val.strval, "while") && ast->size > 1)
        {
            int ret = 0;

            recursive_eval(ast->edges[0], s, ctrl_s);
            if (s->current_val)
            {
                for (int i = 0; i < ast->edges[1]->size; i++)
                {                    
                    ret = recursive_eval(ast->edges[1]->edges[i], s, ctrl_s);

                    if (ret == 0)
                        break;

                    if (ctrl_s->breakcnt)
                    {
                        ctrl_s->breakcnt -= 1;
                        break;
                    }

                    if (i == ast->edges[1]->size - 1 || ctrl_s->continuecnt)
                    {
                        if(ctrl_s->continuecnt)
                            ctrl_s->continuecnt -= 1;

                        i = -1;

                        recursive_eval(ast->edges[0], s, ctrl_s);
                        if (!s->current_val)
                            break;
                    }
                }
            }

            return ret;
        }
    }

    if (ast->type == _opuna)
    {
        int ret;

        ret = recursive_eval(ast->edges[0], s, ctrl_s);
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

        ret = recursive_eval(ast->edges[1], s, ctrl_s);
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

        ret = recursive_eval(ast->edges[0], s, ctrl_s);
        int l = s->current_val;
        ret = recursive_eval(ast->edges[1], s, ctrl_s);
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
            ret = recursive_eval(ast->edges[i], s, ctrl_s);
        }
    }

    return ret;
}

int eval(struct ast *a, struct scope *s)
{
    struct control_scope cs;
    cs.breakcnt = 0;
    cs.returncnt = 0;
    cs.continuecnt = 0;

    s->defs = 0;
    register_builtins(s);
    recursive_eval(a, s, &cs);
    clean_scope(s);
    return 1;
}
