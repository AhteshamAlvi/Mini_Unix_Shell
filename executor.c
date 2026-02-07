#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sysexits.h>
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

int execute(struct tree *t, int allow_builtin) {
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

      if (allow_builtin && handle_builtin(t)) {
         return 0;
      }

      pid_t pid = fork();
      if (pid < 0) {
         perror("fork");
         return 1;
      }

      if (pid != 0) {
         int status;
         waitpid(pid, &status, 0);
         return extract_status(status);
      }

      if (apply_redirection(t) < 0) {
         _exit(1);
      }

      execvp(t->argv[0], t->argv);
      perror(t->argv[0]);
      _exit(127);

   } else if (t->conjunction == AND) {
      // Executes right if left is successful.
      int left = execute(t->left, allow_builtin);
      if (left == 0)
         return execute(t->right, allow_builtin);
      return left;

   } else if (t->conjunction == OR) {
      // Exevutes right if left is unsuccesseful.
      int left = execute(t->left, allow_builtin);
      if (left != 0)
         return execute(t->right, allow_builtin);
      return left;

   } else if (t->conjunction == SEMI) {
      // Executes left then right.
      execute(t->left, 0);
      _exit(execute(t->right, 0));

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
         perror("pipe");
         return 1;
      }

      // left child
      if ((left_pid = fork()) < 0) {
         perror("fork");
         return 1;
      }

      if (left_pid == 0) {
         close(pipe_fd[0]);

         if (dup2(pipe_fd[1], STDOUT_FILENO) < 0) {
            perror("dup2");
            _exit(1);
         }
         close(pipe_fd[1]);

         /* NEW: apply redirection belonging to the left node */
         if (apply_redirection(t->left) < 0) {
            _exit(1);
         }

         _exit(execute(t->left, 0));
      }

      // right child
      if ((right_pid = fork()) < 0) {
         perror("fork");
            return 1;
      }

      if (right_pid == 0) {
         close(pipe_fd[1]);

         if (dup2(pipe_fd[0], STDOUT_FILENO) < 0) {
            perror("dup2");
            _exit(1);
         }
         close(pipe_fd[0]);

         if (apply_redirection(t->right) < 0) {
            _exit(1);
         }

         _exit(execute(t->right, 0));
      }

      // Parent code
      close(pipe_fd[0]);
      close(pipe_fd[1]);

      int left_status, right_status;

      waitpid(left_pid, &left_status, 0);
      waitpid(right_pid, &right_status, 0);

      return extract_status(right_status);
      
   } else if (t->conjunction == SUBSHELL) {
      pid_t pid = fork();
      if (pid < 0) {
         perror("fork");
         return 1;
      }

      if (pid != 0) {
         int status;
         waitpid(pid, &status, 0);
         return extract_status(status);
      }

      if (apply_redirection(t) < 0) {
         _exit(1);
      }

      _exit(execute(t->left, 0));
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
