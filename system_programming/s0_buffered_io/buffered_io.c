#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 

#define BUFFER_SIZE             1024

int buffering_controller = 2;

int main (int argc, char *argv[])
{
    if (argc > 1) {
        buffering_controller = atoi(argv[1]);
    }
    else {
        (void)argc;
        (void)argv;
    }
    char buffer[BUFFER_SIZE];
    setvbuf(stdout, buffer, buffering_controller, BUFFER_SIZE);
    printf ("Arrr!");
    while (1) {
        char input[2];
        ssize_t n = read(STDIN_FILENO, input, sizeof(input));

        if (n <= 0)
            break;

        if (input[0] == '1') {
            fflush(stdout);
        }
        else if (input[0] == '2') {
            break;
        }
    }
    return 0;
}