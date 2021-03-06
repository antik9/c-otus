#define _POSIX_C_SOURCE 200809L

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
#define USAGE             \
    "usage:\n"            \
    "    ./file-daemon\n" \
    "    ./file-daemon -c <config-file>\n"

#define BUF_SIZE 1024
#define LISTEN_QUEUE 1
#define SOCKET_PATH "file-daemon.sock"
#define DEFAULT_CONFIG_FILE "file-daemon.conf"
#define WATCH_EVENTS IN_MODIFY | IN_DELETE | IN_DELETE_SELF

#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))

#define handle_error(msg)     \
    do {                      \
        fprintf(stderr, msg); \
        exit(EXIT_FAILURE);   \
    } while (0)

struct {
    bool daemon;
    char* filename;
} config;

enum ChangeEvent {
    NO_CHANGES,
    FILE_NOT_FOUND,
    FILE_MODIFIED,
    UNKNOWN_ERROR,
};

void free_resources() { free(config.filename); }

void sig_handler() { unlink(SOCKET_PATH); }

int listen_socket() {
    int listenfd;
    struct sockaddr_un servaddr = {0};

    listenfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listenfd == -1) handle_error("cannot start socket");

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
                event_type = FILE_NOT_FOUND;
            else if (event->mask & IN_DELETE_SELF)
                event_type = FILE_NOT_FOUND;
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
    int notifyfd = 0, wd = 0;
    char buf[BUF_SIZE];
    char notification_buffer[EVENT_BUF_LEN];

    enum ChangeEvent state = FILE_NOT_FOUND;
    while (true) {
        struct sockaddr_un cliaddr;
        socklen_t len = sizeof(cliaddr);
        int connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);
        if (connfd == -1) handle_error("cannot accept a new connection");

        if (state == FILE_NOT_FOUND) {
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
                state = FILE_NOT_FOUND;
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

void read_config(char* path) {
    FILE* file = fopen(path, "r");
    if (file == NULL) handle_error("cannot open provided config file");

    char* line = NULL;
    size_t len = 0;
    while (getline(&line, &len, file) > 0) {
        char* separator = strchr(line, '=');
        if (!separator) continue;
        if (separator - line == 4 && !strncmp("file", line, 4)) {
            config.filename = strdup(separator + 1);
            config.filename[strlen(config.filename) - 1] = '\0';
        }
        if (separator - line == 6 && !strncmp("daemon", line, 6))
            config.daemon = !strncmp("true", separator + 1, 4);
    }

    free(line);
    fclose(file);
}

int main(int argc, char* argv[]) {
    if (argc > 3 || argc == 2) handle_error(USAGE);
    if (argc == 3 && strcmp(argv[1], "-c")) handle_error(USAGE);

    read_config(argc == 3 ? argv[2] : DEFAULT_CONFIG_FILE);
    if (config.daemon) {
        pid_t child;
        if ((child = fork()) > 0) {
            printf("pid: %d\n", child);
            return 0;
        }
    }

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    atexit(free_resources);

    filewatch(config.filename, listen_socket());
    return 0;
}
