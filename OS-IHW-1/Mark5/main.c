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
#define FIFO_IN1 "FIFO IN 1"
#define FIFO_IN2 "FIFO IN 2"
#define FIFO_OUT "FIFO OUT"

void readFile(const char *filename, int fp) {
    int file_fd = open(filename, O_RDONLY);
    if (file_fd == -1) {
        perror("Error: Failed to open file for reading.\n");
        exit(1);
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
        write(fp, buffer, bytes_read);
    }

    close(file_fd);
    close(fp);
}

void countCharacters(int fp1, int fp2, int fp) {
    char buffer[BUFFER_SIZE];
    int chars[128] = {0};

    int bytes_read = read(fp1, buffer, BUFFER_SIZE);
    for (int i = 0; i < bytes_read; ++i) {
        chars[(int) buffer[i]]++;
    }
    close(fp1);

    bytes_read = read(fp2, buffer, BUFFER_SIZE);
    for (int i = 0; i < bytes_read; ++i) {
        chars[(int) buffer[i]]++;
    }
    close(fp2);

    char sequence[128] = {0};
    int count = 0;
    for (int i = 0; i < 128; ++i) {
        if (chars[i] > 0)
            sequence[count++] = (char) i;
    }

    write(fp, sequence, count);
    close(fp);
}

void writeFile(const char *filename, int fp) {
    int file_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (file_fd == -1) {
        perror("Error: Failed to open file for writing.\n");
        exit(EXIT_FAILURE);
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

    // named channel - fifo
    int fp;
    
    // create fifo if not exists
    umask(0);
    mknod(FIFO_IN1, S_IFIFO|0666, 0);
    mknod(FIFO_IN2, S_IFIFO|0666, 0);
    mknod(FIFO_OUT, S_IFIFO|0666, 0);
    
    // fork 1st process - file reader for 1st file
    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("Error: Fork of 1st file reader process failed.");
        exit(1);
    } else if (pid1 == 0) {
        // read from file 1
        if((fp = open(FIFO_IN1, O_WRONLY)) < 0){
        	printf("Error: Can\'t open FIFO for writting from file 1\n");
        	exit(-1);
      	}
        readFile(input1_filename, fp);
        close(fp);
        exit(0);
    }
    
    // fork 2nd process - file reader for 2nd file
    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("Error: Fork of 2nd file reader process failed.");
        exit(1);
    } else if (pid2 == 0) {
        // read from file 2
        if((fp = open(FIFO_IN2, O_WRONLY)) < 0){
        	printf("Error: Can\'t open FIFO for writting from file 2\n");
        	exit(-1);
      	}
        readFile(input2_filename, fp);
        close(fp);
        exit(0);
    }

    // fork 3rd process - do main idea
    pid_t pid3 = fork();
    if (pid3 == -1) {
        perror("Error: Fork of symbol counter process failed.\n");
        exit(1);
    } else if (pid3 == 0) {
        // Child process 3
        int fp1;
        int fp2;
        if(((fp1 = open(FIFO_IN1, O_RDONLY)) < 0) || ((fp2 = open(FIFO_IN2, O_RDONLY)) < 0) || ((fp = open(FIFO_OUT, O_WRONLY)) < 0)){
        	printf("Error: Can\'t open FIFO\n");
        	exit(-1);
      	}
        countCharacters(fp1, fp2, fp);
        close(fp1);
        close(fp2);
        close(fp);
        exit(0);
    }

    pid_t pid4 = fork();
    if (pid4 == -1) {
        perror("Error: Fork of file writer failed failed.\n");
        exit(1);
    } else if (pid4 == 0) {
        // Child process 4
        if ((fp = open(FIFO_OUT, O_RDONLY)) < 0) {
        	printf("Error: Can\'t open FIFO for reading to output\n");
        	exit(-1);
        }
        writeFile(output_filename, fp);
        close(fp);
        exit(0);
    }

    // Wait for all child processes to finish
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    waitpid(pid3, NULL, 0);
    waitpid(pid4, NULL, 0);

    exit(0);
}
