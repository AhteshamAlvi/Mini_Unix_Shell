# d8sh - Mini Unix Shell

A minimal Unix shell written in C. It implements a classic three-stage pipeline — **lexer**, **parser**, **executor** — to tokenize input, build an abstract syntax tree (AST), and execute commands using `fork`/`execvp`.

## Building

Requires `gcc` (or any C99-compatible compiler) and `make`.

```bash
make            # build the d8sh binary
make clean      # remove object files and binary
make test       # build and run the test suite
```

## Usage

```bash
./d8sh
d8sh> echo hello world
hello world
d8sh> ls -la | grep ".c" | wc -l
5
d8sh> exit
```

Exit the shell with the `exit` command or `Ctrl-D` (EOF).

## Features

### Commands
- Execute any program in `$PATH` with arguments: `ls -la /tmp`
- Exit codes are captured and propagated correctly
- Commands killed by signals report exit code `128 + signal_number`

### Operators

| Operator | Syntax | Description |
|----------|--------|-------------|
| Pipe | `cmd1 \| cmd2` | Stdout of `cmd1` feeds into stdin of `cmd2` |
| AND | `cmd1 && cmd2` | Run `cmd2` only if `cmd1` succeeds (exit code 0) |
| OR | `cmd1 \|\| cmd2` | Run `cmd2` only if `cmd1` fails (non-zero exit code) |
| Semicolon | `cmd1 ; cmd2` | Run `cmd1` then `cmd2` unconditionally |
| Subshell | `(cmd1 ; cmd2)` | Group commands in a child process |

Operators follow standard precedence (lowest to highest): `;` < `&&`/`||` < `|` < command.

Multiple pipes are supported: `cmd1 | cmd2 | cmd3`.

Operators can be freely combined: `echo hello | grep hello && echo found`.

### I/O Redirection

| Syntax | Description |
|--------|-------------|
| `cmd < file` | Redirect stdin from file |
| `cmd > file` | Redirect stdout to file (creates or truncates) |

Ambiguous redirects are detected and reported:
```
d8sh> echo hi > out.txt | cat
Ambiguous output redirect.
d8sh> cat < in.txt | cat < in.txt
Ambiguous input redirect.
```

### Built-in Commands

| Command | Description |
|---------|-------------|
| `exit [code]` | Exit the shell with an optional exit code (default 0) |
| `cd [path]` | Change directory. No argument defaults to `$HOME` |

### Signal Handling

- `Ctrl-C` (`SIGINT`) is ignored by the shell process, so it won't kill your session
- Child processes restore default signal behavior, so `Ctrl-C` correctly interrupts running commands

## Architecture

```
Input string
     |
     v
  [Lexer]     lexer.c / lexer.h
     |           Splits input into tokens (words, operators, redirects)
     v
  [Parser]    parser.c / parser.h
     |           Recursive descent parser, builds an AST
     v
   [AST]      command.h / command.c
     |           Tree of `struct tree` nodes with operator type,
     |           argv, redirections, and left/right children
     v
 [Executor]   executor.c / executor.h
                Walks the AST: forks processes, sets up pipes,
                applies redirections, runs builtins
```

### AST Node Structure

```c
struct tree {
    char **argv;                  // argument vector (leaf nodes only)
    char *input;                  // input redirection path, or NULL
    char *output;                 // output redirection path, or NULL
    enum conjunction conjunction; // NONE, AND, OR, SEMI, PIPE, SUBSHELL
    struct tree *left;            // left child
    struct tree *right;           // right child
};
```

For a command like `echo hello | grep hello && echo done`:

```
        AND
       /   \
     PIPE   NONE(echo done)
    /    \
NONE      NONE
(echo     (grep
 hello)    hello)
```

### File Overview

| File | Purpose |
|------|---------|
| `d8sh.c` | Main REPL loop — prompt, read, parse, execute, cleanup |
| `lexer.c` / `lexer.h` | Tokenizer — converts input string into a stream of tokens |
| `parser.c` / `parser.h` | Recursive descent parser — builds the AST from tokens |
| `command.c` / `command.h` | AST node definition and conjunction name strings |
| `executor.c` / `executor.h` | Tree walker — forks, pipes, redirects, and execs |
| `Makefile` | Build system |
| `tests/run_tests.sh` | Shell-based test suite (24 tests) |

## Tests

The test suite feeds commands to `d8sh` via stdin and compares output against expected values.

```bash
make test
```

```
=== Simple Commands ===
  PASS: echo hello
  PASS: echo multiple args
=== Semicolon ===
  PASS: semicolon runs both commands
  PASS: triple semicolon
=== AND operator ===
  PASS: AND success path
  PASS: AND failure path (no output)
...
================================
Results: 24 passed, 0 failed
================================
```

Tests cover: simple commands, semicolons, AND/OR, pipes (single and multi-stage), subshells, input/output redirection, `cd` builtin, ambiguous redirect detection, syntax errors, unknown commands, and combined operator expressions.

## Limitations

Features **not** implemented:

- Background execution (`&`) and job control
- Append redirection (`>>`)
- Here-documents (`<<EOF`)
- File descriptor redirection (`2>&1`, `3>file`)
- Environment variable expansion (`$VAR`, `$?`)
- Command substitution (`` `cmd` ``, `$(cmd)`)
- Glob/wildcard expansion (`*`, `?`, `[...]`)
- Quoting and escape sequences (`"..."`, `'...'`, `\`)
- Command history and line editing
- Tab completion
- Aliases and functions
- `export`, `unset`, `pwd`, and other common builtins
