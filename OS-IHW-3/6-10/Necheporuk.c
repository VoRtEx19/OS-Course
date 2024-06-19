#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define BUFFER_SIZE 256
#define CLIENT_ID 3

struct Task {
	int id;
	int status;
	int index;
};

int createClientSocket(char *server_ip, int server_port) {
    int client_socket;

    if ((client_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("Unable to create client socket");
        exit(-1);
    }

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(server_ip);
    server_address.sin_port = htons(server_port);

    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Unable to connect to server");
        exit(-1);
    }

    return client_socket;
}

void sendHandleRequest(int client_socket, struct Task task) {
    int status;
    int received;
    do {
        if (send(client_socket, &task, sizeof(task), MSG_NOSIGNAL) != sizeof(task)) {
            perror("Server closed connection...\n");
            exit(-1);
        }

        if ((received = recv(client_socket, &status, sizeof(int), MSG_NOSIGNAL)) != sizeof(int)) {
            perror("Server closed connection...\n");
            exit(-1);
        }
    } while (status != 1);

    if (task.status == 0) {
        printf("Necheporuk saw Petrov put item #%d in the truck\n", task.index + 1);
    }
    if (task.status == 1) {
        printf("Necheporuk estimated item #%d to be worth %d\n", task.index + 1, status);
    }
}

void work(int client_socket, int number) {
	struct Task task;
	task.id = CLIENT_ID;
	task.status = 0;
	srand(time(NULL));
	for (int i = 0; i < number; ++i) {
		sleep(rand() % 3 + 1);
		task.status = 0; // indicates taking item from storage
		task.index = i;
		sendHandleRequest(client_socket, task);
		sleep(rand() % 3 + 1);
		task.status = 1; // indicates giving item onward (to Petrov)
		sendHandleRequest(client_socket, task);
	}
	task.index = -1;
	task.status = 2; // indicates end of work
	sendHandleRequest(client_socket, task);
}

int main(int argc, char *argv[]) {
    int client_socket;
    unsigned short server_port;
    char *server_ip;
    char buffer[BUFFER_SIZE];
    int bytes_received, total_bytes_received;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <Server address> <Server port>\n", argv[0]);
        exit(1);
    }

    server_ip = argv[1];
    server_port = atoi(argv[2]);

    client_socket = createClientSocket(server_ip, server_port);

    int number;
    if ((bytes_received = recv(client_socket, &number, sizeof(number), 0)) != sizeof(number)) {
        perror("recv() bad");
        exit(-1);
    }

    work(client_socket, number);
    printf("Necheporuk's work is completed\n");

    close(client_socket);
    exit(0);
}
