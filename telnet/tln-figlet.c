#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <getopt.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define handle_error(msg)     \
    do {                      \
        fprintf(stderr, msg); \
        exit(EXIT_FAILURE);   \
    } while (0)

#define USAGE   \
    "usage: \n" \
    "    ./tln-figlet -f <font> -t <text>\n"

#define SOURCE_HOST "telehack.com"
#define PORT 23

int hostname_to_ip(char* hostname, char* ip) {
    struct hostent* he;
    struct in_addr** addr_list;
    int i;

    if ((he = gethostbyname(hostname)) == NULL) return -1;

    addr_list = (struct in_addr**)he->h_addr_list;

    for (i = 0; addr_list[i] != NULL; i++) {
        strcpy(ip, inet_ntoa(*addr_list[i]));
        return 0;
    }

    return -1;
}

void read_from_socket(int sock, char** buf, int* buf_size) {
    int n_bytes;
    while ((n_bytes = read(sock, *buf + strlen(*buf), 1024)) > 0) {
        *buf_size += 1024;
        *buf = realloc(*buf, *buf_size * sizeof(char));
        memset(*buf + strlen(*buf), '\0',
               (*buf_size - strlen(*buf)) * sizeof(char));
        if (strncmp("\n.", *buf + strlen(*buf) - 2, 2) == 0) break;
    }
}

void figlet_through_net(char* font, char* text) {
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        handle_error("cannot open socket");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    char ip[64];
    if (hostname_to_ip(SOURCE_HOST, ip) < 0) {
        fprintf(stderr, "cannot get ip for domain");
        goto clean;
    };

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0)
        handle_error("invalid address");

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
        handle_error("connection Failed");

    // read welcome message
    int buf_size = 1025;
    char* buf = calloc(buf_size, sizeof(char));
    read_from_socket(sock, &buf, &buf_size);

    buf_size = strlen(font) + strlen(text) + 16;
    buf = realloc(buf, buf_size * sizeof(char));
    memset(buf, '\0', buf_size * sizeof(char));
    sprintf(buf, "figlet /%s %s\r\n", font, text);

    if (send(sock, buf, strlen(buf), 0) < 0) {
        fprintf(stderr, "cannot send command to server");
        goto clean;
    }

    // read figlet message
    buf_size = 1025;
    buf = realloc(buf, buf_size * sizeof(char));
    memset(buf, '\0', buf_size * sizeof(char));
    read_from_socket(sock, &buf, &buf_size);

    char* begin = strchr(buf, '\n') + 1;  // omit echo lines
    begin = strchr(begin, '\n') + 1;      // omit echo lines

    buf[strlen(buf) - 2] = '\0';
    printf("%s\n", begin);

clean:
    free(buf);
    close(sock);
}

int main(int argc, char* argv[]) {
    int opt;
    char *font = NULL, *text = NULL;

    while ((opt = getopt(argc, argv, "f:t:")) != -1) {
        switch (opt) {
            case 'f':
                font = strdup(optarg);
                break;
            case 't':
                text = strdup(optarg);
                break;
            default:
                handle_error(USAGE);
        }
    }

    if (font == NULL || text == NULL) handle_error(USAGE);
    figlet_through_net(font, text);

    free(font);
    free(text);
    return 0;
}
