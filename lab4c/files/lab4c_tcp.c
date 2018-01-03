#include <getopt.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <strings.h>
#include <unistd.h>
#include "mraa.h" //Note: use -lmraa to compile
#include <math.h>
#include <time.h>
#include <string.h>
#include <poll.h>
#include <errno.h>

void print_usage(const char *exec_name);
void report_sample(struct timespec ts, float temp);
float convert_into_temp(uint16_t a);
long long get_time_diff_ns(struct timespec a, struct timespec b);
void print_shutdown_msg();

struct option long_opts[] = {
	{"id", required_argument, NULL, 'i'},
	{"host", required_argument, NULL, 'h'},
	{"log", required_argument, NULL, 'l'},
	{0, 0, 0, 0}
};

const int EXIT_SUCC = 0;
const int EXIT_INVALID_PARAM = 1;
const int EXIT_OTHER = 2;

int ID_NUM;
uint16_t PORT_NUM;
char *SERV_NAME;
int sock_fd;

const int PIN_TEMP_SENSOR = 0;
const int PIN_BUTTON = 3;
const long long NS_PER_S = 1000000000;
long long sample_int = 1000000000;
char temp_scale = 'F';
int report_enable = 1;
const float B = 4275;
const float R0 = 100000;

int main(int argc, char *argv[])
{
	ssize_t rc;
	
	int opt;
	while((opt = getopt_long(argc, argv, "", long_opts, NULL)) != -1) {
		switch(opt) {
		case 'i':
			ID_NUM = atoi(optarg);
			break;
		case 'h':
			SERV_NAME = (char *)optarg;
			break;
		case 'l':
			;
			int fd = creat(optarg, 00666); //syscall -1
			dup2(fd, STDOUT_FILENO);
			break;
		default:
			print_usage(argv[0]);
			exit(EXIT_INVALID_PARAM);
		}
	}
	
	if (optind >= argc) {
		fprintf(stderr, "Expected port number argument\n");
		exit(EXIT_INVALID_PARAM);
	}
	PORT_NUM = atoi(argv[optind]);
	
	//Set up the socket.
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	
	struct sockaddr_in serv_addr;
	struct hostent *host = gethostbyname(SERV_NAME);

	bzero((char*)&serv_addr, sizeof(serv_addr));
	bcopy((char*)host->h_addr, (char*)&serv_addr.sin_addr.s_addr,
		host->h_length);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT_NUM);

	rc = connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	if (rc == -1) {
		fprintf(stderr, "Error connecting to socket\n");
		exit(EXIT_OTHER);	
	}
	
	printf("ID=%d\n", ID_NUM);
	dprintf(sock_fd, "ID=%d\n", ID_NUM);
	
	//Declare and initialize temperature sensor.
	mraa_aio_context temp_sensor;
	uint16_t temp_sensor_input;
	temp_sensor = mraa_aio_init(PIN_TEMP_SENSOR);
	if (temp_sensor == NULL)
	{
		fprintf(stderr, "Unable to initialize analog input device: A%d\n", PIN_TEMP_SENSOR);
		exit(EXIT_OTHER);
	}
	float temp_value;
	
	//Declare, initialize, and set direction of button input.
	mraa_gpio_context button;
	button = mraa_gpio_init(PIN_BUTTON);
	if (button == NULL)
	{
		fprintf(stderr, "Unable to initialize button device: D%d\n", PIN_BUTTON);
		exit(EXIT_OTHER);
	}
	mraa_result_t rc_mraa = mraa_gpio_dir(button, MRAA_GPIO_IN);
	if (rc_mraa != MRAA_SUCCESS)
	{
		fprintf(stderr, "Unable to set button direction: D%d\n", PIN_BUTTON);
		exit(EXIT_OTHER);
	}
	
	//Declare and initialize time-keeping mechanisms.
	struct timespec ts_curr;
	struct timespec ts_prev;
	rc = clock_gettime(CLOCK_REALTIME, &ts_curr);
	if (rc == -1)
	{
		fprintf(stderr, "Failed to get time: %s\n", strerror(errno));
		exit(EXIT_OTHER);
	}
	ts_prev.tv_sec = 0;
	ts_prev.tv_nsec = 0;
	
	//Poll on readable data from socket.
	struct pollfd fds[1];
	fds[0].fd = sock_fd;
	fds[0].events = POLLIN;
	int poll_ret;
	
	//Buffer to read in commands from stdin.
	const int BUF_SIZE = 100;
	char *command_buf = (char *)malloc(BUF_SIZE * sizeof(char));
	if (command_buf == NULL)
	{
		fprintf(stderr, "Failed to allocate memory for stdin buffer: %s\n", strerror(errno));
		exit(EXIT_OTHER);
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
			exit(EXIT_OTHER);
		}
	
		if (get_time_diff_ns(ts_prev, ts_curr) >= sample_int)
		{	
			temp_sensor_input = mraa_aio_read(temp_sensor);
			if (temp_sensor_input == -1)
			{
				fprintf(stderr, "Error while reading temperature sensor input\n");
				exit(EXIT_OTHER);
			}
			temp_value = convert_into_temp(temp_sensor_input);
			ts_prev = ts_curr;
			if (report_enable)
				report_sample(ts_curr, temp_value);
			ts_prev = ts_curr;
		}

		poll_ret = poll(fds, 1, 0);
		if (poll_ret == -1)
		{
			fprintf(stderr, "Error during poll system call: %s\n", strerror(errno));
			exit(EXIT_OTHER);
		}
		else if (poll_ret > 0)
		{
			if (fds[0].revents & POLLIN)
			{
				rc = read(sock_fd, command_buf, BUF_SIZE - 1);
				if (rc == 0)
					break;
				else if (rc == -1)
				{
					fprintf(stderr, "Error while reading from socket: %s\n", strerror(errno));
					exit(EXIT_OTHER);
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
					sample_int = (long long)strtol(command_buf + 7, &endchar, 10) * NS_PER_S;
					if (strncmp(endchar, "\n", 1) != 0)
					{
						fprintf(stderr, "Unrecognized command: %s", command_buf);
						exit(EXIT_OTHER);
					}
					//fprintf(stderr, "Sample interval (ns) is now: %lli\n", sample_int);
				}
				else
				{
					fprintf(stderr, "Unrecognized command: %s", command_buf);
					exit(EXIT_OTHER);
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
		exit(EXIT_OTHER);
	}
	rc_mraa = mraa_gpio_close(button);
	if (rc_mraa != MRAA_SUCCESS)
	{
		fprintf(stderr, "Failed to close button input context: D%d\n", PIN_BUTTON);
		exit(EXIT_OTHER);
	}
	free(command_buf);
	exit(EXIT_SUCC);
}

void print_usage(const char *exec_name)
{
  fprintf (stderr,
	"usage: %s [--id=number] [--host=hostname] [--log=filename] PORTNUM\n"
	"\t--id=number:\t\tint number is the 9-digit ID\n"
	"\t--host=hostname:\thostname is the name or address of the host\n"
	"\t--log=filename:\t\tfilename is the file to log output to\n"
	"\tPORTNUM:\t\tPORTNUM is the port number\n",
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
			temp = 9.0/5.0 * temp + 32.0;
			break;
		case 'C':
			break;
		default:
			fprintf(stderr, "Unrecognized temperature scale.\n");
			exit(EXIT_OTHER);
	}
	
	printf("%s %04.1f\n", time_str, temp);
	dprintf(sock_fd, "%s %04.1f\n", time_str, temp);
	return;
}

float convert_into_temp(uint16_t a)
{
	float R = 1023.0/(float)a - 1.0;
	R = R0*R;
	
	float temp = 1.0/((float)log(R/R0)/B + 1.0/298.15) - 273.15;
	
	return temp;
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
	dprintf(sock_fd, "%s SHUTDOWN\n", time_str);
	return;
}