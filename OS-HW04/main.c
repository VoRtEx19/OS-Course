#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

const int size = 32;

int main(int argc, char *argv[]) {
    char input_path[] = "./input.txt";
    char output_path[] = "./output.txt";
    int fd_read, fd_write;
    char buffer[size];

    // getting file names

    printf("Hello, World!\n");
    if (argc == 1) {
        printf("Using default input = \"%s\" and output = \"%s\"\n", input_path, output_path);
    } else if (argc == 3) {
        strcpy(input_path, argv[1]);
        strcpy(output_path, argv[2]);
        printf("Using custom input = \"%s\" and output = \"%s\"\n", input_path, output_path);
    } else {
        printf("Incorrect number of arguments\n");
        exit(-1);
    }

    // fds

    if (strcmp(input_path, output_path) == 0) {
        printf("Paths are equivalent. My job is done.\n");
        exit(0);
    }

    ssize_t ssize;
    if ((fd_read = open(input_path, O_RDONLY)) < 0) {
        printf("Can\'t open file\n");
        exit(-1);
    }
    if ((fd_write = open(output_path, O_WRONLY | O_CREAT, 0666)) < 0) {
        printf("Can\'t open this file\n");
        exit(-1);
    }

    // read and write
    do {
        ssize = read(fd_read, buffer, size);
        if (ssize == -1) {
            printf("Can\'t write this file\n");
            exit(-1);
        }
        buffer[ssize] = '\0';
        printf("%s", buffer);
        size_t length = strlen(buffer);
        ssize = write(fd_write, buffer, length);
        if (ssize != length) {
            printf("Can\'t write all string");
            exit(-1);
        }
    } while (ssize == size);

    // close fds
    if (close(fd_read) < 0) {
        printf("Can\'t close file\n");
    }
    if (close(fd_write) < 0) {
        printf("Can\'t close file");
    }
    return 0;
}
