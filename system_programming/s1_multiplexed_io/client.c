#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/time.h>
#include <arpa/inet.h>

#define MAX_EVENTS 10

static char protocol[4];
static int port;
static int interval;
static volatile sig_atomic_t send_count = 0;
static volatile sig_atomic_t allowed = 0;

void parse_arguments(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s --protocol=<protocol> --port=<port> --interval=<interval>(sec)\n", argv[0]);
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
            else if (strncmp(arg, "--interval=", 11) == 0) {
                interval = atoi(value);
            }
            else {
                fprintf(stderr, "Unknown argument: %s\n", arg);
                exit(EXIT_FAILURE);
            }
        }
    }
}

void sigalrm_handler(int signum)
{
    (void)signum;
    send_count++;
    allowed = 1;
}


int main (int argc, char *argv[])
{
    int client_fd;
    struct sockaddr_in addr;

    parse_arguments(argc, argv);

    struct sigaction sa;
    sa.sa_handler = sigalrm_handler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGALRM, &sa, NULL);

    struct itimerval timer;
    timer.it_value.tv_sec = interval;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = interval;
    timer.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &timer, NULL);

    if (strcmp(protocol, "tcp") == 0) {
        printf("Starting TCP client on port %d\n", port);
        client_fd = socket(AF_INET, SOCK_STREAM, 0);
        addr = (struct sockaddr_in) {
            .sin_family = AF_INET,
            .sin_port = htons(port)
        };

        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
        connect(client_fd, (struct sockaddr *)&addr, sizeof(addr));
    } 
    else if (strcmp(protocol, "udp") == 0) {
        printf("Starting UDP client on port %d\n", port);
        client_fd = socket(AF_INET, SOCK_DGRAM, 0);
        addr = (struct sockaddr_in) {
            .sin_family = AF_INET,
            .sin_port = htons(port)
        };
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    }

    char send_msg[30];
    char buffer[1024];
    while (1) {
        if (allowed) {
            snprintf(send_msg, sizeof(send_msg), "Message %d sent\n", send_count);
            allowed = 0;
            if (strcmp(protocol, "tcp") == 0) {
                send(client_fd, send_msg, strlen(send_msg), 0);
                ssize_t n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
                printf("Sent and received TCP message from server: %.*s\n", (int)n, buffer);
            } else if (strcmp(protocol, "udp") == 0) {
                struct sockaddr_in server_addr;
                socklen_t addr_len = sizeof(server_addr);
                sendto(client_fd, send_msg, strlen(send_msg), 0, (struct sockaddr *)&addr, sizeof(addr));
                ssize_t n = recvfrom(client_fd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&server_addr, &addr_len);
                printf("Sent and received UDP message from server: %.*s\n", (int)n, buffer);
            }
        }   
    }
    
    close(client_fd);
    return 0;
}