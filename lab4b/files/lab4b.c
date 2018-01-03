#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include "mraa.h" //Note: use -lmraa to compile
#include <math.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <poll.h>
#include <errno.h>

void print_usage(const char *exec_name);
void report_sample(struct timespec ts, float temp);
float convert_into_temp(int ts_input);
long long get_time_diff_ns(struct timespec a, struct timespec b);
void print_shutdown_msg();

struct option long_opts[] = {
	{"log", required_argument, NULL, 'l'},
	{"scale", required_argument, NULL, 's'},
	{"period", required_argument, NULL, 'p'},
	{0, 0, 0, 0}
};

//Constants.
const int PIN_TEMP_SENSOR = 0;
const int PIN_BUTTON = 3;
const long long NS_PER_S = 1000000000;

//Run-time behavior variables.
volatile long long sample_interval = 1000000000;
volatile char temp_scale = 'F';

//Run-time flags.
volatile int report_enable = 1;

//Temperature calibration constants.
const int B = 4275;
const int R0 = 100000;

int main(int argc, char *argv[])
{	
	int opt;
	while((opt = getopt_long(argc, argv, "", long_opts, NULL)) != -1)
    {
		switch(opt)
		{
		case 'p':
			sample_interval = (long long)atoi(optarg) * NS_PER_S;
			break;
		case 's':
			temp_scale = optarg[0];
			break;
		case 'l':
			;
			int fd = creat(optarg, 00666);
			if (fd == -1)
			{
				fprintf(stderr, "Failed to create or open output file: %s\n", strerror(errno));
				exit(1);
			}
			dup2(fd, STDOUT_FILENO);
			break;
		default:
			print_usage(argv[0]);
			exit(1);
		}
	}
	
	//Declare and initialize temperature sensor.
	mraa_aio_context temp_sensor;
	volatile int temp_sensor_input = 0;
	temp_sensor = mraa_aio_init(PIN_TEMP_SENSOR);
	if (temp_sensor == NULL)
	{
		fprintf(stderr, "Unable to initialize analog input device: A%d\n", PIN_TEMP_SENSOR);
		exit(1);
	}
	float temp_value;
	
	//Declare, initialize, and set direction of button input.
	mraa_gpio_context button;
	button = mraa_gpio_init(PIN_BUTTON);
	if (button == NULL)
	{
		fprintf(stderr, "Unable to initialize button device: D%d\n", PIN_BUTTON);
		exit(1);
	}
	mraa_result_t rc_mraa = mraa_gpio_dir(button, MRAA_GPIO_IN);
	if (rc_mraa != MRAA_SUCCESS)
	{
		fprintf(stderr, "Unable to set button direction: D%d\n", PIN_BUTTON);
		exit(1);
	}
	
	//Declare and initialize time-keeping mechanisms.
	struct timespec ts_curr;
	struct timespec ts_prev;
	int rc = clock_gettime(CLOCK_REALTIME, &ts_curr);
	if (rc == -1)
	{
		fprintf(stderr, "Failed to get time: %s\n", strerror(errno));
		exit(1);
	}
	ts_prev.tv_sec = 0;
	ts_prev.tv_nsec = 0;
	
	//Poll on readable data from stdin.
	struct pollfd fds[1];
	fds[0].fd = STDIN_FILENO;
	fds[0].events = POLLIN;
	int poll_ret;
	
	//Buffer to read in commands from stdin.
	const int BUF_SIZE = 100;
	char *command_buf = (char *)malloc(BUF_SIZE * sizeof(char));
	if (command_buf == NULL)
	{
		fprintf(stderr, "Failed to allocate memory for stdin buffer: %s\n", strerror(errno));
		exit(1);
	}
	bzero(command_buf, BUF_SIZE);
	
	//Loop to read temperature and process stdin commands.
	//Note: this should check if the button is pressed at a higher rate than 1 time per sec.
	while(mraa_gpio_read(button) != 1)
	{	
		rc = clock_gettime(CLOCK_REALTIME, &ts_curr);
		if (rc == -1)
		{
			fprintf(stderr, "Failed to get time: %s\n", strerror(errno));
			exit(1);
		}
	
		if (get_time_diff_ns(ts_prev, ts_curr) >= sample_interval)
		{
			//fprintf(stderr, "%lli > %lli: reporting current temp\n", get_time_diff_ns(ts_prev, ts_curr), sample_interval);
			temp_sensor_input = mraa_aio_read(temp_sensor);
			if (temp_sensor_input == -1)
			{
				fprintf(stderr, "Error while reading temperature sensor input\n");
				exit(1);
			}
			temp_value = convert_into_temp(temp_sensor_input);
			if (report_enable)
				report_sample(ts_curr, temp_value);
			ts_prev = ts_curr;
		}

		poll_ret = poll(fds, 1, 0);
		if (poll_ret == -1)
		{
			fprintf(stderr, "Error during poll system call: %s\n", strerror(errno));
			exit(1);
		}
		else if (poll_ret > 0)
		{
			if (fds[0].revents & POLLIN)
			{
				rc = read(STDIN_FILENO, command_buf, BUF_SIZE - 1);
				if (rc == 0)
					break;
				else if (rc == -1)
				{
					fprintf(stderr, "Error while reading from stdin: %s\n", strerror(errno));
					exit(1);
				}
				
				if (strncmp(command_buf, "OFF\n", 4) == 0)
				{
					printf("%s", command_buf);
					break;
				}
				else if (strncmp(command_buf, "STOP\n", 5) == 0)
					report_enable = 0;
				else if (strncmp(command_buf, "START\n", 6) == 0)
					report_enable = 1;
				else if (strncmp(command_buf, "SCALE=F\n", 8) == 0)
					temp_scale = 'F';
				else if (strncmp(command_buf, "SCALE=C\n", 8) == 0)
					temp_scale = 'C';
				else if (strncmp(command_buf, "PERIOD=", 7) == 0)
				{
					char *endchar; 
					sample_interval = (long long)strtol(command_buf + 7, &endchar, 10) * NS_PER_S;
					if (strncmp(endchar, "\n", 1) != 0)
					{
						fprintf(stderr, "Unrecognized command: %s", command_buf);
						exit(1);
					}
					//fprintf(stderr, "Sample interval (ns) is now: %lli\n", sample_interval);
				}
				else
				{
					fprintf(stderr, "Unrecognized command: %s", command_buf);
					exit(1);
				}
				printf("%s", command_buf);
				bzero(command_buf, BUF_SIZE);
			}
		}
	}
	
	print_shutdown_msg();
	rc_mraa = mraa_aio_close(temp_sensor);
	if (rc_mraa != MRAA_SUCCESS)
	{
		fprintf(stderr, "Failed to close temperature sensor input context: A%d\n", PIN_TEMP_SENSOR);
		exit(1);
	}
	rc_mraa = mraa_gpio_close(button);
	if (rc_mraa != MRAA_SUCCESS)
	{
		fprintf(stderr, "Failed to close button input context: D%d\n", PIN_BUTTON);
		exit(1);
	}
	free(command_buf);
	exit(0);
}

void print_usage(const char *exec_name)
{
  fprintf (stderr,
	"usage: %s [--period=p] [--scale=s] [--log=filename]\n"
	"\t--period=p:\tint p is the sampling interval in seconds\n"
	"\t\t\t(default is 1)\n"
	"\t--scale=s:\tchar s is the temperature scale\n"
	"\t\t\t(default is 'F')\n"
	"\t--log=filename:\tchar* filename is the logfile\n",
	exec_name);
  return;
}

//Reports the sample with temperature specified by 'temp' (in Celsius)
//at given time specified by 'ts'.
void report_sample(struct timespec ts, float temp)
{
	time_t t = ts.tv_sec;
	struct tm * t_info;
	char time_str[9];
	
	t_info = localtime(&t);
	
	strftime(time_str, sizeof(time_str), "%H:%M:%S", t_info);
	
	switch(temp_scale)
	{
		case 'F':
			temp = 9/5 * temp + 32;
			break;
		case 'C':
			break;
		default:
			fprintf(stderr, "Unrecognized temperature scale.\n");
			exit(1);
	}
	
	printf("%s %04.1f\n", time_str, temp);
	return;
}

float convert_into_temp(int temp_sensor_input)
{
	float R = 1023.0/temp_sensor_input - 1.0;
	R = R0*R;
	
	float temp_value = 1.0/(log(R/R0)/B + 1/298.15) - 273.15;
	
	return temp_value;
}

long long get_time_diff_ns(struct timespec a, struct timespec b)
{
  return ((long long)b.tv_sec - (long long)a.tv_sec) * NS_PER_S
		     + ((long long)b.tv_nsec - (long long)a.tv_nsec);
}

void print_shutdown_msg()
{
	time_t t;
	struct tm * t_info;
	char time_str[9];
	
	time(&t);
	t_info = localtime(&t);
	
	strftime(time_str, sizeof(time_str), "%H:%M:%S", t_info);
	
	printf("%s SHUTDOWN\n", time_str);
	return;
}