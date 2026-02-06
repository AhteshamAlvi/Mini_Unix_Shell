# Mini_Unix_Shell

Open command.h (or create it) and define the AST: enum conjunction { NONE, AND, OR, SEMI, PIPE, SUBSHELL }; extern const char *conj[]; struct tree { char **argv; char *input; char *output; enum conjunction conjunction; struct tree *left, *right; };.

Add a source (e.g., command.c) that defines const char *conj[] = { "NONE", "AND", "OR", "SEMI", "PIPE", "SUBSHELL" }; so print_tree links.

Write a lexer/parser to turn a command line into a struct tree with proper precedence: subshells → pipes → AND/OR → SEMI. Handle tokens for words, <, >, |, &&, ||, ;, (, ). Populate argv, input, output, left, right, conjunction. Provide a parse_line(const char *line) that returns a tree or NULL on syntax error.

Create main driver (e.g., d8sh.c): loop printing a prompt, read a line (use getline), if EOF exit; parse; on syntax error print a message; call execute(tree); free the tree; continue. Exit on exit builtin or Ctrl-D.

Refactor builtins in execute so they run in the parent without forking: detect exit, cd, maybe pwd. For cd, return non‑zero on failure.

Fix PIPE return status: after both waitpids, return the right child’s status (not whichever returned last). Ensure each child closes unused pipe ends before execvp.
Add redirection handling to SUBSHELL nodes: if t->input/output are set, apply dup2 in the child before execute(t->left).

Improve error handling: print perror-style messages for open/execvp; return child status on OR branch too.

Add memory cleanup: implement free_tree(struct tree *) to free argv/input/output strings and recurse; call it in the main loop.

(Optional) Add features: background & (no wait, SIGCHLD reap), signal handling (ignore SIGINT in parent, default in children), environment variable expansion and quoting if required.

Create a Makefile that builds d8sh, compiling d8sh.c, executor.c, command.c, parser/lexer files: e.g., d8sh: d8sh.o executor.o command.o parser.o lexer.o with CC=gcc, CFLAGS=-Wall -Wextra -g.

Test manually: run ./d8sh, try ls, ls | wc -l, false && echo ok, true || echo no, redirections <, >, subshell (cd /tmp; pwd), and cd/exit.









