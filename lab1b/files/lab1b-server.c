/* NAME: Brandon Hua */
/* EMAIL: huabrandon0@gmail.com */
/* ID: 804595738 */
/* lab1b-server.c */

#include <sys/socket.h> //socket
#include <termios.h> //termios
#include <unistd.h> //termios, read, getopt, pipe, _exit
#include <stdlib.h> //malloc
#include <getopt.h> //getopt_long
#include <stdio.h> //fprintf
#include <poll.h> //poll
#include <sys/types.h> //kill, waitpid, socket, kill
#include <signal.h> //kill, signal, kill
#include <errno.h> //errno
#include <string.h> //strerror
#include <sys/wait.h> //waitpid
#include <netinet/in.h> //structures and constants for socket
#include <mcrypt.h> //mcrypt
#include <sys/stat.h> //stat, open
#include <fcntl.h> //stat, open

#define EXIT_OK 0
#define EXIT_SYSCALL_ERR 1
#define EXIT_UNRECOGNIZED_ARG 1
#define EXIT_NO_OPT_PORT 1

struct option long_opts[] = {
  {"port", required_argument, NULL, 'p'},
  {"encrypt", required_argument, NULL, 'e'}
};

const unsigned int NUM_BYTES = 128;
char *char_buf = NULL;
const char *CRLF = "\r\n";

//Socket stuff.
int opt_port = 0;
int port_num = 0;
int listenfd = 0;
int fromfd = 0;

//Child process stuff.
int towards_shell[2];
int from_shell[2];
pid_t pid_sh;

//Encrypt stuff.
int opt_encrypt = 0;
MCRYPT td, td2;
char *key_file = NULL;
char *key = NULL;
int key_len = 0;
char *IV = "AAAAAAAAAAAAAAAA";
char *char_buf_dec = NULL;
char *char_buf_enc = NULL;

void check_err_int(const char *type, const int rc);
void check_err_ptr(const char *type, const void *rp);
void print_usage(const char *exec_name);
void signal_handler(int signo);
void close_fds();
void wait_print_sh();
void restore_exit(const int ec);
void socket_setup();
void pipe_setup();
void loop_port();
void proc_fromcli(const char input);
void proc_fromsh(const char input);
void mcrypt_setup();
void mcrypt_shutdown();

int main(int argc, char *argv[])
{
  //Parse options.
  int opt;
  while ((opt = getopt_long(argc, argv, "", long_opts, NULL)) != -1)
    {
      switch(opt)
	{
	case 'e':
	  opt_encrypt = 1;
	  key_file = optarg;
	  break;
	case 'p':
	  opt_port = 1;
	  port_num = atoi(optarg);
	  break;
	default:
	  print_usage(argv[0]);
	  _exit(EXIT_UNRECOGNIZED_ARG);
	}
    }

  //Allocate memory for buffer.
  char_buf = (char*)malloc(NUM_BYTES * sizeof(char));
    
  //"--port" option protocol.
  if (opt_port)
    { 
      //Catch signal SIGPIPE.
      signal(SIGPIPE, signal_handler);

      //Set up socket.
      socket_setup();

      //Set up child shell process.
      pipe_setup();

      //"--encrypt" option protocol
      if (opt_encrypt)
      	{
      	  //Set up mcrypt.
      	  mcrypt_setup();
      	  char_buf_dec = (char*)malloc(NUM_BYTES * sizeof(char));
	  char_buf_enc = (char*)malloc(NUM_BYTES * sizeof(char));
      	}

      loop_port();
    }
  else
    restore_exit(EXIT_NO_OPT_PORT);
		
  restore_exit(EXIT_OK);
}

void check_err_int(const char *type, const int rc)
{
  if (rc == -1)
    {
      fprintf(stderr, "Failed %s: %s\n", type, strerror(errno));
      restore_exit(EXIT_SYSCALL_ERR);
    }
  return;
}

void check_err_ptr(const char *type, const void *rp)
{
  if (rp == NULL)
   {
      fprintf(stderr, "Failed %s: %s\n", type, strerror(errno));
      restore_exit(EXIT_SYSCALL_ERR);
    }
  return;
}

void print_usage(const char *exec_name)
{
  fprintf(stderr, "Usage: %s [--port=port#]\n", exec_name);
}

void signal_handler (int signo) {
  if (signo == SIGPIPE)
    {
      restore_exit(EXIT_OK);
    }
}


void close_fds()
{
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(listenfd);
  close(towards_shell[1]);
  close(from_shell[0]);
}

void wait_print_sh()
{
  int status_sh;
  waitpid(pid_sh, &status_sh, 0);
  int status_sh_low = status_sh & 0x007f;
  int status_sh_high = (status_sh >> 8) & 0x00ff;
  
  fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n",
	  status_sh_low, status_sh_high);
  return;
}

void restore_exit(const int ec)
{ 
  free(char_buf);
  close_fds();
  wait_print_sh();
  
  close(STDOUT_FILENO);
  close(STDIN_FILENO);

  if (opt_encrypt)
    {
      free(key);
      free(char_buf_dec);
      free(char_buf_enc);
      mcrypt_shutdown();
    }
  
  _exit(ec);
}

void socket_setup()
{
  //Set up socket.
  struct sockaddr_in server_address;
  struct sockaddr_in client_address;

  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  check_err_int("socket", listenfd);
      
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(port_num);

  check_err_int("bind", bind(listenfd,
			     (struct sockaddr*)&server_address,
			     sizeof(server_address)));
  check_err_int("listen", listen(listenfd, 5));
  unsigned cli_len = sizeof(client_address);
  fromfd = accept(listenfd,
		  (struct sockaddr*)&client_address,
		  &cli_len);
  check_err_int("accept", fromfd);

  dup2(fromfd, STDIN_FILENO);
  dup2(fromfd, STDOUT_FILENO);
  close(fromfd);

}

void pipe_setup()
{
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
}

void loop_port()
{
  struct pollfd fds[2];

  //Client input.
  fds[0].fd = STDIN_FILENO;
  fds[0].events = POLLIN | POLLHUP | POLLERR;

  //Shell output.
  fds[1].fd = from_shell[0];
  fds[1].events = POLLIN | POLLHUP | POLLERR;
  
  int poll_ret;
  int i;
  ssize_t nbytes;
  
  for(;;)
    {
      poll_ret = poll(fds, 2, 0);
      check_err_int("poll", poll_ret);
      
      if (poll_ret > 0) {
	//Client input POLLIN event.
	if (fds[0].revents & POLLIN)
	  {
	    nbytes = read(STDIN_FILENO, char_buf, NUM_BYTES);
	    check_err_int("read", nbytes);

	    //EOF from client.
	    if (nbytes == 0)
	      kill(pid_sh, SIGTERM);
	    
	    for (i = 0; i < nbytes; i++)
		proc_fromcli(char_buf[i]);
	  }

	//Shell output POLLIN event.
	if (fds[1].revents & POLLIN)
	  {
	    nbytes = read(from_shell[0], char_buf, NUM_BYTES);
	    check_err_int("read", nbytes);

	    //EOF from shell.
	    if (nbytes == 0)
	      restore_exit(0);
	    
	    for (i = 0; i < nbytes; i++)
	        proc_fromsh(char_buf[i]);
	  }

	//Error events.
        if (fds[1].revents & (POLLHUP | POLLERR))
	  {
	    restore_exit(EXIT_OK);
	  }
      }
   }

  return;
}

void proc_fromcli(const char input)
{
  if (opt_encrypt)
    {
      strncpy(char_buf_dec, &input, 1);
      mdecrypt_generic(td2, char_buf_dec, 1);

      //fprintf(stderr,"received %s from client\n", char_buf_dec);
      switch (*char_buf_dec)
	{
	case 0x03:
	  kill(pid_sh, SIGINT);
	  return;
	case 0x04:
	  close(towards_shell[1]);
	  return;
	default:
	  check_err_int("write", write(towards_shell[1], char_buf_dec, 1));
	  return;
	}
    }
  else
    {
      switch (input)
	{
	case 0x03:
	  kill(pid_sh, SIGINT);
	  return;
	case 0x04:
	  close(towards_shell[1]);
	  return;
	default:
	  check_err_int("write", write(towards_shell[1], &input, 1));
	  return;
	}
    }
}

void proc_fromsh(const char input)
{
  if (opt_encrypt)
    {
      switch (input)
	{
	case 0x04:
	  restore_exit(0);
	  return;
	default:
	  //fprintf(stderr, "encrypting %s\n", &input);
	  strncpy(char_buf_enc, &input, 1);
	  mcrypt_generic(td, char_buf_enc, 1);
	  check_err_int("write", write(STDOUT_FILENO, char_buf_enc, 1));
	  return;
	}
    }
  else
    {
      switch (input)
	{
	case 0x04:
	  restore_exit(0);
	  return;
	default:
	  check_err_int("write", write(STDOUT_FILENO, &input, 1));
	  return;
	}
    }
}

void mcrypt_setup()
{
  int key_fd;

  //Getting the key.
  struct stat keyfile_st;
  check_err_int("stat", stat(key_file, &keyfile_st));
  key_len = keyfile_st.st_size;

  key_fd = open(key_file, O_RDONLY);
  check_err_int("open", key_fd);

  key = (char*)malloc(key_len * sizeof(char));
  check_err_int("read", read(key_fd, key, key_len));

  //fprintf(stderr, "key is %s. len is %d\r\n", key, key_len);
  
  //Setting up the mcrypt module for encryption.
  td = mcrypt_module_open("twofish", NULL, "cfb", NULL);
  if (td == MCRYPT_FAILED)
    {
      fprintf(stderr, "Failed mcrypt_module_open: %s\n", strerror(errno));
      restore_exit(EXIT_SYSCALL_ERR);
    }

  //Setting up the mcrypt module for decryption.
  td2 = mcrypt_module_open("twofish", NULL, "cfb", NULL);
  if (td2 == MCRYPT_FAILED)
    {
      fprintf(stderr, "Failed mcrypt_module_open: %s\n", strerror(errno));
      restore_exit(EXIT_SYSCALL_ERR);
    }
  
  //Initializing both modules.
  if (mcrypt_generic_init(td, key, key_len, IV) < 0)
    {
      fprintf(stderr, "Failed mcrypt_generic_init: %s\n", strerror(errno));
      restore_exit(EXIT_SYSCALL_ERR);
    }

  if (mcrypt_generic_init(td2, key, key_len, IV) < 0)
    {
      fprintf(stderr, "Failed mcrypt_generic_init: %s\n", strerror(errno));
      restore_exit(EXIT_SYSCALL_ERR);
    }  
}

void mcrypt_shutdown()
{
  mcrypt_generic_deinit(td);
  mcrypt_module_close(td);
  mcrypt_generic_deinit(td2);
  mcrypt_module_close(td2);
}
