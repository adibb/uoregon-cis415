/* 
 * Name: Alex Dibb
 * Username: adibb
 * Assignment: CIS 415 Project 1
 * 
 * This is my own work, except for the 'p1fxns.h' library, which was 
 * provided by Professor Joe Sventek, and part of the main program
 * for waiting on child processes and syncing the 'ready' variable 
 * with semaphores, which are from StackOverflow.
 * 
 */

#include <stdlib.h>
#include <unistd.h> 
#include <stdio.h>
#include <sys/stat.h>   // For file access modes
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include "p1fxns.h"

// Globals EVERYWHERE I'M SO SICK OF RACE CONDITIONS HOLY SHIT
#define BUFFER_SIZE 512
#define UNUSED __attribute__((unused))
int n = 0;
int ready = 0;
char **lines;
int line_index;
pid_t parent_pid;
int i;   // Throwaway for loops

// Forward declarations
char **split(char *);

// Signal handlers
void onusr1(UNUSED int);

// Main program
int main(int argc, char *argv[]){
    int fd = 0;
    int quantum = -1;
    parent_pid = getpid();

    // Check for arguments
    for (i = 1; i < argc; i++){
        printf("%i\n", p1strchr(argv[i], '-'));
        if (p1strchr(argv[i], '-') == 0){
            int offset = p1strchr(argv[i], '=') + 1;
            quantum = p1atoi(argv[i]+offset);
            if (quantum == 0){
                p1perror(2, "Bad value for quantum");
                exit(EXIT_FAILURE);
            }
        } else {
            int temp;
            if ((temp = open(argv[i], O_RDONLY)) >= 0){
                fd = temp;
            } else {
                p1perror(2, "Could not open specified file");
                exit(EXIT_FAILURE);
            }
        }
    }

    // If quantum is -1 by now, then it wasn't set by user
    if (quantum == -1){
        char *temp;
        if ((temp = getenv("USPS_QUANTUM_MSEC")) == NULL){
            errno = EINVAL;
            p1perror(2, "Could not find env value for quantum");
            exit(EXIT_FAILURE);
        } else {
            quantum = p1atoi(temp);
        }
    }

    // !!! PARENT PROCESS - STARTUP !!!
    char buffer[BUFFER_SIZE];

    // Get the number of lines in the file
    if (fd != 0){
        while(p1getline(fd, buffer, BUFFER_SIZE) > 0){
            n++;
        }
        lseek(fd, 0, SEEK_SET);
    } else {
        p1getline(fd, buffer, BUFFER_SIZE);
        n = p1atoi(buffer);
    }
        
    // Malloc for the lines array
    lines = (char **) malloc(sizeof(char *) * n);

    // Read the file into the lines array
    int bytes_read;
    for (i = 0; i < n; i++){
        bytes_read = p1getline(fd, buffer, BUFFER_SIZE);
        lines[i] = (char *) malloc(bytes_read + 1); // for '/0'
        p1strcpy(lines[i], buffer);
    }

    // Done with the file, so close it if not stdin
    if (fd != 0)
        close(fd);

    // !!! PARENT PROCESS - FILE WAS READ !!!

    // Attach the signal handler for SIGUSR1
    if (signal(SIGUSR1, onusr1) ==  SIG_ERR){
        p1perror(2, "Could not attach SIGUSR1 handler");
        exit(EXIT_FAILURE);
    }

    // Fork each line into a new process
    pid_t pid[n];
    for (i = 0; i < n; i++){
        // Fork the process
        pid[i] = fork();

        // Check if the running process is the child
        if (pid[i] == 0){

            // !!! CHILD PROCESS CODE !!!
            line_index = i;
            //mx = sem_open(SEM_NAME, 0);
            kill(parent_pid, SIGUSR1);

        } else if (pid[i] < 0) {
            // Error on the fork attempt
            p1perror(2, "Could not fork");
            exit(EXIT_FAILURE);
        }
    }

    // !!! PARENT CODE WITH ALL CHILDREN STARTED AND ONLY STARTED !!!

    // Wait for all processes to be ready before starting them
    while (ready < n){
        sleep(1); // Sloppy, replace with something better later
    }

    // Start all processes
    for (i = 0; i < n; i++){
        kill(pid[i], SIGUSR1);
    }

    // Stop all processes
    for (i = 0; i < n; i++){
        kill(pid[i], SIGSTOP);
    }

    // Continue all processes
    for (i = 0; i < n; i++){
        kill(pid[i], SIGCONT);
    }

    // Wait for child processes to finish
    while (n > 0){
        (void) wait(NULL);
        n--;
    }

    // Free the lines array
    for (i = 0; i < n; i++){
        free(lines[i]);
    }
    free(lines);

    // Exit successfully.
    exit(EXIT_SUCCESS);
}

// Returns the contents of the line split by non-quoted whitespace,
// terminated with a NULL
char **split(char *line){
    // Find out how many words we're working with
    int i = 0, word_count = 0;
    char temp[BUFFER_SIZE];
    char **words;

    while (i >= 0){
        word_count++;
        i = p1getword(line, i, temp);
    }

    // Set up the 2D array for the words
    words = (char **) malloc(sizeof(char *) * (word_count + 1));
    int b_index = 0;
    for (i = 0; i < word_count; i++){
        // Read in word to excessively sized string
        char *w = (char *) malloc(BUFFER_SIZE);
        b_index = p1getword(line, b_index, w);

        // Get length, and prune newlines with it
        int wlen = p1strlen(w);
        if (w[wlen - 1] == '\n'){
            w[wlen - 1] = '\0';
            wlen--;
        }
        
        // Shrink allocated memory to what we actually need
        w = (char *) realloc(w, wlen);
        words[i] = w;
    }

    // Null-terminate it
    words[word_count] = NULL;

    return words;
}

// Signal handler for the SIGUSR1
void onusr1(UNUSED int signal){
    if (getpid() == parent_pid){
        ready++;
    } else if (getppid() == parent_pid) {
        // Split the line into words
        char **words = split(lines[line_index]);

        // Execute the command from the words
        execvp(words[0], words);

        // If we returned here, then starting the process failed
        p1perror(2, "Could not execute command");
        exit(EXIT_FAILURE);

    }
}
