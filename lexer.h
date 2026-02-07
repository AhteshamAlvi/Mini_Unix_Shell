#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>

typedef enum {
    TOK_WORD,
    TOK_LT,       // <
    TOK_GT,       // >
    TOK_PIPE,     // |
    TOK_AND,      // &&
    TOK_OR,       // ||
    TOK_SEMI,     // ;
    TOK_LPAREN,   // (
    TOK_RPAREN,   // )
    TOK_EOF,
    TOK_ERROR
} tok_type_t;

typedef struct {
    tok_type_t type;
    char *text;          // only non-NULL for TOK_WORD (heap allocated)
} token_t;

typedef struct {
    const char *input;   // original line
    const char *p;       // current scan position
    const char *error;   // points to a static string describing last error
} lexer_t;

void lexer_init(lexer_t *lx, const char *line);
token_t lexer_next(lexer_t *lx);
void token_free(token_t *t);

#endif
