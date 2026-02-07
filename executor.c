#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sysexits.h>
#include <err.h>
#include <fcntl.h>
#include "command.h"
#include "executor.h"

#define OPEN_FLAGS (O_WRONLY | O_TRUNC | O_CREAT)
#define DEF_MODE 0664

static void print_tree(struct tree *t);
static int handle_builtin(struct tree *t);

static int extract_status(int status) {
   if (WIFEXITED(status)) {
      return WEXITSTATUS(status);
   }
   if (WIFSIGNALED(status)) {
      return 128 + WTERMSIG(status);
   }
   return 1;
}

static int apply_redirection(struct tree *t) {
   if (t->input) {
      int fd = open(t->input, O_RDONLY);
      if (fd < 0) {
         perror(t->input);
         return -1;
      }
      if (dup2(fd, STDIN_FILENO) < 0) {
         perror("dup2");
         close(fd);
         return -1;
      }
      close(fd);
   }

   if (t->output) {
      int fd = open(t->output, OPEN_FLAGS, DEF_MODE);
      if (fd < 0) {
         perror(t->output);
         return -1;
      }
      if (dup2(fd, STDOUT_FILENO) < 0) {
         perror("dup2");
         close(fd);
         return -1;
      }
      close(fd);
   }
   return 0;
}

// Checks for different basic commands (exit, cd, etc)
static int handle_builtin(struct tree *t) {
   if (t == NULL || t->argv == NULL || t->argv[0] == NULL) {
      return 0;
   }

   if (strcmp(t->argv[0], "exit") == 0) {
      int code = 0;
      if (t->argv[1] != NULL) {
         code = atoi(t->argv[1]);
      }
      exit(code);
   }

   if (strcmp(t->argv[0], "cd") == 0) {
      if (t->argv[1] == NULL) {
         if (chdir(getenv("HOME")) != 0) {
            perror("cd");
         }
      } else {
         if (chdir(t->argv[1]) != 0) {
            perror("cd");
         }
      }
      // built-in handled
      return 1;
   }
   // not a built-in
   return 0;
}

int execute(struct tree *t) {
   int status;
   pid_t pid;

   if (t == NULL) {
      return -1;
   }

   // Simple Command (Leaf)
   if (t->conjunction == NONE) {
      if (!t->argv || !t->argv[0]) {
         return 0;
      }

      // Built-ins handled first
      if (handle_builtin(t)) {
         return 0;
      }

      // Fork
      if ((pid = fork()) < 0) {
         err(EX_OSERR, "fork error");
      }

      // Parent Code
      if (pid != 0) {
         waitpid(pid, &status, 0);
         return extract_status(status);
      }

      // Child Code
      if (apply_redirection(t) < 0) {
         _exit(1);
      }
      execvp(t->argv[0], t->argv);

      // If the execvp command works, it never reaches here.
      // If it fails, exits the child process.
      perror(t->argv[0]);
      _exit(127);

   } else if (t->conjunction == AND) {
      // Executes right if left is successful.
      int left = execute(t->left);
      if (left == 0) {
         return execute(t->right);
      }
      return left;

   } else if (t->conjunction == OR) {
      // Exevutes right if left is unsuccesseful.
      int left = execute(t->left);
      if (left != 0) {
         return execute(t->right);
      }
      return left;

   } else if (t->conjunction == SEMI) {
      // Executes left then right.
      execute(t->left);
      return execute(t->right);

   } else if (t->conjunction == PIPE) {
      pid_t left_pid, right_pid;
      int pipe_fd[2];

      // Ambiguous input redirection check.
      if (t->left != NULL && t->left->output != NULL) {
         fprintf(stdout, "Ambiguous output redirect.\n");
         return 1;
      }
      // Ambiguous output redirection check.
      if (t->right != NULL && t->right->input != NULL) {
         fprintf(stdout, "Ambiguous input redirect.\n");
         return 1;
      }

      if (pipe(pipe_fd) < 0) {
         err(EX_OSERR, "pipe error");
      }

      // Left Child
      if ((left_pid = fork()) < 0) {
         err(EX_OSERR, "fork error.\n");
      }
      if (left_pid == 0) {
         // Close read end.
         close(pipe_fd[0]);

         // dup2 so output is in the pipe
         dup2(pipe_fd[1], STDOUT_FILENO);

         // Closing write end
         close(pipe_fd[1]);

         exit(execute(t->left));
      }

      // Right Child
      if ((right_pid = fork()) < 0) {
         err(EX_OSERR, "fork error.\n");
      }
      if (right_pid == 0) {
         // Close write end.
         close(pipe_fd[1]);

         // dup2 so input comes from the pipe
         dup2(pipe_fd[0], STDIN_FILENO);

         // Closing read end
         close(pipe_fd[0]);

         exit(execute(t->right));
      }

      // Parent code
      close(pipe_fd[0]);
      close(pipe_fd[1]);

      waitpid(left_pid, &status, 0);
      waitpid(right_pid, &status, 0);
      return extract_status(status);
      
   } else if (t->conjunction == SUBSHELL) {
      pid_t n_pid;
      if ((n_pid = fork()) < 0) {
         err(EX_OSERR, "fork error.\n");
      }

      // Parent Code
      if (n_pid != 0) {
         waitpid(n_pid, &status, 0);
         return extract_status(status);

         // Child Code
      } else {
         // Executes the command (which is in the left tree) in a subshell.
         exit(execute(t->left));
      }
   }

   print_tree(t);
   return 0;
}

static void
print_tree(struct tree *t) {
   if (t != NULL) {
      print_tree(t->left);

      if (t->conjunction == NONE) {
         printf("NONE: %s, ", t->argv[0]);
      } else {
         printf("%s, ", conj[t->conjunction]);
      }
      printf("IR: %s, ", t->input);
      printf("OR: %s\n", t->output);

      print_tree(t->right);
   }
}
