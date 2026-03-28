#include "lexer.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static token_t make_tok(tok_type_t type, char *text) {
    token_t t;
    t.type = type;
    t.text = text;
    return t;
}

void lexer_init(lexer_t *lx, const char *line) {
    lx->input = line;
    lx->p = line;
    lx->error = NULL;
}

void token_free(token_t *t) {
    if (t && t->text) {
        free(t->text);
        t->text = NULL;
    }
}

static void skip_ws(lexer_t *lx) {
    while (*lx->p && isspace((unsigned char)*lx->p)) {
        lx->p++;
    }
}

static token_t lex_operator(lexer_t *lx) {
    // assumes lx->p points to a non-space char
    char c = *lx->p;

    if (c == '&') {
        if (lx->p[1] == '&') {
            lx->p += 2;
            return make_tok(TOK_AND, NULL);
        }
        lx->error = "single '&' is not supported (use &&)";
        return make_tok(TOK_ERROR, NULL);
    }

    if (c == '|') {
        if (lx->p[1] == '|') {
            lx->p += 2;
            return make_tok(TOK_OR, NULL);
        }
        lx->p += 1;
        return make_tok(TOK_PIPE, NULL);
    }

    if (c == '<') { lx->p++; return make_tok(TOK_LT, NULL); }
    if (c == '>') { lx->p++; return make_tok(TOK_GT, NULL); }
    if (c == ';') { lx->p++; return make_tok(TOK_SEMI, NULL); }
    if (c == '(') { lx->p++; return make_tok(TOK_LPAREN, NULL); }
    if (c == ')') { lx->p++; return make_tok(TOK_RPAREN, NULL); }

    // not an operator we recognize
    return make_tok(TOK_ERROR, NULL);
}

static int is_metachar(unsigned char c) {
    return (c == '<' || c == '>' || c == '|' || c == '&' ||
            c == ';' || c == '(' || c == ')');
}

static char *substr_dup(const char *start, size_t len) {
    char *s = (char *)malloc(len + 1);
    if (!s) return NULL;
    memcpy(s, start, len);
    s[len] = '\0';
    return s;
}

static token_t lex_word(lexer_t *lx) {
    const char *start = lx->p;
    while (*lx->p && !isspace((unsigned char)*lx->p) && !is_metachar((unsigned char)*lx->p)) {
        lx->p++;
    }

    size_t len = (size_t)(lx->p - start);
    if (len == 0) {
        lx->error = "expected word";
        return make_tok(TOK_ERROR, NULL);
    }

    char *w = substr_dup(start, len);
    if (!w) {
        lx->error = "out of memory";
        return make_tok(TOK_ERROR, NULL);
    }

    return make_tok(TOK_WORD, w);
}

token_t lexer_next(lexer_t *lx) {
    skip_ws(lx);

    if (*lx->p == '\0') {
        return make_tok(TOK_EOF, NULL);
    }

    unsigned char c = (unsigned char)*lx->p;

    // operator starters
    if (is_metachar(c)) {
        token_t t = lex_operator(lx);
        if (t.type == TOK_ERROR && lx->error == NULL) {
            lx->error = "invalid operator";
        }
        return t;
    }

    // everything else becomes a WORD
    return lex_word(lx);
}
