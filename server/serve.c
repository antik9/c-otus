#define _POSIX_C_SOURCE 200809L

#include "serve.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <threads.h>
#include <unistd.h>

#ifndef SO_REUSEPORT
#define SO_REUSEPORT 15
#endif

#define GET "GET"

#define INTERNAL_SERVER_ERROR \
    "HTTP/1.1 500 Internal Server Error\r\n\r\ninternal server error\r\n\r\n"
#define METHOD_NOT_ALLOWED \
    "HTTP/1.1 405 Method Not Allowed\r\nallow: GET\r\n\r\n"
#define FILE_NOT_FOUND "HTTP/1.1 404 Not Found\r\n\r\nfile not found\r\n\r\n"
#define ACCESS_DENIED "HTTP/1.1 403 Forbidden\r\n\r\naccess denied\r\n\r\n"
#define OK                              \
    "HTTP/1.1 200 OK\r\nContent-Type: " \
    "application/octet-stream\r\nContent-Length: %lu\r\n\r\n"

#define BUF_SIZE 2048
#define BACKLOG 128
#define MAX_EPOLL_EVENTS 128
thread_local struct epoll_event events[MAX_EPOLL_EVENTS];
thread_local int nfds;
thread_local char buffer[BUF_SIZE];

int setnonblocking(int sock) {
    int opts;
    opts = fcntl(sock, F_GETFL);
    if (opts < 0) {
        perror("fcntl(F_GETFL)");
        return -1;
    }
    opts = (opts | O_NONBLOCK);
    if (fcntl(sock, F_SETFL, opts) < 0) {
        perror("fcntl(F_SETFL)");
        return -1;
    }

    return 0;
}

void handle_request(int fd, char* directory) {
#define HTTP_ERROR(msg)                                         \
    do {                                                        \
        if (send(fd, msg, strlen(msg), 0) < 0) perror("write"); \
        if (filename != NULL) free(filename);                   \
        return;                                                 \
    } while (0)
    char* filename = NULL;

    memset(buffer, '\0', BUF_SIZE);
    int rc = recv(fd, buffer, sizeof(buffer), 0);
    if (rc < 0) {
        perror("read");
        return;
    }
    buffer[rc] = 0;

    if (strncmp(GET, buffer, strlen(GET))) HTTP_ERROR(METHOD_NOT_ALLOWED);

    char* path = strchr(buffer, '/');
    int filename_length = strchr(path, ' ') - path + strlen(directory) + 1;
    filename = calloc(filename_length + 1, 1);
    sprintf(filename, "%s/", directory);
    snprintf(filename + strlen(filename), filename_length - strlen(filename),
             "%s", path + 1);

    if (access(filename, F_OK) != 0) HTTP_ERROR(FILE_NOT_FOUND);
    if (access(filename, R_OK) != 0) HTTP_ERROR(ACCESS_DENIED);

    struct stat sb;
    if (stat(filename, &sb) < 0) {
        if (errno == ENOENT) HTTP_ERROR(FILE_NOT_FOUND);
        HTTP_ERROR(INTERNAL_SERVER_ERROR);
    }

    int file = open(filename, O_RDONLY);
    if (file < 0) HTTP_ERROR(INTERNAL_SERVER_ERROR);

    char response_headers_ok[1025] = {0};
    snprintf(response_headers_ok, 1024, OK, sb.st_size);
    if (send(fd, response_headers_ok, strlen(response_headers_ok), 0) < 0) {
        perror("write");
        goto clean_resources;
    }

    off_t bytes_send = 0;
    while (bytes_send < sb.st_size) {
        int n = sendfile(fd, file, NULL, sb.st_size - bytes_send);
        if (n < 0) {
            perror("cannot send file");
            goto clean_resources;
        }
        bytes_send += n;
    }

clean_resources:
    free(filename);
    close(file);
}

void* serve(void* _serve_args) {
#define log_err_and_return(msg)       \
    do {                              \
        fprintf(stderr, "%s\n", msg); \
        return NULL;                  \
    } while (0)
    ServeArgs* serve_args = _serve_args;

    signal(SIGPIPE, SIG_IGN);

    int efd = epoll_create(MAX_EPOLL_EVENTS);
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) log_err_and_return("cannot create socket");

    setnonblocking(listenfd);

    struct sockaddr_in servaddr = {0};
    servaddr.sin_family = AF_INET;

    char ip_addr[INET_ADDRSTRLEN] = {0};
    char* port_separator = strchr(serve_args->address, ':');
    if (port_separator == NULL) log_err_and_return("invalid address provided");

    strncpy(ip_addr, port_separator,
            strchr(port_separator, ':') - port_separator);

    if (inet_pton(AF_INET, ip_addr, &servaddr.sin_addr.s_addr) < 0)
        log_err_and_return("invalid ip provided");
    servaddr.sin_port = htons(atoi(port_separator + 1));

    int flag = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &flag,
                   sizeof(flag)))
        log_err_and_return("cannot set socket options");

    if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
        log_err_and_return("cannot bind socket");

    if (listen(listenfd, BACKLOG) < 0)
        log_err_and_return("cannot start listen");

    struct epoll_event listenev;
    listenev.events = EPOLLIN | EPOLLET;
    listenev.data.fd = listenfd;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, listenfd, &listenev) < 0)
        log_err_and_return("epoll_ctl failed");

    struct epoll_event connev;
    int events_count = 1;

    while (true) {
        int nfds = epoll_wait(efd, events, MAX_EPOLL_EVENTS, -1);

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == listenfd) {
                while (true) {
                    int connfd = accept(listenfd, NULL, NULL);
                    if (connfd < 0) {
                        if (errno == EAGAIN) break;
                        perror("accept");
                        break;
                    }

                    if (events_count == MAX_EPOLL_EVENTS - 1) {
                        printf("Event array is full\n");
                        close(connfd);
                        break;
                    }

                    connev.data.fd = connfd;
                    connev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
                    if (epoll_ctl(efd, EPOLL_CTL_ADD, connfd, &connev) < 0) {
                        perror("epoll_ctl");
                        close(connfd);
                        break;
                    }

                    events_count++;
                }
            } else {
                int fd = events[i].data.fd;
                if (events[i].events & EPOLLIN) {
                    handle_request(fd, serve_args->directory);
                    epoll_ctl(efd, EPOLL_CTL_DEL, fd, &connev);
                    close(fd);
                    events_count--;
                }
            }
        }
    }

    return NULL;
}
