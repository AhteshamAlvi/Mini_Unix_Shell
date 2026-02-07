#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "lexer.h"

typedef struct {
    lexer_t lx;
    token_t cur;
} parser_t;

static void advance(parser_t *ps) {
    token_free(&ps->cur);
    ps->cur = lexer_next(&ps->lx);
}

static int expect(parser_t *ps, tok_type_t type) {
    if (ps->cur.type != type)
        return 0;
    advance(ps);
    return 1;
}

static struct tree *parse_semi(parser_t *ps);
static struct tree *parse_and_or(parser_t *ps);
static struct tree *parse_pipe(parser_t *ps);
static struct tree *parse_command(parser_t *ps);

void free_tree(struct tree *t) {
    if (!t) return;

    free_tree(t->left);
    free_tree(t->right);

    if (t->argv) {
        for (int i = 0; t->argv[i]; i++)
            free(t->argv[i]);
        free(t->argv);
    }

    free(t->input);
    free(t->output);
    free(t);
}

static struct tree *parse_command(parser_t *ps) {
    if (ps->cur.type == TOK_LPAREN) {
        advance(ps);

        struct tree *inside = parse_semi(ps);
        if (!inside) return NULL;

        if (!expect(ps, TOK_RPAREN)) {
            free_tree(inside);
            return NULL;
        }

        struct tree *t = calloc(1, sizeof(*t));
        t->conjunction = SUBSHELL;
        t->left = inside;
        return t;
    }

    if (ps->cur.type != TOK_WORD)
        return NULL;

    struct tree *t = calloc(1, sizeof(*t));
    t->conjunction = NONE;

    size_t argc = 0, cap = 4;
    t->argv = malloc(sizeof(char *) * cap);

    while (ps->cur.type == TOK_WORD ||
           ps->cur.type == TOK_LT ||
           ps->cur.type == TOK_GT) {

        if (ps->cur.type == TOK_WORD) {
            if (argc + 1 >= cap) {
                cap *= 2;
                t->argv = realloc(t->argv, sizeof(char *) * cap);
            }
            t->argv[argc++] = strdup(ps->cur.text);
            advance(ps);
        } else if (ps->cur.type == TOK_LT) {
            advance(ps);
            if (ps->cur.type != TOK_WORD) {
                free_tree(t);
                return NULL;
            }
            t->input = strdup(ps->cur.text);
            advance(ps);
        } else if (ps->cur.type == TOK_GT) {
            advance(ps);
            if (ps->cur.type != TOK_WORD) {
                free_tree(t);
                return NULL;
            }
            t->output = strdup(ps->cur.text);
            advance(ps);
        }
    }

    t->argv[argc] = NULL;
    return t;
}

static struct tree *parse_pipe(parser_t *ps) {
    struct tree *left = parse_command(ps);
    if (!left) return NULL;

    while (ps->cur.type == TOK_PIPE) {
        advance(ps);

        struct tree *right = parse_command(ps);
        if (!right) {
            free_tree(left);
            return NULL;
        }

        struct tree *t = calloc(1, sizeof(*t));
        t->conjunction = PIPE;
        t->left = left;
        t->right = right;
        left = t;
    }

    return left;
}

static struct tree *parse_and_or(parser_t *ps) {
    struct tree *left = parse_pipe(ps);
    if (!left) return NULL;

    while (ps->cur.type == TOK_AND || ps->cur.type == TOK_OR) {
        tok_type_t op = ps->cur.type;
        advance(ps);

        struct tree *right = parse_pipe(ps);
        if (!right) {
            free_tree(left);
            return NULL;
        }

        struct tree *t = calloc(1, sizeof(*t));
        t->conjunction = (op == TOK_AND) ? AND : OR;
        t->left = left;
        t->right = right;
        left = t;
    }

    return left;
}

static struct tree *parse_semi(parser_t *ps) {
    struct tree *left = parse_and_or(ps);
    if (!left) return NULL;

    while (ps->cur.type == TOK_SEMI) {
        advance(ps);

        struct tree *right = parse_and_or(ps);
        if (!right) {
            free_tree(left);
            return NULL;
        }

        struct tree *t = calloc(1, sizeof(*t));
        t->conjunction = SEMI;
        t->left = left;
        t->right = right;
        left = t;
    }

    return left;
}

struct tree *parse_line(const char *line) {
    parser_t ps;
    lexer_init(&ps.lx, line);
    ps.cur = lexer_next(&ps.lx);

    struct tree *t = parse_semi(&ps);
    if (!t || ps.cur.type != TOK_EOF) {
        free_tree(t);
        token_free(&ps.cur);
        return NULL;
    }

    token_free(&ps.cur);
    return t;
}

