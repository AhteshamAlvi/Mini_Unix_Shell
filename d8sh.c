#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "executor.h"

#define MAXLINE 1024

int main(void) {
    char line[MAXLINE];

    while (1) {
        /* Print prompt */
        printf("d8sh> ");
        fflush(stdout);

        /* Read input */
        if (fgets(line, sizeof(line), stdin) == NULL) {
            /* EOF (Ctrl-D) */
            putchar('\n');
            break;
        }

        /* Strip trailing newline */
        line[strcspn(line, "\n")] = '\0';

        /* Ignore empty lines */
        if (line[0] == '\0')
            continue;

        /* Parse */
        struct tree *t = parse_line(line);
        if (!t) {
            fprintf(stderr, "syntax error\n");
            continue;
        }

        /* Execute */
        execute(t, 1);

        /* Cleanup */
        free_tree(t);
    }

    return 0;
}
