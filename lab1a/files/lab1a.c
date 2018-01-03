/* NAME: Brandon Hua */
/* EMAIL: huabrandon0@gmail.com */
/* ID: 804595738 */
/* lab1a.c */

#include <termios.h> //termios
#include <unistd.h> //termios, read, getopt, pipe, _exit
#include <stdlib.h> //malloc
#include <getopt.h> //getopt_long
#include <stdio.h> //fprintf
#include <poll.h> //poll
#include <sys/types.h> //kill, waitpid
#include <signal.h> //kill, signal
#include <errno.h> //errno
#include <string.h> //strerror
#include <sys/wait.h> //waitpid

const unsigned int NUM_BYTES = 128;
const char LF = '\n';
const char CRLF[2]="\r\n";

struct termios t_original;
int shell_opt = 0;
int towards_shell[2];
int from_shell[2];
pid_t pid_sh;

struct option long_opts[] = {
  {"shell", no_argument, NULL, 's'}
};

void print_usage(char *exec_name);
void check_err_int(char *type, const int rc);
void check_err_ptr(char *type, char *rp);
void restore_state();
void close_pipes();
void wait_print_sh();
void restore_exit(const int ec);
void sigpipe_handler(int signo);
void proc_fromkb(const char input);
void proc_fromsh(const char input);
void loop_nosh();
void loop_sh();
char *char_buf = NULL;

int main(int argc, char *argv[]) {

  //Parse options.
  int opt;
  while ((opt = getopt_long(argc, argv, "", long_opts, NULL)) != -1) {
      switch(opt) {
      case 's':
	shell_opt = 1;
	break;
      default:
	print_usage(argv[0]);
	_exit(1);
      }
  }

  //Change terminal modes.
  int tga_ret = tcgetattr(0, &t_original);
  check_err_int("tcgetattr", tga_ret);

  struct termios t_edited = t_original;
  t_edited.c_iflag = ISTRIP;
  t_edited.c_oflag = 0;
  t_edited.c_lflag = 0;
  int tsa_ret = tcsetattr(0, TCSANOW, &t_edited);
  check_err_int("tcsetattr", tsa_ret);

  if (shell_opt)
    {
      //Catch signal SIGPIPE.
      signal(SIGPIPE, sigpipe_handler);
      
      //Create child process running bash.
      //Connect the parent to the child with pipes.
      pipe(towards_shell);
      pipe(from_shell);
      
      pid_sh = fork();
      check_err_int("fork", pid_sh);
      
      if (pid_sh == 0)
	{
	  //Child process.
	  close(towards_shell[1]);
	  dup2(towards_shell[0], STDIN_FILENO);

	  close(from_shell[0]);
	  dup2(from_shell[1], STDOUT_FILENO);
	  dup2(from_shell[1], STDERR_FILENO);
	  
	  char *sh_args[2];
	  sh_args[0] = "bash";
	  sh_args[1] = NULL;
	  execvp("/bin/bash", sh_args);
	}
      else
	{
	  //Parent process.
	  close(towards_shell[0]);
	  close(from_shell[1]);
	}

      loop_sh();
    }
  else
    loop_nosh();
  
  restore_exit(0);
}


void print_usage(char *exec_name) {
  fprintf (stderr, "Usage: %s [--shell]\n", exec_name);
}

void check_err_int(char *type, const int rc)
{
  if (rc == -1)
    {
      fprintf(stderr, "Failed %s: %s\n", type, strerror(errno));
      restore_exit(1);
    }
  return;
}

void check_err_ptr(char *type, char *rp)
{
  if (rp == NULL)
   {
      fprintf(stderr, "Failed %s: %s\n", type, strerror(errno));
      restore_exit(1);
    }
  return;
}

void restore_state()
{
  tcsetattr(0, TCSANOW, &t_original);
}

void close_pipes()
{
  if (shell_opt)
    {
      close(towards_shell[0]);
      close(towards_shell[1]);
      close(from_shell[0]);
      close(from_shell[1]);
    }
}

void wait_print_sh()
{
  int status_sh;
  waitpid(pid_sh, &status_sh, 0);
  int status_sh_low = status_sh & 0x007f;
  int status_sh_high = (status_sh >> 8) & 0x00ff;
  /* int status_sh_low = WTERMSIG(status_sh); */
  /* int status_sh_high = WEXITSTATUS(status_sh); */

  fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n",
	  status_sh_low, status_sh_high);
  return;
}

void restore_exit(const int ec)
{ 
  free(char_buf);
  restore_state();
  
  if (shell_opt)
    {
      close_pipes();
      wait_print_sh();
    }
  
  _exit(ec);
}

void sigpipe_handler (int signo) {
  if (signo == SIGPIPE)
    {
      restore_exit(0);
    }
}

void proc_fromkb(const char input)
{ 
  switch (input)
    {
    case 0x04:
      if (shell_opt)
	close(towards_shell[1]);
      else
	restore_exit(0);
      return;
    case '\r':
    case '\n':
      check_err_int("write", write(1, CRLF, 2));
      if (shell_opt)
	check_err_int("write", write(towards_shell[1], &LF, 1));
      return;
    case 0x03:
      if (shell_opt)
	{
	  kill(pid_sh, SIGINT);
	  return;
	}
    default:
      check_err_int("write", write(1, &input, 1));
      if (shell_opt)
	check_err_int("write", write(towards_shell[1], &input, 1));
      return;
    }
}

void proc_fromsh(const char input)
{
  switch (input)
    {
    case 0x04:
      restore_exit(0);
      return;
    case '\r':
    case '\n':
      check_err_int("write", write(1, CRLF, 2));
      return;
    default:
      check_err_int("write", write(1, &input, 1));
      return;
    }
}

void loop_nosh()
{
  char_buf = (char*)malloc(NUM_BYTES * sizeof(char));
  check_err_ptr("malloc", char_buf);
  
  int i;
  ssize_t nbytes;
  int proc_ret;
  
  for(;;)
    {
      nbytes = read(STDIN_FILENO, char_buf, NUM_BYTES);
      check_err_int("read", nbytes);
      
      for (i = 0; i < nbytes; i++)
	  proc_fromkb(char_buf[i]);
    }

  return;
}

void loop_sh()
{
  struct pollfd fds[2];

  //Keyboard input.
  fds[0].fd = STDIN_FILENO;
  fds[0].events = POLLIN | POLLHUP | POLLERR;

  //Shell output.
  fds[1].fd = from_shell[0];
  fds[1].events = POLLIN | POLLHUP | POLLERR;

  char_buf = (char*)malloc(NUM_BYTES * sizeof(char));
  check_err_ptr("malloc", char_buf);
  
  int poll_ret;
  int i;
  ssize_t nbytes;
  ssize_t proc_ret;
  int escape_flag = 0;
  
  for(;;)
    {
      poll_ret = poll(fds, 2, 0);
      check_err_int("poll", poll_ret);
      
      if (poll_ret > 0) {
	//Keyboard input POLLIN event.
	if (fds[0].revents & POLLIN)
	  {
	    nbytes = read(STDIN_FILENO, char_buf, NUM_BYTES);
	    check_err_int("read", nbytes);
	    
	    for (i = 0; i < nbytes; i++)
		proc_fromkb(char_buf[i]);
	  }

	//Shell output POLLIN event.
	if (fds[1].revents & POLLIN)
	  {
	    nbytes = read(from_shell[0], char_buf, NUM_BYTES);
	    check_err_int("read", nbytes);
	    
	    for (i = 0; i < nbytes; i++)
	        proc_fromsh(char_buf[i]);
	  }

	//Error events.
        if (fds[1].revents & (POLLHUP | POLLERR))
	  {
	    restore_exit(0);
	  }
      }
   }

  return;
}
