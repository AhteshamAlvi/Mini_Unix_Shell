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

static void print_tree (struct tree *t);

int
execute (struct tree *t)
{
  int f_in, f_out, status;
  pid_t pid;

  /* Check existence of tree and arguments */
  if (t == NULL)
    {
      return -1;
    }

  if (t->conjunction == NONE)
    {
      /* Checks for different basic commands (exit, cd, etc) */
      if (strcmp (t->argv[0], "exit") == 0)
	{
	  exit (0);
	}
      else if (strcmp (t->argv[0], "cd") == 0)
	{
	  /* Changes directory if possible. If not prints the error. */
	  if (t->argv[1] == NULL)
	    {
	      chdir (getenv ("HOME"));
	    }
	  else if (chdir (t->argv[1]) != 0)
	    {
	      fprintf (stderr, "%s: No such file or directory.\n",
		       t->argv[1]);
	      fflush (stdout);
	    }
	}

      /* Forks the parent process */
      if ((pid = fork ()) < 0)
	{
	  err (EX_OSERR, "fork error.\n");
	}

      /* Parent Code */
      if (pid != 0)
	{
	  waitpid (pid, &status, 0);
	  return WEXITSTATUS (status);

	  /* Child Code */
	}
      else
	{
	  /* If an input file is provided, changes the file descriptor to the 
	   * input file from the standard input. 
	   */
	  if (t->input != NULL)
	    {
	      if ((f_in = open (t->input, O_RDONLY)) < 0)
		{
		  err (EX_OSERR, "file opening failed.\n");
		}
	      /* stdin now associated with the file */
	      if (dup2 (f_in, STDIN_FILENO) < 0)
		{
		  err (EX_OSERR, "dup2 error\n");
		}
	      /* Closes old file id. */
	      if (close (f_in) < 0)
		{
		  err (EX_OSERR, "close error\n");
		}
	    }

	  /* If an output file is provided, changes the file descriptor to the 
	   * input file from the standard input. 
	   */
	  if (t->output != NULL)
	    {
	      if ((f_out = open (t->output, OPEN_FLAGS, DEF_MODE)) < 0)
		{
		  err (EX_OSERR, "file opening failed.\n");
		}
	      /* stdin now associated with the file */
	      if (dup2 (f_out, STDOUT_FILENO) < 0)
		{
		  err (EX_OSERR, "dup2 error\n");
		}
	      /* Closes old file id. */
	      if (close (f_out) < 0)
		{
		  err (EX_OSERR, "close error\n");
		}
	    }
	  execvp (t->argv[0], t->argv);

	  /* If the execvp command works, it never reaches here. If it fails, 
	   * exits the child process.              
	   */
	  fprintf (stderr, "Failed to execute %s\n", t->argv[0]);
	  exit (1);
	}
      return 0;

    }
  else if (t->conjunction == AND)
    {
      /* Executes right if left is successful. */
      if (execute (t->left) == 0)
	{
	  return execute (t->right);
	}
      return 1;

    }
  else if (t->conjunction == OR)
    {
      /* Exevutes right if left is unsuccesseful. */
      if (execute (t->left) != 0)
	{
	  return execute (t->right);
	}

    }
  else if (t->conjunction == SEMI)
    {
      /* Executes left then right. */
      execute (t->left);
      return execute (t->right);

    }
  else if (t->conjunction == PIPE)
    {
      pid_t left_pid, right_pid;
      int pipe_fd[2];

      /* Ambiguous input redirection check. */
      if (t->left != NULL && t->left->output != NULL)
	{
	  fprintf (stdout, "Ambiguous output redirect.\n");
	  fflush (stdout);
	  return 1;
	}
      /* Ambiguous output redirection check. */
      if (t->right != NULL && t->right->input != NULL)
	{
	  fprintf (stdout, "Ambiguous input redirect.\n");
	  fflush (stdout);
	  return 1;
	}

      if (pipe (pipe_fd) < 0)
	{
	  err (EX_OSERR, "pipe error");
	}

      /* Left Child */
      if ((left_pid = fork ()) < 0)
	{
	  err (EX_OSERR, "fork error.\n");
	}
      if (left_pid == 0)
	{
	  /* Close read end. */
	  close (pipe_fd[0]);

	  /* dup2 so output is in the pipe */
	  dup2 (pipe_fd[1], STDOUT_FILENO);

	  /* Closing write end */
	  close (pipe_fd[1]);

	  exit (execute (t->left));
	}

      /* Right Child */
      if ((right_pid = fork ()) < 0)
	{
	  err (EX_OSERR, "fork error.\n");
	}
      if (right_pid == 0)
	{
	  /* Close write end. */
	  close (pipe_fd[1]);

	  /* dup2 so input comes from the pipe */
	  dup2 (pipe_fd[0], STDIN_FILENO);

	  /* Closing read end */
	  close (pipe_fd[0]);

	  exit (execute (t->right));
	}

      /* Parent code */
      close (pipe_fd[0]);
      close (pipe_fd[1]);

      waitpid (left_pid, &status, 0);
      waitpid (right_pid, &status, 0);
      return WEXITSTATUS (status);

    }
  else if (t->conjunction == SUBSHELL)
    {
      pid_t n_pid;
      if ((n_pid = fork ()) < 0)
	{
	  err (EX_OSERR, "fork error.\n");
	}

      /* Parent Code */
      if (n_pid != 0)
	{
	  waitpid (n_pid, &status, 0);
	  return WEXITSTATUS (status);

	  /* Child Code */
	}
      else
	{
	  /* Executes the command (which is in the left tree) in a subshell. */
	  exit (execute (t->left));
	}
    }

  print_tree (t);
  return 0;
}

static void
print_tree (struct tree *t)
{
  if (t != NULL)
    {
      print_tree (t->left);

      if (t->conjunction == NONE)
	{
	  printf ("NONE: %s, ", t->argv[0]);
	}
      else
	{
	  printf ("%s, ", conj[t->conjunction]);
	}
      printf ("IR: %s, ", t->input);
      printf ("OR: %s\n", t->output);

      print_tree (t->right);
    }
}