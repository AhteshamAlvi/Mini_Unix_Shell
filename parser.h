#ifndef PARSER_H
#define PARSER_H

#include "command.h"

/*
 * Parse a command line into an abstract syntax tree.
 * Returns NULL on syntax error.
 */
struct tree *parse_line(const char *line);

/*
 * Free an AST produced by parse_line.
 */
void free_tree(struct tree *t);

#endif
