#include "my_parser.h"
#include "my_calc.h"
#include <error.h>
#include <stdio.h>
#include <stdlib.h>

char *readfile(char *filename)
{
    FILE *textfile;
    char *text;
    long numbytes;

    textfile = fopen(filename, "r");
    if (textfile == NULL)
        return NULL;

    fseek(textfile, 0L, SEEK_END);
    numbytes = ftell(textfile);
    fseek(textfile, 0L, SEEK_SET);

    text = calloc(numbytes + 1, sizeof(char));
    if (text == NULL)
        return NULL;

    fread(text, sizeof(char), numbytes, textfile);
    fclose(textfile);

    return text;
}

int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        printf("Filename argument expected.\n");
        return 0;
    }

    char *content = readfile(argv[1]);

    struct ast ast;
    struct scope s;
    struct parser *p = new_parser(content);
    if (my_calc(p, &ast) && eval(&ast, &s))
    {
        printf("\nResult : %ld\n", s.current_val);
    }
    else
    {
        // gestion d'erreur
        char *errline = get_line_error(p);
        struct position pos;
        count_lines(p, &pos);
        fprintf(stderr, "Erreur line: %d, col: %d\n", pos.line, pos.col);
        fprintf(stderr, "%s\n", errline);
        for (int i = 0; i < pos.col - 1; i += 1)
            fprintf(stderr, " ");
        fprintf(stderr, "^\n");
        free(errline);
    }

    clean_parser(p);
    clean_ast(&ast);
    free(content);
    return 1;
}
