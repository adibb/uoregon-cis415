#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>

#define THREAD_COUNT 4

char *messages[] = {"Hello","World","Test","String",NULL};
int thread_count = THREAD_COUNT;
pthread_t pthreads[THREAD_COUNT];


pthread_mutex_t mutex_lock;


int ERROR_LOGGING_ON= 1;

int memory_size = 0;
volatile char *memory = "StartingMessage";

void set_message(char *message)
{
	memory = message;
}

char *get_message()
{
	char *returnValue = memory;
	return returnValue;
}

void log_time(FILE *file)
{

	// http://stackoverflow.com/questions/3756323/getting-the-current-time-in-milliseconds
    long            ms; // Milliseconds
    time_t          s;  // Second
    
struct timeval  tv;
gettimeofday(&tv, NULL);

double time_in_mill =    (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ; // convert tv_sec & tv_usec to millisecond


    fprintf(file,"%d", time_in_mill);
}

void Error(const char* format, ...)
{
    if(!ERROR_LOGGING_ON)
    {
        return;
    }
    va_list argptr;
    va_start(argptr, format);
    
    fprintf(stderr,"ERROR :");
    log_time(stderr);
    fprintf(stderr,"\n");
    vfprintf(stderr,format, argptr);
    fprintf(stderr,"\n");
    
    va_end(argptr);
    
}

void *pthread_proc(void *ptr)
{	
	char *message = (char*)ptr;
	
	if(NULL == message)
	{	
		printf("Thread started with invalid message.");
		return NULL;
	}
    
    pthread_cond_t p_cond = PTHREAD_COND_INITIALIZER;
    pthread_cond_init(&p_cond, NULL);

    
	int exit_count = 1000;
	while(exit_count-- > 0)
	{
		set_message(message);
		usleep(1);
		char *temp = get_message();
		if(strcmp(message,temp)==0)
		{
			//printf("Messages equal: %s\n",temp);
		}
		else
		{
			printf("Messages are NOT equal: %s, %s \n",message, temp);
		}
	}
	return NULL;
}

void create_threads()
{
	// create a thread passing 1 message to set / get, and print.

    pthread_mutex_init(&mutex_lock, NULL);

	int i = 0;
	for (i = 0; i < thread_count; i++)
	{
		pthread_create(&pthreads[i],NULL,pthread_proc,(void*)messages[i]);
		printf("Creating Thread: %d\n",pthreads[i]);
	}
}

void wait_for_threads_to_exit()
{
	// spin in a loop,
	void *returnValue = NULL;
	int i = 0;
	for (i = 0; i < thread_count; i++)
	{
		printf("Waiting for thread to join %d\n", pthreads[i]);
		pthread_join(pthreads[i],&returnValue);
	}
}

int main(int argc, char *argv[])
{
	log_time(stdout);
	//create_threads();
	
	//wait_for_threads_to_exit();
	return 0;
}
