/* 
 * Name: Alex Dibb
 * Username: adibb
 * Assignment: CIS 415 Project 1
 * 
 * This is my own work, except for the 'p1fxns.h' library, which was 
 * provided by Professor Joe Sventek, and part of the main program
 * for waiting on child processes, which is from StackOverflow.
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
#include "p1fxns.h"

// Globals
const int BUFFER_SIZE = 512;

// Forward declarations
char **split(char *);

// Main program
int main(int argc, char *argv[]){
    int i;
    int fd;
    int n = 0;
    char **lines;

    if (argc == 1){
        errno = EINVAL;
        p1perror(2, "No workload file specified");
        exit(EXIT_FAILURE);
    }

    // Open the workload file and read out the lines
    if ((fd = open(argv[argc - 1], O_RDONLY)) >= 0) {
        // File opened successfully
        char buffer[BUFFER_SIZE];

        // Get the number of lines in the file
        while(p1getline(fd, buffer, BUFFER_SIZE) > 0){
            n++;
        }
        
        // Malloc for the lines array
        lines = (char **) malloc(sizeof(char *) * n);

        // Return to the start of the file
        lseek(fd, 0, SEEK_SET);

        // Read the file into the lines array
        int bytes_read;
        for (i = 0; i < n; i++){
            bytes_read = p1getline(fd, buffer, BUFFER_SIZE);
            lines[i] = (char *) malloc(bytes_read + 1);
            p1strcpy(lines[i], buffer);
        }

        // Done with the file, so close it.
        close(fd);

    } else {
        // Workload file failed to open
        p1perror(2, "Could not open the workload text");
        exit(EXIT_FAILURE);
    }

    // Fork each line into a new process, freeing each line
    // as it goes
    pid_t pid[n];
    for (i = 0; i < n; i++){
        // Fork the proces
        pid[i] = fork();

        // Check if the running process is the child
        if (pid[i] == 0){
            // Split the line into words
            char **words = split(lines[i]);

            // Execute the command from the words
            execvp(words[0], words);

            // If we returned here, then starting the process failed
            p1perror(2, "Could not execute command");
            exit(EXIT_FAILURE);
        } else if (pid[i] < 0) {
            // Error on the fork attempt
            p1perror(2, "Could not fork");
            exit(EXIT_FAILURE);
        }

        // Free this line while we're here
        free(lines[i]);
    }

    // Free the lines array itself
    free(lines);

    // Wait for child processes to finish
    int signal;
    while (n > 0){
        wait(&signal);
        n--;
    }

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
