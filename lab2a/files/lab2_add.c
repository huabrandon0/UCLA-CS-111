#include <pthread.h>
#include <time.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h> //atoi

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

void add(long long *pointer, long long value);
void add_mutex(long long *pointer, long long value);
void add_spin(long long *pointer, long long value);
void add_cas(long long *pointer, long long value);
void *counter_routine(void *arg);

long long get_time_diff(struct timespec a, struct timespec b);
void print_csv();

typedef struct __spinlock_t
{
  int flag;
} spinlock_t;

void spinlock_init(spinlock_t *lock);
void spinlock_lock(spinlock_t *lock);
void spinlock_unlock(spinlock_t *lock);

long long counter = 0;
char *test_name;
long long num_threads = 1;
long long num_iterations = 1;
long long run_time;

struct option long_opts[] = {
  {"threads", optional_argument, NULL, 't'},
  {"iterations", optional_argument, NULL, 'i'},
  {"yield", no_argument, NULL, 'y'},
  {"sync", required_argument, NULL, 's'}
};

int opt_yield = 0;
int opt_sync = 0;
char opt_sync_arg;

pthread_mutex_t counter_mutex;
spinlock_t counter_spinlock;

int main(int argc, char *argv[])
{
  //Parse options.
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

  //Set test_name, initialize locks.
  test_name = (char*)wrapped_malloc(100 * sizeof(char), "Failed to allocate memory for test_name");
  bzero(test_name, 100);
  strcpy(test_name, "add");
  
  if (opt_yield)
    strcat(test_name, "-yield");
  
  if (opt_sync)
    switch(opt_sync_arg)
      {
      case 'm':
	pthread_mutex_init(&counter_mutex, NULL);
	strcat(test_name, "-m");
	break;
      case 's':
	spinlock_init(&counter_spinlock);
	strcat(test_name, "-s");
	break;
      case 'c':
	strcat(test_name, "-c");
	break;
      default:
	print_usage(argv[0]);
	exit(1);
      }
  else
    strcat(test_name, "-none");

  //Create, join threads. Calculate run_time.
  struct timespec start;
  struct timespec stop;
	
  wrapped_clock_gettime(CLOCK_MONOTONIC, &start,
			"Failed to get start time");
	
  pthread_t thread_ids[num_threads];
	
  int i;
  for(i = 0; i < num_threads; i++)
    {
      wrapped_pthread_create(&thread_ids[i], NULL,
			     counter_routine, NULL,
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
  
  print_csv();
  free(test_name);
  
  exit(0);
}

void print_usage(const char *exec_name) {
  fprintf (stderr, "Usage: %s [--threads=#] [--iterations=#] [--yield] [--sync=[msc]]\n", exec_name);
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

void add(long long *pointer, long long value)
{
  long long sum = *pointer + value;
  if (opt_yield)
    sched_yield();
  *pointer = sum;
}

void add_mutex(long long *pointer, long long value)
{
  pthread_mutex_lock(&counter_mutex);
  long long sum = *pointer + value;
  if (opt_yield)
    sched_yield();
  *pointer = sum;
  pthread_mutex_unlock(&counter_mutex);
}

void add_spin(long long *pointer, long long value)
{
  spinlock_lock(&counter_spinlock);
  long long sum = *pointer + value;
  if (opt_yield)
    sched_yield();
  *pointer = sum;
  spinlock_unlock(&counter_spinlock);
}

void add_cas(long long *pointer, long long value)
{
  long long old, new;
  do {
    old = *pointer;
    new = old + value;
    if (opt_yield)
      sched_yield();
  } while (__sync_val_compare_and_swap(pointer, old, new) != old);
}

void *counter_routine(void *arg)
{
  int i;
  if (opt_sync)
    switch(opt_sync_arg)
      {
      case 'm':
	for (i = 0; i < num_iterations; i++)
	  {
	    add_mutex(&counter, 1);
	    add_mutex(&counter, -1);
	  }
	break;
      case 's':
	for (i = 0; i < num_iterations; i++)
	  {
	    add_spin(&counter, 1);
	    add_spin(&counter, -1);
	  }
	break;
      case 'c':
	for (i = 0; i < num_iterations; i++)
	  {
	    add_cas(&counter, 1);
	    add_cas(&counter, -1);
	  }
	break;
      default:
        fprintf(stderr, "Option argument to sync has become incorrect.\n");
	exit(1);
      }
  else
    for (i = 0; i < num_iterations; i++)
      {
	add(&counter, 1);
	add(&counter, -1);
      }
  
  return NULL;
}

long long get_time_diff(struct timespec a, struct timespec b)
{
  return (long long)((a.tv_sec - b.tv_sec) * 1000000000
		     + (a.tv_nsec - b.tv_nsec));
}

void print_csv()
{
  long long num_ops = num_threads * num_iterations * 2;
  long long avg_time_per_op = run_time/num_ops;
  
  fprintf(stdout, "%s,%lli,%lli,%lli,%lli,%lli,%lli\n",
	 test_name,
	 num_threads,
	 num_iterations,
	 num_ops,
	 run_time,
	 avg_time_per_op,
	 counter);
		
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
