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
    close(pipe_fd);
}

void countCharacters(int pipe_read_fd1, int pipe_read_fd2, int pipe_write_fd) {
    char buffer[BUFFER_SIZE];
    int chars[128] = {0};

    int bytes_read = read(pipe_read_fd1, buffer, BUFFER_SIZE);
    for (int i = 0; i < bytes_read; ++i) {
        chars[(int) buffer[i]]++;
    }
    close(pipe_read_fd1);

    bytes_read = read(pipe_read_fd2, buffer, BUFFER_SIZE);
    for (int i = 0; i < bytes_read; ++i) {
        chars[(int) buffer[i]]++;
    }
    close(pipe_read_fd2);

    char sequence[128] = {0};
    int count = 0;
    for (int i = 0; i < 128; ++i) {
        if (chars[i] > 0)
            sequence[count++] = (char) i;
    }

    write(pipe_write_fd, sequence, count);
    close(pipe_write_fd);
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
    close(pipe_fd);
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
    int pipe_in1[2], pipe_in2[2], pipe_out[2];
    
    // create pipes
    if (pipe(pipe_in1) == -1 || pipe(pipe_in2) == -1 || pipe(pipe_out) == -1) {
        perror("Error: Pipe creation failed.\n");
        exit(1);
    }
    
    // fork 1st process - file reader for 1st file
    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("Error: Fork of 1st file reader process failed.");
        exit(1);
    } else if (pid1 == 0) {
        // read from file 1
        close(pipe_in1[0]);
        readFile(input1_filename, pipe_in1[1]);
        exit(0);
    }
    
    // fork 2nd process - file reader for 2nd file
    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("Error: Fork of 2nd file reader process failed.");
        exit(1);
    } else if (pid2 == 0) {
        // read from file 2
        close(pipe_in2[0]);
        readFile(input2_filename, pipe_in2[1]);
        exit(0);
    }

    // fork 3rd process - do main idea
    pid_t pid3 = fork();
    if (pid3 == -1) {
        perror("Error: Fork of symbol counter process failed.\n");
        exit(1);
    } else if (pid3 == 0) {
        // Child process 2
        close(pipe_in1[1]); // Close write end of pipe1
        close(pipe_in2[1]); // Close write end of pipe2
        close(pipe_out[0]); // Close read end of pipe3
        countCharacters(pipe_in1[0], pipe_in2[0], pipe_out[1]);
        exit(0);
    }

    pid_t pid4 = fork();
    if (pid4 == -1) {
        perror("Error: Fork of file writer failed failed.\n");
        exit(1);
    } else if (pid4 == 0) {
        // Child process 3
        close(pipe_out[1]); // Close read end of pipe2
        writeFile(output_filename, pipe_out[0]);
        exit(0);
    }

    close(pipe_in1[0]); // Close read end of pipe1
    close(pipe_in1[1]); // Close write end of pipe1
    close(pipe_in2[0]); // Close write end of pipe1
    close(pipe_in2[1]); // Close write end of pipe1
    close(pipe_out[0]); // Close read end of pipe2
    close(pipe_out[1]); // Close write end of pipe2

    // Wait for all child processes to finish
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    waitpid(pid3, NULL, 0);
    waitpid(pid4, NULL, 0);

    exit(0);
}
