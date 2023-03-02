#ifndef _MY_PARSER_H
#define _MY_PARSER_H

struct capture_list
{
    const char *tagname;
    int begin;
    int end;
    struct capture_list *next;
};

struct position
{
    int line;
    int col;
};

struct parser
{
    const char *content;
    int current_pos;
    // dernière position maximal atteinte == position de l'erreur
    int last_pos;
    struct capture_list *captures;
    char *err;
};

// instancie et nettoie un parseur
struct parser *new_parser(const char *content);
void clean_parser(struct parser *p);

// retourne vrai si on est à la fin du contenu
int readeof(struct parser *p);

// primitive . (passe au caractère suivant, échoue uniquement si EOF
int nextchar(struct parser *p);
// primitive '[\S\s]*a'. (lis jusqu'au caractère donné, échoue uniquement si EOF
int readuntil(struct parser *p, char c);
// primitive 'a'
int readchar(struct parser *p, char c);
// primitive [a-z]
int readrange(struct parser *p, char begin, char end);
// primitive [^a-z]
int readnotrange(struct parser *p, char begin, char end);
// primitive [...]
int readset(struct parser *p, char *set);
// primitive [^...]
int readnotset(struct parser *p, char *set);
// primitive "hello"
int readtext(struct parser *p, char *text);
// pseudo-primitive [0-9]+
int readint(struct parser *p);
// pseudo-primitive [a-zA-Z_][a-zA-Z_0-9]*
int readid(struct parser *p);

/*
    Float <- ('-' / '+')* (Dec / Frac) Exp?

    Dec <- Int '.' Int?

    Frac <- '.' Int

    Exp <- ('e' / 'E') ('-' / '+')? Int

    Int <- [0-9]+
*/
int readfloat(struct parser *p);

// gestion de l'AST
int begin_capture(struct parser *p, const char *tagname);
int end_capture(struct parser *p, const char *tagname);
char *get_value(struct parser *p, const char *tagname);

// gestion des erreurs

// calcul le nombre de ligne/colonne depuis le début du contenu,
// jusqu'à la position maximal atteinte
// pour être utilisable dans une séquence de && ou || retourne toujours vrai
int count_lines(struct parser *p, struct position *pos);

// modifie current_pos par tmp.
// pour être utilisable dans une séquence de && ou || retourne toujours vrai, peu planter si pos > à EOF
int reset_pos(struct parser *p, int pos);

// extrait et retourne une copie de la dernière ligne avant erreur (last_pos)
char *get_line_error(struct parser *p);

#endif /* _MY_PARSER_H */
