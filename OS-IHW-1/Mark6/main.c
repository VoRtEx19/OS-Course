#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define BUFFER_SIZE 5000

void readFile(const char *filename, int pipe_fd) {
    int file_fd = open(filename, O_RDONLY);
    if (file_fd == -1) {
        perror("Error: Failed to open file for reading.\n");
        exit(1);
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
        write(pipe_fd, buffer, bytes_read);
    }

    close(file_fd);
}

void countCharacters(int pipe_read_fd, int pipe_write_fd) {
    char buffer[BUFFER_SIZE];
    int chars[128] = {0};
    
    int bytes_read = read(pipe_read_fd, buffer, BUFFER_SIZE);
    for (int i = 0; i < bytes_read; ++i) {
        chars[(int) buffer[i]]++;
    }

    bytes_read = read(pipe_read_fd, buffer, BUFFER_SIZE);
    for (int i = 0; i < bytes_read; ++i) {
        chars[(int) buffer[i]]++;
    }

    char sequence[128] = {0};
    int count = 0;
    for (int i = 0; i < 128; ++i) {
        if (chars[i] > 0)
            sequence[count++] = (char) i;
    }

    write(pipe_write_fd, sequence, count);
}

void writeFile(const char *filename, int pipe_fd) {
    int file_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (file_fd == -1) {
        perror("Error: Failed to open file for writing.\n");
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = read(pipe_fd, buffer, BUFFER_SIZE)) > 0) {
        write(file_fd, buffer, bytes_read);
    }

    close(file_fd);
}

int main(int argc, char *argv[]) {
	// check if there is 4 arguments
	if (argc != 4) {
		perror("Error: Insufficient amount of arguments");
		exit(1);
	}
    
    // filenames
    const char *input1_filename = argv[1];
    const char *input2_filename = argv[2];
    const char *output_filename = argv[3];

    // pipes
    int pipe_12[2]; // first to second
    int pipe_21[2]; // second to first
    
    // create pipes
    if (pipe(pipe_12) == -1 || pipe(pipe_21) == -1) {
        perror("Error: Pipe creation failed.\n");
        exit(1);
    }
    
    // fork 1st process - file reader for 1st file
    pid_t pid = fork();
    if (pid == -1) {
        perror("Error: Fork of 1st file reader process failed.");
        exit(1);
    } else if (pid > 0) {
        // first process
        close(pipe_12[0]);
        readFile(input1_filename, pipe_12[1]);
        readFile(input2_filename, pipe_12[1]);
        close(pipe_12[1]);
        close(pipe_21[1]);
        writeFile(output_filename, pipe_21[0]);
        close(pipe_21[0]);
        exit(0);
    } else {
    	// second process
    	close(pipe_12[1]);
    	close(pipe_21[0]);
    	countCharacters(pipe_12[0], pipe_21[1]);
    	close(pipe_12[0]);
    	close(pipe_21[1]);
    }

    close(pipe_12[0]); // Close read end of pipe
    close(pipe_12[1]); // Close write end of pipe
    close(pipe_21[0]); // Close read end of pipe
    close(pipe_21[1]); // Close write end of pipe

    // Wait for all child processes to finish
    waitpid(pid, NULL, 0);

    exit(0);
}
