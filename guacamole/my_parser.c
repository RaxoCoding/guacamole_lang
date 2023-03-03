#include "my_parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <criterion/logging.h>

struct parser *new_parser(const char *content)
{
    struct parser *p = calloc(1, sizeof(struct parser));
    p->content = content;
    return p;
}

void clean_parser(struct parser *p)
{
    struct capture_list *ptr = p->captures;
    if (ptr != NULL)
    {
        while (ptr->next)
        {
            struct capture_list *tmp = ptr->next;

            free(ptr);
            ptr = tmp;
        }
        free(ptr);
    }


    free(p);
}

int count_lines(struct parser *p, struct position *pos)
{
    int i = 0;
    pos->line = 1;
    pos->col = 1;
    while (i != p->last_pos)
    {
        if (p->content[i] == '\n')
        {
            pos->line++;
            pos->col = 1;
        }
        else
            pos->col++;

        i++;
    }

    return i;
}

int reset_pos(struct parser *p, int tmp)
{
    if (p->current_pos > p->last_pos)
        p->last_pos = p->current_pos;

    p->current_pos = tmp;

    if (readeof(p))
        return 0;
    return 1;
}

char *get_line_error(struct parser *p)
{
    int begin;
    for (begin = p->last_pos; p->content[begin] != '\n' && begin > 0; begin -= 1)
        ;

    if (p->content[begin] == '\n')
    {
        begin += 1;
    }

    int end;
    for (end = p->last_pos; p->content[end] != '\n' && p->content[end]; end += 1)
        ;

    char *res = strndup(&p->content[begin], end - begin);

    for (size_t i = 0; i < strlen(res); i++)
        if (res[i] == '\t')
            res[i] = ' ';

    return res;
}

int readeof(struct parser *p)
{
    if (p->content[p->current_pos] == 0)
        return 1;
    return 0;
}

void nextpos(struct parser *p)
{
    p->current_pos += 1;
    if (p->current_pos > p->last_pos)
        p->last_pos = p->current_pos;
}

int nextchar(struct parser *p)
{
    if (readeof(p))
        return 0;

    nextpos(p);

    return 1;
}

int readchar(struct parser *p, char c)
{
    if (readeof(p))
        return 0;

    if (p->content[p->current_pos] == c)
        return nextchar(p);
    return 0;
}

int readuntil(struct parser *p, char c)
{
    if (readeof(p))
        return 0;

    while (p->content[p->current_pos] != c)
        nextchar(p);

    return nextchar(p);
}

int readrange(struct parser *p, char begin, char end)
{
    if (readeof(p))
        return 0;

    if (begin <= p->content[p->current_pos] && end >= p->content[p->current_pos])
        return nextchar(p);
    return 0;
}

int readnotrange(struct parser *p, char begin, char end)
{
    if (readeof(p))
        return 0;

    if (begin > p->content[p->current_pos] || end < p->content[p->current_pos])
        return nextchar(p);
    return 0;
}

int readset(struct parser *p, char *set)
{
    if (readeof(p))
        return 0;

    for (int i = 0; set[i] != 0; i++)
    {
        if (p->content[p->current_pos] == set[i])
            return nextchar(p);
    }

    return 0;
}

int readnotset(struct parser *p, char *set)
{
    if (readeof(p))
        return 0;

    for (int i = 0; set[i] != 0; i++)
    {
        if (p->content[p->current_pos] == set[i])
            return 0;
    }

    return nextchar(p);
}

int readtext(struct parser *p, char *text)
{
    if (readeof(p))
        return 0;

    int tmp_pos = p->current_pos;
    for (int i = 0; text[i] != 0; i++)
    {
        if (p->content[p->current_pos] == text[i])
            nextchar(p);
        else
        {
            if (p->last_pos == p->current_pos)
                p->last_pos -= 1;
            p->current_pos = tmp_pos;
            return 0;
        }
    }

    return 1;
}

// [0-9]+
int readint(struct parser *p)
{
    if (readeof(p))
        return 0;

    int i = 0;
    while (readrange(p, '0', '9'))
    {
        i++;
    }

    return i;
}

// [a-zA-Z_][a-zA-Z_0-9]*
int readid(struct parser *p)
{
    if (readeof(p))
        return 0;

    if (readrange(p, 'a', 'z') || readrange(p, 'A', 'Z') || readchar(p, '_'))
    {
        while (readrange(p, 'a', 'z') || readrange(p, 'A', 'Z') || readchar(p, '_') || readrange(p, '0', '9'))
            ;
        return 1;
    }

    return 0;
}

/*
    Float <- ('-' / '+')* (Dec / Frac) Exp?

    Dec <- Int '.' Int?

    Frac <- '.' Int

    Exp <- ('e' / 'E') ('-' / '+')? Int

    Int <- [0-9]+
*/
int readfloat_dec(struct parser *p)
{
    if (readint(p) && readchar(p, '.'))
    {
        readint(p);
        return 1;
    }

    return 0;
}
int readfloat_frac(struct parser *p)
{
    if (readchar(p, '.') && readint(p))
        return 1;

    return 0;
}
int readfloat_exp(struct parser *p)
{
    if (readchar(p, 'e') || readchar(p, 'E'))
    {
        readchar(p, '+');
        readchar(p, '-');

        if (readint(p))
            return 1;
    }

    return 0;
}
int readfloat(struct parser *p)
{
    while (readchar(p, '-') || readchar(p, '+'))
        ;

    if (readfloat_dec(p) || readfloat_frac(p))
    {
        readfloat_exp(p);
        return 1;
    }

    return 0;
}

// 2ieme partie - manipulation d'AST

struct capture_list *captures_lookup(struct parser *p, const char *tagname)
{
    struct capture_list *ptr = p->captures;

    if (ptr == NULL)
        return NULL;

    while (strcmp(ptr->tagname, tagname))
    {
        if (ptr->next != NULL)
            ptr = ptr->next;
        else
            return NULL;
    }

    return ptr;
}

struct capture_list *create_or_reuse(struct parser *p, const char *tagname)
{

    struct capture_list *ptr;
    if ((ptr = captures_lookup(p, tagname)) != NULL)
    {
        return ptr;
    }
    else
    {
        struct capture_list *cl = calloc(1, sizeof(struct capture_list));

        if (p->captures == NULL)
        {
            p->captures = cl;
            return cl;
        }

        cl->next = p->captures;
        p->captures = cl;
        return cl;
    }
}

int begin_capture(struct parser *p, const char *tagname)
{
    struct capture_list *cl = create_or_reuse(p, tagname);

    cl->tagname = tagname;
    cl->begin = p->current_pos;

    return 1;
}

int end_capture(struct parser *p, const char *tagname)
{

    struct capture_list *ptr;
    if ((ptr = captures_lookup(p, tagname)) != NULL)
    {
        ptr->end = p->current_pos;
        return 1;
    }

    return 0;
}

char *get_value(struct parser *p, const char *tagname)
{
    struct capture_list *cl;
    if ((cl = captures_lookup(p, tagname)) != NULL)
    {
        int begin = cl->begin;
        int end = cl->end;

        char *r = calloc(1, sizeof(char) * ((end - begin) + 1));
        memcpy(r, p->content + begin, sizeof(char) * (end - begin));

        return r;
    }

    return 0;
}
