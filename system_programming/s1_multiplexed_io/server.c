#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#define MAX_EVENTS 10

char protocol[4];
int port;

void parse_arguments(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s --protocol=<protocol> --port=<port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];
        char *value = strchr(arg, '=');
        if (value) {
            value++;
            if (strncmp(arg, "--protocol=", 11) == 0) {
                snprintf(protocol, sizeof(protocol), "%s", value);
            }
            else if (strncmp(arg, "--port=", 7) == 0) {
                port = atoi(value);
            }
            else {
                fprintf(stderr, "Unknown argument: %s\n", arg);
                exit(EXIT_FAILURE);
            }
        }
    }
}

int main (int argc, char *argv[])
{
    int server_fd;
    struct sockaddr_in addr;
    parse_arguments(argc, argv);
    if (strcmp(protocol, "tcp") == 0) {
        printf("Starting TCP server on port %d\n", port);
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        addr = (struct sockaddr_in) {
            .sin_family = AF_INET,
            .sin_port = htons(port),
            .sin_addr = { htonl(INADDR_ANY) }
        };
        if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            perror("bind");
            exit(EXIT_FAILURE);
        }
        listen(server_fd, 10);
        fcntl(server_fd, F_SETFL, fcntl(server_fd, F_GETFL) | O_NONBLOCK);
    } 
    else if (strcmp(protocol, "udp") == 0) {
        printf("Starting UDP server on port %d\n", port);
        server_fd = socket(AF_INET, SOCK_DGRAM, 0);
        addr = (struct sockaddr_in) {
            .sin_family = AF_INET,
            .sin_port = htons(port),
            .sin_addr = { htonl(INADDR_ANY) }
        };

        if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            perror("bind");
            exit(EXIT_FAILURE);
        }
    }
    int epfd = epoll_create1(0);
    struct epoll_event server_event = {
        .events = EPOLLIN,
        .data.fd = server_fd
    };
    struct epoll_event events[MAX_EVENTS];
    epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &server_event);

    char *response_msg = "Received your message!\n";
    while (1) {
        int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == server_fd) {
                if (strcmp(protocol, "tcp") == 0) {
                    int client_fd = accept(server_fd, NULL, NULL);
                    fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL) | O_NONBLOCK);
                    struct epoll_event client_event = {
                        .events = EPOLLIN,
                        .data.fd = client_fd
                    };
                    epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &client_event);
                }
                else if (strcmp(protocol, "udp") == 0) {
                    char buffer[1024];
                    struct sockaddr_in client_addr;
                    socklen_t addr_len = sizeof(client_addr);
                    ssize_t bytes_received = recvfrom(server_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &addr_len);
                    if (bytes_received > 0) {
                        printf("Received UDP message: %.*s from client %s:%d\n", (int)bytes_received, buffer, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                        sendto(server_fd, response_msg, strlen(response_msg), 0, (struct sockaddr *)&client_addr, addr_len);
                    }
                }
            } 
            else {
                char buffer[1024];
                ssize_t bytes_received = recv(events[i].data.fd, buffer, sizeof(buffer), 0);
                if (bytes_received > 0) {
                    printf("Received TCP message: %.*s from client %d\n", (int)bytes_received, buffer, events[i].data.fd);
                    send(events[i].data.fd, response_msg, strlen(response_msg), 0);
                } else {
                    close(events[i].data.fd);
                }
            }
        }   
    }

    close(server_fd);
    close(epfd);
    return 0;
}