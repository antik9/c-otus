#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#define DAEMON_ERROR "daemon error\n"
#define FILE_NOT_EXIST "file does not exist\n"
#define DAEMON_ERROR "daemon error\n"
#define USAGE                    \
    "usage:\n"                   \
    "    ./file-daemon <file>\n" \
    "    ./file-daemon -d <file>\n"

#define BUF_SIZE 1024
#define LISTEN_QUEUE 1
#define SOCKET_PATH "file-daemon.sock"
#define WATCH_EVENTS IN_MODIFY | IN_DELETE | IN_DELETE_SELF

#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))

#define handle_error(msg)     \
    do {                      \
        fprintf(stderr, msg); \
        exit(EXIT_FAILURE);   \
    } while (0)

enum ChangeEvent {
    NO_CHANGES,
    FILE_DELETED,
    FILE_MODIFIED,
    UNKNOWN_ERROR,
};

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

enum ChangeEvent last_notification(int notifyfd, char* buffer) {
    int available;
    ioctl(notifyfd, FIONREAD, &available);
    if (available == 0) return NO_CHANGES;

    int length = read(notifyfd, buffer, EVENT_BUF_LEN);
    if (length < 0) return UNKNOWN_ERROR;
    if (length == 0) return NO_CHANGES;

    enum ChangeEvent event_type = NO_CHANGES;
    for (int i = 0; i < length;) {
        struct inotify_event* event = (struct inotify_event*)&buffer[i];
        if (!event->len) {
            if (event->mask & IN_MODIFY)
                event_type = FILE_MODIFIED;
            else if (event->mask & IN_DELETE)
                event_type = FILE_DELETED;
            else if (event->mask & IN_DELETE_SELF)
                event_type = FILE_DELETED;
        }

        i += EVENT_SIZE + event->len;
    }

    return event_type;
}

int notify_changes() {
    int fd = inotify_init();
    if (fd < 0) handle_error("cannot init inotify");
    return fd;
}

void filewatch(char* filename, int listenfd) {
    int notifyfd = notify_changes();
    int wd = inotify_add_watch(notifyfd, filename, WATCH_EVENTS);

    char buf[BUF_SIZE];
    char notification_buffer[EVENT_BUF_LEN];

    enum ChangeEvent state = NO_CHANGES;
    while (true) {
        struct sockaddr_un cliaddr;
        socklen_t len = sizeof(cliaddr);
        int connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);
        if (connfd == -1) handle_error("cannot accept a new connection");

        if (state == FILE_DELETED) {
            inotify_rm_watch(notifyfd, wd);
            // try to watch a newly created file with the same name
            notifyfd = notify_changes();
            wd = inotify_add_watch(notifyfd, filename, WATCH_EVENTS);
        } else if (strlen(buf) > 0) {
            switch (
                (state = last_notification(notifyfd, notification_buffer))) {
                case NO_CHANGES:
                    goto buffered_response;
                case UNKNOWN_ERROR:
                    write(connfd, DAEMON_ERROR, strlen(DAEMON_ERROR));
                    goto finalize_connection;
                default:
                    break;
            }
        }

        memset(buf, 0, BUF_SIZE);

        struct stat sb;
        if (stat(filename, &sb) < 0) {
            if (errno == ENOENT) {
                state = FILE_DELETED;
                snprintf(buf, strlen(FILE_NOT_EXIST) + 1, FILE_NOT_EXIST);
                goto buffered_response;
            }
            write(connfd, DAEMON_ERROR, strlen(DAEMON_ERROR));
            goto finalize_connection;
        }

        snprintf(buf, BUF_SIZE, "size of %s is %lu bytes\n", filename,
                 sb.st_size);

    buffered_response:
        write(connfd, buf, strlen(buf));

    finalize_connection:
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

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    filewatch(filename, listen_socket());
    return 0;
}
