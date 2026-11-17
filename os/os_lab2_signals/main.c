#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

static volatile sig_atomic_t was_sighup = 0;

static void on_sighup(int signal_number)
{
    (void)signal_number;
    was_sighup = 1;
}

static void fail(const char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

static void set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        fail("fcntl");
    }
}

int main(void)
{
    const int port = 8080;

    sigset_t blocked_mask;
    sigset_t wait_mask;
    sigemptyset(&blocked_mask);
    sigaddset(&blocked_mask, SIGHUP);
    if (sigprocmask(SIG_BLOCK, &blocked_mask, &wait_mask) == -1) {
        fail("sigprocmask");
    }
    sigdelset(&wait_mask, SIGHUP);

    struct sigaction action = {0};
    action.sa_handler = on_sighup;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGHUP, &action, NULL) == -1) {
        fail("sigaction");
    }

    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener == -1) {
        fail("socket");
    }
    if (listener >= FD_SETSIZE) {
        fprintf(stderr, "Socket descriptor is too large for pselect\n");
        close(listener);
        return EXIT_FAILURE;
    }
    int reuse = 1;
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
        fail("setsockopt");
    }
    set_nonblocking(listener);

    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons((unsigned short)port);
    if (bind(listener, (struct sockaddr *)&address, sizeof(address)) == -1) {
        fail("bind");
    }
    if (listen(listener, SOMAXCONN) == -1) {
        fail("listen");
    }

    setvbuf(stdout, NULL, _IOLBF, 0);
    printf("Listening on port %d, PID %ld\n", port, (long)getpid());
    int client = -1;

    for (;;) {
        fd_set readable;
        FD_ZERO(&readable);
        FD_SET(listener, &readable);
        int max_fd = listener;
        if (client != -1) {
            FD_SET(client, &readable);
            if (client > max_fd) {
                max_fd = client;
            }
        }

        int ready = pselect(max_fd + 1, &readable, NULL, NULL, NULL, &wait_mask);
        if (ready == -1 && errno != EINTR) {
            fail("pselect");
        }
        if (was_sighup) {
            was_sighup = 0;
            puts("Received SIGHUP");
        }
        if (ready == -1) {
            continue;
        }

        if (client != -1 && FD_ISSET(client, &readable)) {
            char buffer[4096];
            ssize_t count = recv(client, buffer, sizeof(buffer), 0);
            if (count > 0) {
                printf("Received %zd bytes\n", count);
            } else if (count == 0) {
                puts("Client disconnected");
                close(client);
                client = -1;
            } else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
                perror("recv");
                close(client);
                client = -1;
            }
        }

        if (FD_ISSET(listener, &readable)) {
            int incoming = accept(listener, NULL, NULL);
            if (incoming == -1) {
                if (errno != EAGAIN && errno != EWOULDBLOCK &&
                    errno != EINTR && errno != ECONNABORTED) {
                    fail("accept");
                }
            } else if (client != -1 || incoming >= FD_SETSIZE) {
                close(incoming);
                puts("New connection: closed");
            } else {
                set_nonblocking(incoming);
                client = incoming;
                puts("New connection: accepted");
            }
        }
    }
}
