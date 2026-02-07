#ifndef COMMAND_H
#define COMMAND_H

/* Logical connectors between commands as they appear in the parsed line. */
enum conjunction { NONE, AND, OR, SEMI, PIPE, SUBSHELL };

/* Names for each conjunction (index with enum conjunction). */
extern const char *conj[];

/*
 * Abstract syntax tree node for a command line.
 *  - argv        : null-terminated argument vector when conjunction == NONE
 *  - input/output: optional redirection paths (NULL if absent)
 *  - conjunction : operator joining left/right children; NONE for simple cmd
 *  - left/right  : child subtrees used by operators (pipe, and/or, etc.)
 */
struct tree {
    char **argv;
    char *input;
    char *output;
    enum conjunction conjunction;
    struct tree *left;
    struct tree *right;
};

#endif /* COMMAND_H */
