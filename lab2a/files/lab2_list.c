#include <pthread.h>
#include <time.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h> //atoi
#include <signal.h>
#include "SortedList.h"

void print_usage(const char *exec_name);

int wrapped_clock_gettime(clockid_t clk_id,
			  struct timespec *tp,
			  const char *err_msg);
int wrapped_pthread_create(pthread_t *thread,
			   const pthread_attr_t *attr,
			   void *(*start_routine) (void *),
			   void *arg,
			   const char *err_msg);
int wrapped_pthread_join(pthread_t thread,
			 void **retval,
			 const char *err_msg);
void *wrapped_malloc(size_t size, const char *err_msg);

void signal_handler (int signo);
int format_yieldopts (char *formatted_yieldopts,
		      const char *unformatted_yieldopts,
		      int buf_size);
void list_elements_init();
void list_elements_free_keys();
void *list_routine(void *arg);
long long get_time_diff(struct timespec a, struct timespec b);
void print_csv();

typedef struct __spinlock_t
{
  int flag;
} spinlock_t;

void spinlock_init(spinlock_t *lock);
void spinlock_lock(spinlock_t *lock);
void spinlock_unlock(spinlock_t *lock);

void SortedList_insert_mutex(SortedList_t *list, SortedListElement_t *element);
int SortedList_delete_mutex(SortedListElement_t *element);
SortedListElement_t *SortedList_lookup_mutex(SortedList_t *list, const char *key);
int SortedList_length_mutex(SortedList_t *list);

void SortedList_insert_spinlock(SortedList_t *list, SortedListElement_t *element);
int SortedList_delete_spinlock(SortedListElement_t *element);
SortedListElement_t *SortedList_lookup_spinlock(SortedList_t *list, const char *key);
int SortedList_length_spinlock(SortedList_t *list);

char *test_name;
long long num_threads = 1;
long long num_iterations = 1;
long long run_time;

SortedListElement_t *list_elements;
SortedList_t main_list = {NULL, NULL, NULL};

struct option long_opts[] = {
  {"threads", optional_argument, NULL, 't'},
  {"iterations", optional_argument, NULL, 'i'},
  {"yield", required_argument, NULL, 'y'},
  {"sync", required_argument, NULL, 's'}
};

int opt_sync = 0;
char opt_sync_arg;

int opt_yield = 0;
char *yieldopts;

pthread_mutex_t list_mutex;
spinlock_t list_spinlock;

int main(int argc, char *argv[])
{
  //Catch segmentation faults.
  signal(SIGSEGV, signal_handler);

  //Parsing options.
  yieldopts = (char*)wrapped_malloc(100 * sizeof(char), "Failed to allocated memory for yieldopts");
  
  int opt;
  while((opt = getopt_long(argc, argv, "", long_opts, NULL)) != -1)
    {
      switch(opt)
	{
	case 't':
	  if (optarg != NULL)
	    num_threads = atoi(optarg);
	  break;
	case 'i':
	  if (optarg != NULL)
	    num_iterations = atoi(optarg);
	  break;
	case 'y':
	  opt_yield = 1;
	  if (format_yieldopts(yieldopts, optarg, 100) == -1)
	    {
	      print_usage(argv[0]);
	      exit(1);
	    }
	  break;
	case 's':
	  opt_sync = 1;
	  opt_sync_arg = optarg[0];
	  break;
	default:
	  print_usage(argv[0]);
	  exit(1);
	}
    }

  //Setting test_name, initializing locks.
  test_name = (char*)wrapped_malloc(100 * sizeof(char), "Failed to allocate memory for test_name");
  bzero(test_name, 100);
  strcpy(test_name, "list");
  
  if (opt_yield)
    {
      strcat(test_name, "-");
      strcat(test_name, yieldopts);
    }
  else
    strcat(test_name, "-none");
  
  if (opt_sync)
    switch(opt_sync_arg)
      {
      case 'm':
	pthread_mutex_init(&list_mutex, NULL);
	strcat(test_name, "-m");
	break;
      case 's':
	spinlock_init(&list_spinlock);
	strcat(test_name, "-s");
	break;
      default:
	print_usage(argv[0]);
	exit(1);
      }
  else
    strcat(test_name, "-none");

  //Create num_thread * num_iterations list elements.
  //Initialize their keys with random alphanumeric length-10 C strings.
  list_elements = (SortedListElement_t *)wrapped_malloc(num_threads * num_iterations * sizeof(SortedListElement_t),
							"Failed to allocate memory for list_elements");
  list_elements_init(10);

  //Keep track of time.
  struct timespec start;
  struct timespec stop;
	
  wrapped_clock_gettime(CLOCK_MONOTONIC, &start,
			"Failed to get start time");

  //Make and join threads calling list_routine.
  pthread_t thread_ids[num_threads];
  int thread_num[num_threads];

  int i;
  for(i = 0; i < num_threads; i++)
    {
      thread_num[i] = i;
      wrapped_pthread_create(&thread_ids[i], NULL,
			     list_routine, &thread_num[i],
			     "Failed to create thread");
    }
  
  for(i = 0; i < num_threads; i++)
    {
      wrapped_pthread_join(thread_ids[i], NULL,
			   "Failed to join thread");
    }
  
  wrapped_clock_gettime(CLOCK_MONOTONIC, &stop,
			"Failed to get stop time");
  
  run_time = get_time_diff(stop, start);

  //Check ending length of list.
  int end_len = SortedList_length(&main_list);
  if (end_len == -1)
    {
      fprintf(stderr, "Next/prev pointer corruption found during SortedList_length call.\n");
      exit(2);
    }
  else if (end_len != 0)
    {
      fprintf(stderr, "List length is not zero at test end.\n");
      exit(2);
    }
  
  print_csv();

  list_elements_free_keys();
  free(list_elements);
  free(test_name);
  free(yieldopts);
  
  exit(0);
}

void print_usage(const char *exec_name) {
  fprintf (stderr, "Usage: %s [--threads=#] [--iterations=#] [--yield=[idl]] [--sync=[ms]]\n", exec_name);
  return;
}

int wrapped_clock_gettime(clockid_t clk_id,
			  struct timespec *tp,
			  const char *err_msg)
{
  int rc = clock_gettime(clk_id, tp);
  if (rc == -1)
    {
      fprintf(stderr, "%s: %s\n", err_msg, strerror(errno));
      exit(1);
    }
  return rc;
}

int wrapped_pthread_create(pthread_t *thread,
			   const pthread_attr_t *attr,
			   void *(*start_routine) (void *),
			   void *arg,
			   const char *err_msg)
{
  int rc = pthread_create(thread, attr, start_routine, arg);
  if (rc != 0)
    {
      fprintf(stderr, "%s: %s\n", err_msg, strerror(errno));
      exit(1);
    }
  return rc;
}

int wrapped_pthread_join(pthread_t thread,
			 void **retval,
			 const char *err_msg)
{
  int rc = pthread_join(thread, retval);
  if (rc != 0)
    {
      fprintf(stderr, "%s: %s\n", err_msg, strerror(errno));
      exit(1);
    }
  return rc;
}

void *wrapped_malloc(size_t size, const char *err_msg)
{
  void *rp = malloc(size);
  if (rp == NULL)
    {
      fprintf(stderr, "%s: %s\n", err_msg, strerror(errno));
      exit(1);
    }
  return rp;
}

void signal_handler (int signo) {
  if (signo == SIGSEGV)
    {
      fprintf(stderr, "Caught and received SIGSEGV.\n");
      exit(2);
    }
}

//Formats unformatted_yieldopts and makes formatted_yieldopts
//point to the formatted version. Also, sets opt_yield to appropriate value.
//Returns 0 if successful, -1 if unrecognized yieldopt.
int format_yieldopts (char *formatted_yieldopts,
		      const char *unformatted_yieldopts,
		      int buf_size)
{ 
  bzero(formatted_yieldopts, buf_size);
  
  int arg_i = 0;
  int arg_d = 0;
  int arg_l = 0;

  int i;
  for (i = 0; unformatted_yieldopts[i] != '\0'; i++)
    {
      switch (unformatted_yieldopts[i])
	{
	case 'i':
	  arg_i = 1;
	  break;
	case 'd':
	  arg_d = 1;
	  break;
	case 'l':
	  arg_l = 1;
	  break;
	default:
	  return -1;
	}
    }

  if (arg_i)
    {
      opt_yield |= INSERT_YIELD;
      strcat(formatted_yieldopts, "i");
    }
  if (arg_d)
    {
      opt_yield |= DELETE_YIELD;
      strcat(formatted_yieldopts, "d");
    }
  if (arg_l)
    {
      opt_yield |= LOOKUP_YIELD;
      strcat(formatted_yieldopts, "l");
    }

  return 0;
}

//Initializes the keys of list_elements to be random alphanumeric C strings of length len.
void list_elements_init(int len)
{
  srand((unsigned int)time(NULL));

  char chars[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"; 
  int amt = strlen(chars);
  
  int i,j;
  for (i = 0; i < num_threads * num_iterations; i++)
    {
      char *rand_key_i = (char *)wrapped_malloc(sizeof(char) * (len + 1),
						"Failed to allocate memory for list element key");
      bzero(rand_key_i, len + 1);
      
      for (j = 0; j < len; j++)
	  strncat(rand_key_i, &chars[rand() % amt], 1);
      
      list_elements[i].key = rand_key_i;
    }
}

//Frees keys of list_elements.
void list_elements_free_keys()
{
  int i;
  for (i = 0; i < num_threads * num_iterations; i++)
    {
      free((char*)list_elements[i].key);
    }
}

//Routine to be called by threads.
void *list_routine(void *thread_num)
{
  int elem_index = *(int *)thread_num;
  int i;

  if (opt_sync)
    {
      switch (opt_sync_arg)
	{
	case 'm':
	  //Mutex.
	  for (i = 0; i < num_iterations; i++)
	    {
	      SortedList_insert_mutex(&main_list, &list_elements[elem_index * num_iterations + i]);
	    }
  
	  if (SortedList_length_mutex(&main_list) == -1)
	    {
	      fprintf(stderr, "Next/prev pointer corruption found during thread %d's SortedList_length call.\n", elem_index);
	      exit(2);
	    }

	  for (i = 0; i < num_iterations; i++)
	    {
	      SortedListElement_t *to_delete = SortedList_lookup_mutex(&main_list, list_elements[elem_index * num_iterations + i].key);
  
	      if (to_delete == NULL)
		{
		  fprintf(stderr, "Unable to find previously inserted element in SortedList_lookup call.\n");
		  exit(2);
		}

	      if (SortedList_delete_mutex(to_delete) != 0)
		{
		  fprintf(stderr, "Next/prev pointer corruption found during SortedList_delete call.\n");
		  exit(2);
		}
	    }
	  break;
	case 's':
	  //Spinlock.
	  for (i = 0; i < num_iterations; i++)
	    {
	      SortedList_insert_spinlock(&main_list, &list_elements[elem_index * num_iterations + i]);
	    }
  
	  if (SortedList_length_spinlock(&main_list) == -1)
	    {
	      fprintf(stderr, "Next/prev pointer corruption found during thread %d's SortedList_length call.\n", elem_index);
	      exit(2);
	    }

	  for (i = 0; i < num_iterations; i++)
	    {
	      SortedListElement_t *to_delete = SortedList_lookup_spinlock(&main_list, list_elements[elem_index * num_iterations + i].key);
  
	      if (to_delete == NULL)
		{
		  fprintf(stderr, "Unable to find previously inserted element in SortedList_lookup call.\n");
		  exit(2);
		}

	      if (SortedList_delete_spinlock(to_delete) != 0)
		{
		  fprintf(stderr, "Next/prev pointer corruption found during SortedList_delete call.\n");
		  exit(2);
		}
	    }
	  break;
	default:
	  fprintf(stderr, "Argument to option sync changed to invalid.\n");
	  exit(1);
	}
    }
  else
    {
      //Unprotected.
      for (i = 0; i < num_iterations; i++)
	{
	  SortedList_insert(&main_list, &list_elements[elem_index * num_iterations + i]);
	}
  
      if (SortedList_length(&main_list) == -1)
	{
	  fprintf(stderr, "Next/prev pointer corruption found during thread %d's SortedList_length call.\n", elem_index);
	  exit(2);
	}

      for (i = 0; i < num_iterations; i++)
	{
	  SortedListElement_t *to_delete = SortedList_lookup(&main_list, list_elements[elem_index * num_iterations + i].key);
  
	  if (to_delete == NULL)
	    {
	      fprintf(stderr, "Unable to find previously inserted element in SortedList_lookup call.\n");
	      exit(2);
	    }

	  if (SortedList_delete(to_delete) != 0)
	    {
	      fprintf(stderr, "Next/prev pointer corruption found during SortedList_delete call.\n");
	      exit(2);
	    }
	}
    }

  return NULL;
}

//Gets time difference in ns between two timespec structs.
long long get_time_diff(struct timespec a, struct timespec b)
{
  return (long long)((a.tv_sec - b.tv_sec) * 1000000000
		     + (a.tv_nsec - b.tv_nsec));
}

//Prints csv outputs.
void print_csv()
{
  long long num_ops = num_threads * num_iterations * 3;
  long long avg_time_per_op = run_time/num_ops;
  
  fprintf(stdout, "%s,%lli,%lli,%lli,%lli,%lli,%lli\n",
	  test_name,
	  num_threads,
	  num_iterations,
	  (long long)1, //number of lists = 1
	  num_ops,
	  run_time,
	  avg_time_per_op);
		
  return;
}

 void spinlock_init(spinlock_t *lock)
{
  lock->flag = 0;
}

void spinlock_lock(spinlock_t *lock)
{
  while (__sync_lock_test_and_set(&lock->flag, 1) == 1)
    ;
}

void spinlock_unlock(spinlock_t *lock)
{
  __sync_lock_release(&lock->flag);
}

void SortedList_insert_mutex(SortedList_t *list, SortedListElement_t *element)
{
  pthread_mutex_lock(&list_mutex);
  SortedList_insert(list, element);
  pthread_mutex_unlock(&list_mutex);
  return;
}

int SortedList_delete_mutex(SortedListElement_t *element)
{
  pthread_mutex_lock(&list_mutex);
  int rc = SortedList_delete(element);
  pthread_mutex_unlock(&list_mutex);
  return rc;
}

SortedListElement_t *SortedList_lookup_mutex(SortedList_t *list, const char *key)
{
  pthread_mutex_lock(&list_mutex);
  SortedListElement_t *rs = SortedList_lookup(list, key);
  pthread_mutex_unlock(&list_mutex);
  return rs;
}

int SortedList_length_mutex(SortedList_t *list)
{
  pthread_mutex_lock(&list_mutex);
  int rc = SortedList_length(list);
  pthread_mutex_unlock(&list_mutex);
  return rc;
}

void SortedList_insert_spinlock(SortedList_t *list, SortedListElement_t *element)
{
  spinlock_lock(&list_spinlock);
  SortedList_insert(list, element);
  spinlock_unlock(&list_spinlock);
  return;
}

int SortedList_delete_spinlock(SortedListElement_t *element)
{
  spinlock_lock(&list_spinlock);
  int rc = SortedList_delete(element);
  spinlock_unlock(&list_spinlock);
  return rc;
}

SortedListElement_t *SortedList_lookup_spinlock(SortedList_t *list, const char *key)
{
  spinlock_lock(&list_spinlock);
  SortedListElement_t *rs = SortedList_lookup(list, key);
  spinlock_unlock(&list_spinlock);
  return rs;
}

int SortedList_length_spinlock(SortedList_t *list)
{
  spinlock_lock(&list_spinlock);
  int rc = SortedList_length(list);
  spinlock_unlock(&list_spinlock);
  return rc;
}
