#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>

#define WAITING_TIME 1000
#define BUFFER_SIZE 256
#define CLIENT_ID 3

struct Task {
    int id;
    int index;
    int status;
};

struct sockaddr_in getServerAddress(char *server_ip, int server_port) {
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(server_ip);
    server_address.sin_port = htons(server_port);

    return server_address;
}

int createClientSocket(char *server_ip, int server_port) {
    int client_socket;

    if ((client_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("Unable to create client socket");
        exit(-1);
    }

    struct sockaddr_in server_address = getServerAddress(server_ip, server_port);

    // TODO
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Unable to connect");
        exit(-1);
    }

    return client_socket;
}

int trySend(int client_socket, void *buffer, int size, int suspend_time) {
    if (send(client_socket, buffer, size, 0) != size) {
        perror("send() bad");
        exit(-1);
    }

    struct pollfd fd;
    fd.fd = client_socket;
    fd.events = POLLIN;

    int attempts = 0;
    int wait_result = poll(&fd, 1, suspend_time + WAITING_TIME);

    while (wait_result <= 0 && attempts < 5) {
        ++attempts;
        printf("Can't receive answer from server. Attempt %d/5\n", attempts);

        if (send(client_socket, buffer, size, 0) != size) {
            perror("send() bad");
            exit(-1);
        }

        wait_result = poll(&fd, 1, suspend_time + WAITING_TIME);
    }

    if (attempts == 5) {
        return -1;
    }
    return attempts;
}

void sendHandleRequest(int client_socket, struct Task task) {
    int status;
    int received;

    if (trySend(client_socket, &task, sizeof(task), 10000) < 0) {
        printf("Server error\n");
        exit(0);
    }

    if ((received = recv(client_socket, &status, sizeof(int), 0)) != sizeof(int)) {
        perror("recv() bad");
        exit(-1);
    }

    if (task.status == 0) {
        printf("Necheporuk saw Petrov put item #%d into the truck\n", task.index + 1);
    }
    if (task.status == 1) {
        printf("Necheporuk evaluated item #%d\n", task.index + 1);
    }
}

void work(int client_socket, int number) {
    struct Task task;
    task.id = CLIENT_ID;
    
    for (int i = 0; i < number; ++i) {
    	sleep(rand() % 3 + 1);
		task.status = 0; // indicates taking item from storage
		task.index = i;
		sendHandleRequest(client_socket, task);
		sleep(rand() % 3 + 1);
		task.status = 1; // indicates giving item onward (to Petrov)
		sendHandleRequest(client_socket, task);
    }

    task.status = 3;
    sendHandleRequest(client_socket, task);
}

int main(int argc, char *argv[]) {
    int client_socket;
    unsigned short server_port;
    int working_time;
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

    struct pollfd fd;
    fd.fd = client_socket;
    fd.events = POLLIN;

    struct Task task;
    task.id = CLIENT_ID;
    task.status = 2;

    if (trySend(client_socket, &task, sizeof(task), 0) < 0) {
        printf("Server error\n");
        exit(0);
    }

    int number;
    if ((bytes_received = recv(client_socket, &number, sizeof(number), 0)) != sizeof(number)) {
        perror("recv() bad");
        exit(-1);
    }

    work(client_socket, number);
    printf("The work is completed\n");

    close(client_socket);
    exit(0);
}
