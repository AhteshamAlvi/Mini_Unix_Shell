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
