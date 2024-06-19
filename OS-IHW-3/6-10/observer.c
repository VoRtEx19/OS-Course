#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 256

int client_socket;

void sigint_handler(int signum) {
    printf("Observer stopped\n");
    close(client_socket);
    exit(0);
}

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

int main(int argc, char *argv[]) {
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

    while (1) {
        char buffer[1024];
        if ((bytes_received = recv(client_socket, buffer, sizeof(buffer), 0)) < 0) {
            perror("recv() bad");
            close(client_socket);
            exit(-1);
        }

        if (bytes_received == sizeof(int)) {
            printf("Server closed the connection...\n");
            close(client_socket);
            exit(0);
        }

        buffer[bytes_received] = '\0';
        printf("%s", buffer);
    }

    close(client_socket);
    exit(0);
}
