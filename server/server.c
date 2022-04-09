#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "serve.h"

#define handle_error(msg)     \
    do {                      \
        fprintf(stderr, msg); \
        exit(EXIT_FAILURE);   \
    } while (0)

#define USAGE   \
    "usage: \n" \
    "    ./server -a <address:port> -d <directory> -t <number of threads>\n"

pthread_t* threads;
int num_threads = 1;

void stop_server() {
    for (int i = 0; i < num_threads; ++i) pthread_kill(threads[i], SIGINT);
}

int main(int argc, char* argv[]) {
    int opt;
    ServeArgs serve_args;

    while ((opt = getopt(argc, argv, "a:d:t:")) != -1) {
        switch (opt) {
            case 'a':
                serve_args.address = optarg;
                break;
            case 'd':
                serve_args.directory = optarg;
                break;
            case 't':
                num_threads = atoi(optarg);
                break;
            default:
                handle_error(USAGE);
        }
    }

    if (serve_args.address == NULL || serve_args.directory == NULL ||
        num_threads < 1)
        handle_error(USAGE);
    threads = malloc(sizeof(pthread_t) * num_threads);

    signal(SIGINT, stop_server);
    for (int i = 0; i < num_threads; ++i)
        pthread_create(&threads[i], NULL, serve, &serve_args);
    for (int i = 0; i < num_threads; ++i) pthread_join(threads[i], NULL);

    free(threads);
    return 0;
}
