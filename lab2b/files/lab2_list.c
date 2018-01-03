#include <pthread.h>
#include <time.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include "SortedList.h"

void print_usage(const char *exec_name);
void signal_handler (int signo);
int format_yieldopts (char *formatted_yieldopts,
		      const char *unformatted_yieldopts,
		      int buf_size);
void elements_free_keys();
unsigned long hashkey(const char *key);
void *list_routine(void *arg);
long long get_time_diff(struct timespec stop, struct timespec start);
void print_csv();
void init_all();
void set_test_name();

typedef struct __spinlock_t
{
  int flag;
} spinlock_t;

void spinlock_init(spinlock_t *lock);
void spinlock_lock(spinlock_t *lock);
void spinlock_unlock(spinlock_t *lock);

long long num_threads = 1;
long long num_iterations = 1;
long long run_time = 0;
long long lock_time = 0;
long long *thread_lock_time = NULL;
char *test_name = NULL;
const int TEST_NAME_LEN = 100;
SortedListElement_t *elements = NULL;
const int KEY_LEN = 10;
SortedList_t *sublists;
int num_lists = 1;
pthread_mutex_t *sublist_mutexes = NULL;
spinlock_t *sublist_spinlocks = NULL;

struct option long_opts[] = {
	{"threads", optional_argument, NULL, 't'},
	{"iterations", optional_argument, NULL, 'i'},
	{"yield", required_argument, NULL, 'y'},
	{"sync", required_argument, NULL, 's'},
	{"lists", required_argument, NULL, 'l'}
};

int opt_sync = 0;
char opt_sync_arg;
int opt_yield = 0;
char *yieldopts = NULL;
const int YIELD_OPT_LEN = 10;

int main(int argc, char *argv[])
{
	signal(SIGSEGV, signal_handler);
	yieldopts = (char*)malloc(YIELD_OPT_LEN * sizeof(char));
	if (yieldopts == NULL)
	{
		fprintf(stderr, "Failed to allocate memory for yieldopts: %s\n", strerror(errno));
		exit(1);
	}
	
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
			if (format_yieldopts(yieldopts, optarg, YIELD_OPT_LEN) == -1)
			{
				print_usage(argv[0]);
				exit(1);
			}
			break;
		case 's':
			opt_sync = 1;
			opt_sync_arg = optarg[0];
			break;
		case 'l':
			num_lists = atoi(optarg);
			break;
		default:
			print_usage(argv[0]);
			exit(1);
		}
    }

	set_test_name();
	init_all();

	struct timespec start;
	clock_gettime(CLOCK_MONOTONIC, &start);

	pthread_t thread_ids[num_threads];
	int thread_nums[num_threads];
  
	int i;
	for(i = 0; i < num_threads; i++)
    {
		thread_nums[i] = i;
		
		if (pthread_create(&thread_ids[i], NULL,
			list_routine, &thread_nums[i]) != 0)
		{
			fprintf(stderr, "Failed to create threads: %s\n", strerror(errno));
			exit(1);	
		}
    }
  
	for(i = 0; i < num_threads; i++)
    {
		if(pthread_join(thread_ids[i], NULL) != 0)
		{
			fprintf(stderr, "Failed to join threads: %s\n", strerror(errno));
			exit(1);
		}
    }
  
	struct timespec stop;
	clock_gettime(CLOCK_MONOTONIC, &stop);
  
	run_time = get_time_diff(stop, start);
  
	lock_time = 0;
	for (i = 0; i < num_threads; i++)
	{
		lock_time += thread_lock_time[i];
	}

	long long list_len_end = 0;
	for (i = 0; i < num_lists; i++)
	{
		long long list_len_i = SortedList_length(&sublists[i]);
		if (list_len_i == -1)
		{
			fprintf(stderr, "Next/prev pointer corruption found during SortedList_length call.\n");
			exit(2);
		}
		
		list_len_end += list_len_i;
	}
	
	if (list_len_end != 0)
    {
      fprintf(stderr, "List length is not zero at test end.\n");
      exit(2);
    }
  
	print_csv();

	elements_free_keys();
	free(elements);
	free(test_name);
	free(yieldopts);
	free(sublists);
	free(sublist_mutexes);
	free(sublist_spinlocks);
	free(thread_lock_time);
  
	exit(0);
}

void print_usage(const char *exec_name) {
  fprintf (stderr, "Usage: %s [--threads=#] [--iterations=#] [--yield=[idl]] [--sync=[ms]] [--lists=#]\n", exec_name);
  return;
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

//Frees keys of elements.
void elements_free_keys()
{
	int i;
	for (i = 0; i < num_threads * num_iterations; i++)
    {
		free((char*)elements[i].key);
    }
}

unsigned long hashkey(const char *key) {
    unsigned long sum = 0;
	int i;
    for (i = 0; i < KEY_LEN; i++)
        sum = sum + key[i];
    return sum%num_lists;
}

//Routine to be called by threads.
void *list_routine(void *thread_num)
{
	long long list_length;
	long long sub_list_length;
	int rc;
	
	struct timespec at_want_lock;
	struct timespec at_has_lock;
	long long time_spent_lock = 0;
	
	int elem_index = *(int *)thread_num;
	
	int i;
	SortedListElement_t *list_element_i;
	SortedList_t *main_list_i;
	pthread_mutex_t *list_mutex_i;
	spinlock_t *list_spinlock_i;
	
	if (opt_sync)
    {
		switch (opt_sync_arg)
		{
		case 'm':
			//Mutex-synchronized list operations.
			//Insert elements.
			for (i = 0; i < num_iterations; i++)
			{
				list_element_i = &elements[elem_index * num_iterations + i];
				main_list_i = &sublists[hashkey(list_element_i->key)];
				list_mutex_i = &sublist_mutexes[hashkey(list_element_i->key)];
				
				clock_gettime(CLOCK_MONOTONIC, &at_want_lock);
				pthread_mutex_lock(list_mutex_i);
				clock_gettime(CLOCK_MONOTONIC, &at_has_lock);
				time_spent_lock += get_time_diff(at_has_lock, at_want_lock);	
				
				SortedList_insert(main_list_i, list_element_i);
				
				pthread_mutex_unlock(list_mutex_i);
			}
	  
			//Get total length of entire compiled list.
			//Get lock.
			clock_gettime(CLOCK_MONOTONIC, &at_want_lock);
			for (i = 0; i < num_lists; i++)
				pthread_mutex_lock(sublist_mutexes + i);
			clock_gettime(CLOCK_MONOTONIC, &at_has_lock);
			time_spent_lock += get_time_diff(at_has_lock, at_want_lock);
			
			list_length = 0;
			for (i = 0; i < num_lists; i++)
			{
				sub_list_length = SortedList_length(main_list_i);
				
				pthread_mutex_unlock(sublist_mutexes + i);
				
				if (sub_list_length == -1)
				{
					fprintf(stderr, "Next/prev pointer corruption found during thread %d's SortedList_length call.\n", elem_index);
					exit(2);
				}
				list_length += sub_list_length;
			}

			//Lookup and delete elements.
			for (i = 0; i < num_iterations; i++)
			{
				//Lookup element.
				list_element_i = &elements[elem_index * num_iterations + i];
				main_list_i = &sublists[hashkey(list_element_i->key)];
				list_mutex_i = &sublist_mutexes[hashkey(list_element_i->key)];
				
				clock_gettime(CLOCK_MONOTONIC, &at_want_lock);
				pthread_mutex_lock(list_mutex_i);
				clock_gettime(CLOCK_MONOTONIC, &at_has_lock);
				time_spent_lock += get_time_diff(at_has_lock, at_want_lock);
				
				SortedListElement_t *to_delete = SortedList_lookup(main_list_i, list_element_i->key);
				
				pthread_mutex_unlock(list_mutex_i);
				
				if (to_delete == NULL)
				{
					fprintf(stderr, "Unable to find previously inserted element in SortedList_lookup call.\n");
					exit(2);
				}
				
				//Delete found element.
				clock_gettime(CLOCK_MONOTONIC, &at_want_lock);
				pthread_mutex_lock(list_mutex_i);
				clock_gettime(CLOCK_MONOTONIC, &at_has_lock);
				time_spent_lock += get_time_diff(at_has_lock, at_want_lock);
				
				rc = SortedList_delete(to_delete);
				
				pthread_mutex_unlock(list_mutex_i);
				
				if (rc != 0)
				{
					fprintf(stderr, "Next/prev pointer corruption found during SortedList_delete call.\n");
					exit(2);
				}
			}
			break;
		case 's':
			//Spinlock-synchronized list operations.
			//Insert elements.
			for (i = 0; i < num_iterations; i++)
			{
				list_element_i = &elements[elem_index * num_iterations + i];
				main_list_i = &sublists[hashkey(list_element_i->key)];
				list_spinlock_i = &sublist_spinlocks[hashkey(list_element_i->key)];
				
				clock_gettime(CLOCK_MONOTONIC, &at_want_lock);
				spinlock_lock(list_spinlock_i);
				clock_gettime(CLOCK_MONOTONIC, &at_has_lock);
				time_spent_lock += get_time_diff(at_has_lock, at_want_lock);	
				
				SortedList_insert(main_list_i, list_element_i);
				
				spinlock_unlock(list_spinlock_i);
			}
	  
			//Get total length of entire compiled list.
			//Get lock.
			clock_gettime(CLOCK_MONOTONIC, &at_want_lock);
			for (i = 0; i < num_lists; i++)
				spinlock_lock(sublist_spinlocks + i);
			clock_gettime(CLOCK_MONOTONIC, &at_has_lock);
			time_spent_lock += get_time_diff(at_has_lock, at_want_lock);
			
			list_length = 0;
			for (i = 0; i < num_lists; i++)
			{
				sub_list_length = SortedList_length(main_list_i);
				
				spinlock_unlock(sublist_spinlocks + i);
				
				if (sub_list_length == -1)
				{
					fprintf(stderr, "Next/prev pointer corruption found during thread %d's SortedList_length call.\n", elem_index);
					exit(2);
				}
				list_length += sub_list_length;
			}

			//Lookup and delete elements.
			for (i = 0; i < num_iterations; i++)
			{
				//Lookup element.
				list_element_i = &elements[elem_index * num_iterations + i];
				main_list_i = &sublists[hashkey(list_element_i->key)];
				list_spinlock_i = &sublist_spinlocks[hashkey(list_element_i->key)];
				
				clock_gettime(CLOCK_MONOTONIC, &at_want_lock);
				spinlock_lock(list_spinlock_i);
				clock_gettime(CLOCK_MONOTONIC, &at_has_lock);
				time_spent_lock += get_time_diff(at_has_lock, at_want_lock);
				
				SortedListElement_t *to_delete = SortedList_lookup(main_list_i, list_element_i->key);
				
				spinlock_unlock(list_spinlock_i);
				
				if (to_delete == NULL)
				{
					fprintf(stderr, "Unable to find previously inserted element in SortedList_lookup call.\n");
					exit(2);
				}
				
				//Delete found element.
				clock_gettime(CLOCK_MONOTONIC, &at_want_lock);
				spinlock_lock(list_spinlock_i);
				clock_gettime(CLOCK_MONOTONIC, &at_has_lock);
				time_spent_lock += get_time_diff(at_has_lock, at_want_lock);
				
				rc = SortedList_delete(to_delete);
				
				spinlock_unlock(list_spinlock_i);
				
				if (rc != 0)
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
		//Unprotected list operations.
		//Insert elements.
		for (i = 0; i < num_iterations; i++)
		{
			list_element_i = &elements[elem_index * num_iterations + i];
			main_list_i = &sublists[hashkey(list_element_i->key)];
			
			SortedList_insert(main_list_i, list_element_i);
		}
  
		//Get total length of entire compiled list.
		list_length = 0;
		for (i = 0; i < num_lists; i++)
		{
			sub_list_length = SortedList_length(main_list_i);
			if (sub_list_length == -1)
			{
				fprintf(stderr, "Next/prev pointer corruption found during thread %d's SortedList_length call.\n", elem_index);
				exit(2);
			}
			list_length += sub_list_length;
		}

		//Lookup and delete elements.
		for (i = 0; i < num_iterations; i++)
		{
			//Lookup element.
			list_element_i = &elements[elem_index * num_iterations + i];
			main_list_i = &sublists[hashkey(list_element_i->key)];
			
			SortedListElement_t *to_delete = SortedList_lookup(main_list_i, list_element_i->key);
			if (to_delete == NULL)
			{
				fprintf(stderr, "Unable to find previously inserted element in SortedList_lookup call.\n");
				exit(2);
			}
			
			//Delete found element.
			rc = SortedList_delete(to_delete);
			if (rc != 0)
			{
				fprintf(stderr, "Next/prev pointer corruption found during SortedList_delete call.\n");
				exit(2);
			}
		}
		time_spent_lock = 0;
    }

	thread_lock_time[elem_index] = time_spent_lock;
	return NULL;
}

long long get_time_diff(struct timespec stop, struct timespec start)
{
  return (long long)((stop.tv_sec - start.tv_sec) * 1000000000
		     + (stop.tv_nsec - start.tv_nsec));
}

void print_csv()
{
	long long num_ops = num_threads * num_iterations * 3;
	long long avg_time_per_op = run_time/num_ops;
	long long avg_wait_for_lock = lock_time/num_ops;
		
	fprintf(stdout, "%s,%lli,%lli,%lli,%lli,%lli,%lli,%lli\n",
		test_name,
		num_threads,
		num_iterations,
		num_lists,
		num_ops,
		run_time,
		avg_time_per_op,
		avg_wait_for_lock);
		
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

//Needs the following to be initialized before calling:
//	num_lists, num_threads, num_iterations.
//Initializes sublists, elements (and keys), sublist_mutexes, sublist_spinlocks, thread_lock_time.
void init_all()
{
	int i, j;
	sublists = (SortedList_t *)malloc(sizeof(SortedList_t) * num_lists);
	if (sublists == NULL)
	{
		fprintf(stderr, "Failed to allocate memory for sublists: %s\n", strerror(errno));
		exit(1);
	}
	
	SortedList_t list_head_init = {NULL, NULL, NULL};
	for (i = 0; i < num_lists; i++)
		sublists[i] = list_head_init;
	
	//Initialize list elements.
	long long num_elements = num_threads * num_iterations;
	elements = (SortedListElement_t *)malloc(num_elements * sizeof(SortedListElement_t));
	if (elements == NULL)
	{
		fprintf(stderr, "Failed to allocate memory for elements: %s\n", strerror(errno));
		exit(1);
	}
	
	srand((unsigned int)time(NULL));
	char chars[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"; 
	int amt = strlen(chars);
	
	for (i = 0; i < num_threads * num_iterations; i++)
    {
		char *rand_key_i = (char *)malloc(sizeof(char) * (KEY_LEN + 1));
		if (rand_key_i == NULL)
		{
			fprintf(stderr, "Failed to allocate memory for rand_key_i: %s\n", strerror(errno));
			exit(1);
		}
		bzero(rand_key_i, KEY_LEN + 1);
      
		for (j = 0; j < KEY_LEN; j++)
		strncat(rand_key_i, &chars[rand() % amt], 1);
      
		elements[i].key = rand_key_i;
		elements[i].next = NULL;
		elements[i].prev = NULL;
    }
	
	//Initialize mutexes.
	sublist_mutexes = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t) * num_lists);
	if (sublist_mutexes == NULL)
	{
		fprintf(stderr, "Failed to allocate memory for sublist_mutexes: %s\n", strerror(errno));
		exit(1);
	}
	for (i = 0; i < num_lists; i++)
		pthread_mutex_init(&sublist_mutexes[i], NULL);
	
	//Initialize spinlocks.
	sublist_spinlocks = (spinlock_t *)malloc(sizeof(spinlock_t) * num_lists);
	if (sublist_spinlocks == NULL)
	{
		fprintf(stderr, "Failed to allocate memory for sublist_spinlocks: %s\n", strerror(errno));
		exit(1);
	}
	for (i = 0; i < num_lists; i++)
		spinlock_init(&sublist_spinlocks[i]);
	
	//Initialize thread_lock_time.
	thread_lock_time = (long long *)malloc(sizeof(long long) * num_threads);
	if (thread_lock_time == NULL)
	{
		fprintf(stderr, "Failed to allocate memory for thread_lock_time: %s\n", strerror(errno));
		exit(1);
	}
	for (i = 0; i < num_threads; i++)
		thread_lock_time[i] = 0;
}

void set_test_name()
{
	//Setting test_name, initializing locks.
	test_name = (char*)malloc(TEST_NAME_LEN * sizeof(char));
	if (test_name == NULL)
	{
		fprintf(stderr, "Failed to allocate memory for test_name: %s", strerror(errno));
		exit(1);
	}
	bzero(test_name, TEST_NAME_LEN);
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
				strcat(test_name, "-m");
				break;
			case 's':
				strcat(test_name, "-s");
				break;
			default:
				fprintf(stderr, "weird argument to opt_sync\n");
				exit(1);
		}
	else
		strcat(test_name, "-none");
}