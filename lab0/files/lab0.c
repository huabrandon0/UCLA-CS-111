/* NAME: Brandon Hua */
/* EMAIL: huabrandon0@gmail.com */
/* ID: 804595738 */
/* lab0.c */

#include <stdio.h>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

const int NUM_BYTES = 1;

struct option long_opts[] = {
  {"input", required_argument, NULL, 'i'},
  {"output", required_argument, NULL, 'o'},
  {"segfault", no_argument, NULL, 's'},
  {"catch", no_argument, NULL ,'c'}
};

void sigsegv_handler (int signo) {
  if (signo == SIGSEGV)
    {
      fprintf(stderr, "Caught and received SIGSEGV.\n");
      _exit(4);
    }
}
  
void force_segfault() {
  char *ptr = NULL;
  *ptr = 0;
}

void print_usage(char *exec_name) {
  fprintf (stderr, "Usage: %s [-sc] [-i input_file] [-o output_file]\n", exec_name);
}

int main (int argc, char *argv[]) {
  char* input_file = NULL;
  char* output_file = NULL;
  int segfault_opt = 0;
  int catch_opt = 0;
  
  int opt;
  while ((opt = getopt_long(argc, argv, "i:o:sc", long_opts, NULL)) != -1) {
      switch(opt) {
      case 'i':
	input_file = optarg;
	break;
      case 'o':
	output_file = optarg;
	break;
      case 's':
	segfault_opt = 1;
	break;
      case 'c': ;
	catch_opt = 1;
	break;
      default:
	print_usage(argv[0]);
	_exit(1);
      }
  }

  if (catch_opt)
    signal(SIGSEGV, sigsegv_handler);
  
  if (segfault_opt)
    force_segfault();

  if (input_file != NULL)
    {
      int ifd = open (input_file, O_RDONLY);
      if (ifd >= 0) {
	close(0);
	dup(ifd);
	close(ifd);
      }
      else {
	fprintf(stderr, "Unable to open specified input file: %s\n", strerror(errno));
	_exit(2);
      }
    }

  if (output_file != NULL)
    {
      int ofd = creat(output_file, 0666);
      if (ofd >= 0) {
	close(1);
	dup(ofd);
	close(ofd);
      }
      else {
	fprintf(stderr, "Unable to create specified output file: %s\n", strerror(errno));
	_exit(3);
      }
    }

  //Copy input to output.
  char *char_buf = (char*)malloc(NUM_BYTES * sizeof(char));
  while (1)
    {
      ssize_t nbytes = read(0, char_buf, NUM_BYTES);
      if (nbytes <= 0)
	break;
      
      write(1, char_buf, NUM_BYTES); 
    }

  free(char_buf);
  close(0);
  close(1);
  _exit(0);
}
