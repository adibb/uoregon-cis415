#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>


#define THREAD_COUNT 4

char *messages[] = {"Hello","World","Test","String",NULL};
int thread_count = THREAD_COUNT;
pthread_t pthreads[THREAD_COUNT]; 

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

void *pthread_proc(void *ptr)
{	
	char *message = (char*)ptr;
	
	// check message... ?
	// print the message, and thread id.
    printf("Got %s in thread %i;\n", message, pthread_self());

	int exit_count = 1000;
	while(exit_count-- > 0)
	{
		// set the message.
        set_message(message);
        
        usleep(2);
        
		// get the message.
        char *new_message = get_message();
        
        usleep(2);
        
        // print the messages, and thread id. if they are different.
        if (strcmp(message, new_message) != 0){
            printf("Got mismatch in thread %i: %s vs. %s\n", pthread_self(), message, new_message);
	
        }
    }
	return NULL;
}

void create_threads()
{
	// create a thread passing 1 message each.
	// for each:  print the thread id and the message passed. 
    int i;
    for (i = 0; i < THREAD_COUNT; i++){
        pthread_create(&pthreads[i], NULL, pthread_proc, messages[i]);
    }

}

void wait_for_threads_to_exit()
{
	// spin in a loop, wait for each thread to exit.
    int i;
    for (i = 0; i < THREAD_COUNT; i++){
        pthread_join(pthreads[i], NULL);
    }
}

int main(int argc, char *argv[])
{
	create_threads();
	
	wait_for_threads_to_exit();
	return 0;
}
