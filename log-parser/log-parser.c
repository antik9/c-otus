#define _POSIX_C_SOURCE 200809L

#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#include "../hash-table/ht.h"
#include "process.h"
#include "stats.h"
#include "utils.h"
#include "walk.h"

#define USAGE   \
    "usage: \n" \
    "./log-parser -t <number-of-threads> -d <log-directory>\n"

void parse_logs_multithread(char* log_dir, int num_threads) {
    Message message;
    message.mtype = 1;

    LogFileStats* stats[1024];
    pthread_t* threads = malloc(sizeof(pthread_t) * num_threads);

    int number_of_files = get_log_stats_from_dir(log_dir, stats, 1024);
    if (number_of_files == -1) goto err_get_files;
    if (number_of_files == 0) goto no_files_in_dir;

    int msgid = msgget(QUEUE_MESSAGE_KEY, 0666 | IPC_CREAT);

    for (int i = 0; i < number_of_files; ++i) {
        memset(message.mtext, '\0', 64);
        strncpy(message.mtext, stats[i]->filename,
                strlen(stats[i]->filename) + 1);
        msgsnd(msgid, &message, MESSAGE_SIZE, MSG_FLAGS);
    }

    for (int i = 0; i < num_threads; ++i) {
        memset(message.mtext, '\0', 64);
        strncpy(message.mtext, DONE, strlen(DONE) + 1);
        msgsnd(msgid, &message, MESSAGE_SIZE, MSG_FLAGS);
    }

    for (int i = 0; i < num_threads; ++i) {
        ProcessFilesArgs* args = malloc(sizeof(ProcessFilesArgs));
        args->stats = stats;
        args->number_of_files = number_of_files;
        pthread_create(&threads[i], NULL, process_log_files, args);
    }

    for (int i = 0; i < num_threads; ++i) pthread_join(threads[i], NULL);

    msgctl(msgid, IPC_RMID, NULL);
    print_log_stats(stats, number_of_files);
    goto clean;

err_get_files:
    fprintf(stderr, "cannot get files from directory\n");
    goto clean;

no_files_in_dir:
    fprintf(stderr, "no log files in provided directory\n");

clean:
    for (int i = 0; i < number_of_files; ++i) {
        free(stats[i]->filename);
        destroy_hash_table(stats[i]->requests_by_referrer);
        destroy_hash_table(stats[i]->traffic_by_url);
        free(stats[i]);
    }

    free(threads);
}

int main(int argc, char* argv[]) {
    int num_threads = 1, opt;
    char* log_dir = NULL;

    while ((opt = getopt(argc, argv, "t:d:")) != -1) {
        switch (opt) {
            case 't':
                num_threads = atoi(optarg);
                break;
            case 'd':
                log_dir = strdup(optarg);
                break;
            default:
                handle_error(USAGE);
        }
    }

    if (log_dir == NULL) handle_error(USAGE);
    parse_logs_multithread(log_dir, num_threads);

    free(log_dir);
    return 0;
}
