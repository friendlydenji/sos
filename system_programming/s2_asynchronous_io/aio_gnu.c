#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <aio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h> 
#include <signal.h>
#include <semaphore.h>

#define BUFFER_SIZE     1024
#define PIPE_NAME       "/tmp/aio_pipe"

int notify_signal = SIGEV_NONE;
static volatile sig_atomic_t done = 0;
static sem_t semaphore;

static void signal_handler(int sig, siginfo_t *si, void *ctx) {
    (void)ctx;
    if (sig != SIGUSR1) {
        fprintf(stderr, "Unexpected signal: %d\n", sig);
        exit(EXIT_FAILURE);
    }
    printf("Received SIGUSR1 signal\n");
    struct aiocb aio_control_tmp = *(struct aiocb *)si->si_value.sival_ptr;
    (void)aio_control_tmp;
    done = 1;
}

static void thread_handler(union sigval sv) {
    struct aiocb *aio_control_tmp = (struct aiocb *)sv.sival_ptr;
    ssize_t bytes = aio_return(aio_control_tmp);
    if (bytes > 0) {
        printf("Thread handler read %zd bytes: %.*s\n", bytes, (int)bytes, (char *)aio_control_tmp->aio_buf);
    } else {
        perror("aio_return in thread handler");
    }
    sem_post(&semaphore);
}

int main(int argc, char *argv[]) 
{
    if (argc > 1) {
        notify_signal = atoi(argv[1]);
    }
    else {
        (void)argc;
        (void)argv;
    }

    if (mkfifo(PIPE_NAME, 0666) == -1) {
        if (unlink(PIPE_NAME) == -1) {
            perror("unlink");
            perror("mkfifo");
            exit(EXIT_FAILURE);
        }
        mkfifo(PIPE_NAME, 0666);
    }

    struct sigaction sa;
    sigset_t block, old; 
    if (notify_signal == SIGEV_SIGNAL) {
        memset(&sa, 0, sizeof(sa));
        sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = signal_handler;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGUSR1, &sa, NULL);

        sigemptyset(&block);
        sigaddset(&block, SIGUSR1);
        sigprocmask(SIG_BLOCK, &block, &old);
    }

    char buffer[BUFFER_SIZE];
    struct aiocb aio_control;
    memset(&aio_control, 0, sizeof(struct aiocb));

    int fd = open(PIPE_NAME, O_RDONLY);
    aio_control.aio_fildes = fd;
    aio_control.aio_buf = buffer;
    aio_control.aio_nbytes = BUFFER_SIZE;
    aio_control.aio_offset = 0;
    aio_control.aio_sigevent.sigev_notify = notify_signal;
    if (notify_signal == SIGEV_SIGNAL) {
        aio_control.aio_sigevent.sigev_signo = SIGUSR1;
        aio_control.aio_sigevent.sigev_value.sival_ptr = &aio_control;
    }
    else if (notify_signal == SIGEV_THREAD) {
        aio_control.aio_sigevent.sigev_notify_function = thread_handler;
        aio_control.aio_sigevent.sigev_notify_attributes = NULL;
        aio_control.aio_sigevent.sigev_value.sival_ptr = &aio_control;
    }

    if (aio_read(&aio_control) == -1) {
        perror("aio_read");
        exit(EXIT_FAILURE);
    }

    if (notify_signal == SIGEV_NONE) {
        const struct aiocb *list[1] = { &aio_control };

        if (aio_error(&aio_control) == EINPROGRESS) {
            aio_suspend(list, 1, NULL);
        }
    }
    else if (notify_signal == SIGEV_SIGNAL) {
        while (!done) {
            sigsuspend(&old);
        }
    }
    else if (notify_signal == SIGEV_THREAD) {
        sem_wait(&semaphore);
    }

    if (notify_signal == SIGEV_NONE || notify_signal == SIGEV_SIGNAL) {
        ssize_t bytes = aio_return(&aio_control);
        if (bytes > 0) {
            printf("Read %zd bytes: %.*s\n", bytes, (int)bytes, buffer);
        } else {
            perror("aio_return");
        }
    }

    close(fd);
    unlink(PIPE_NAME);
    return 0;
}