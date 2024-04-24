#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <linux/stat.h>

#define BUFFER_SIZE 5000
#define FIFO_12 "FIFO 1 to 2"
#define FIFO_21 "FIFO 2 to 1"

void readFile(const char *filename, const char* fifo_name) {
    int file_fd = open(filename, O_RDONLY);
    int fp;
    if (file_fd == -1) {
        perror("Error: Failed to open file for reading.\n");
        exit(1);
    }
    if((fp = open(fifo_name, O_WRONLY)) < 0){
		printf("Error: Can\'t open FIFO for writting from file 1\n");
        exit(-1);
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
        write(fp, buffer, bytes_read);
    }

    close(file_fd);
    close(fp);
}

void countCharacters(const char* fifo_read_name, const char* fifo_write_name) {
    char buffer[BUFFER_SIZE];
    int chars[128] = {0};
    
    int fp;
    
    if((fp = open(fifo_read_name, O_RDONLY)) < 0){
        printf("Error: Can\'t open FIFO for reading from file 1\n");
        exit(-1);
    }

    int bytes_read = read(fp, buffer, BUFFER_SIZE);
    for (int i = 0; i < bytes_read; ++i) {
        chars[(int) buffer[i]]++;
    }

    bytes_read = read(fp, buffer, BUFFER_SIZE);
    for (int i = 0; i < bytes_read; ++i) {
        chars[(int) buffer[i]]++;
    }
    close(fp);
    
    if((fp = open(fifo_write_name, O_WRONLY)) < 0){
        printf("Error: Can\'t open FIFO for writting from file 1\n");
        exit(-1);
    }

    char sequence[128] = {0};
    int count = 0;
    for (int i = 0; i < 128; ++i) {
        if (chars[i] > 0)
            sequence[count++] = (char) i;
    }

    write(fp, sequence, count);
    close(fp);
}

void writeFile(const char *filename, const char* fifo_name) {
    int file_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    int fp;
    
    if (file_fd == -1) {
        perror("Error: Failed to open file for writing.\n");
        exit(EXIT_FAILURE);
    }
    
    if((fp = open(fifo_name, O_RDONLY)) < 0){
        printf("Error: Can\'t open FIFO for reading from file 1\n");
        exit(-1);
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = read(fp, buffer, BUFFER_SIZE)) > 0) {
        write(file_fd, buffer, bytes_read);
    }

    close(file_fd);
    close(fp);
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
    
    // create fifo if not exists
    umask(0);
    mknod(FIFO_12, S_IFIFO|0666, 0);
    mknod(FIFO_21, S_IFIFO|0666, 0);
    
    // fork 1st process - file reader for 1st file
    pid_t pid = fork();
    if (pid == -1) {
        perror("Error: Fork of 1st file reader process failed.");
        exit(1);
    } else if (pid > 0) {
        // first process
        readFile(input1_filename, FIFO_12);
        readFile(input2_filename, FIFO_12);
        writeFile(output_filename, FIFO_21);
        exit(0);
    } else {
    	// second process
    	countCharacters(FIFO_12, FIFO_21);
    }
    

    // Wait for all child processes to finish
    waitpid(pid, NULL, 0);

    exit(0);
}
