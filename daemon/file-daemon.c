#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#define DAEMON_ERROR "daemon error\n"
#define FILE_DOES_NOT_EXIST "file does not exist\n"
#define USAGE                    \
    "usage:\n"                   \
    "    ./file-daemon <file>\n" \
    "    ./file-daemon -d <file>\n"

#define BUF_SIZE 1024
#define LISTEN_QUEUE 1
#define SOCKET_PATH "file-daemon.sock"

#define handle_error(msg)     \
    do {                      \
        fprintf(stderr, msg); \
        exit(EXIT_FAILURE);   \
    } while (0)

void sig_handler() { unlink(SOCKET_PATH); }

int listen_socket() {
    int listenfd;
    struct sockaddr_un servaddr;

    listenfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listenfd == -1) handle_error("cannot start socket");

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sun_family = AF_UNIX;
    strncpy(servaddr.sun_path, SOCKET_PATH, sizeof(servaddr.sun_path) - 1);

    if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1)
        handle_error("cannot bind socket");

    if (listen(listenfd, LISTEN_QUEUE) == -1)
        handle_error("cannot listen to a socket");
    return listenfd;
}

void filewatch(char* filename) {
    int listenfd = listen_socket();
    char buf[BUF_SIZE];

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    while (true) {
        int connfd;
        socklen_t len;
        struct sockaddr_un cliaddr;

        len = sizeof(cliaddr);
        connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);
        if (connfd == -1) handle_error("cannot accept a new connection");

        struct stat sb;
        if (stat(filename, &sb) < 0) {
            if (errno == ENOENT)
                write(connfd, FILE_DOES_NOT_EXIST, strlen(FILE_DOES_NOT_EXIST));
            else
                write(connfd, DAEMON_ERROR, strlen(DAEMON_ERROR));
            goto finalize;
        }

        memset(buf, 0, BUF_SIZE);
        snprintf(buf, BUF_SIZE, "size of %s is %lu bytes\n", filename,
                 sb.st_size);
        write(connfd, buf, strlen(buf));

    finalize:
        close(connfd);
    }
}

int main(int argc, char* argv[]) {
    if (argc > 3 || argc < 2) handle_error(USAGE);
    if (argc == 3 && strcmp(argv[1], "-d")) handle_error(USAGE);

    bool daemon_mode = argc == 3;
    char* filename = daemon_mode ? argv[2] : argv[1];
    if (daemon_mode) {
        pid_t child;
        if ((child = fork()) > 0) {
            printf("pid: %d\n", child);
            return 0;
        }
    }

    filewatch(filename);
    return 0;
}
