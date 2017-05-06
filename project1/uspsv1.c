/* 
 * Name: Alex Dibb
 * Username: adibb
 * Assignment: CIS 415 Project 1
 * 
 * This is my own work, except for the 'p1fxns.h'
 * library, which was provided by Professor 
 * Joe Sventek.
 * 
 */

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "p1fxns.h"

// Globals
const int BUFFER_SIZE = 512;
int fd;

// Forward declarations
void cleanup();

// Main program
int main(){
    atexit(cleanup);

    if ((fd = open("./workload.txt", 0)) < 0) {
        exit(EXIT_FAILURE);
    } else {
        char buffer[BUFFER_SIZE];
        int bytes_read;

        while((bytes_read = p1getline(fd, buffer, BUFFER_SIZE)) > 0){
            write(1, buffer, bytes_read);
        }
    }

    exit(EXIT_SUCCESS);
}

// Function called on any exit
void cleanup(){
    close(fd);
}

