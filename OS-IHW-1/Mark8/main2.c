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

int main() {
	// create fifo if not exists
    umask(0);
    mknod(FIFO_12, S_IFIFO|0666, 0);
    mknod(FIFO_21, S_IFIFO|0666, 0);
  	
  	// second process
    countCharacters(FIFO_12, FIFO_21);

    exit(0);
}
