#define _POSIX_C_SOURCE 200809L

#include "process.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#include "utils.h"

void process_log_file(char* filename, LogFileStats** stats, int count) {
    LogFileStats* log_file_stats = NULL;
    for (int i = 0; i < count; ++i) {
        if (!strcmp(stats[i]->filename, filename)) {
            log_file_stats = stats[i];
            break;
        }
    }

    if (log_file_stats == NULL) return;

    FILE* file = fopen(filename, "r");
    if (file == NULL) return;

    char* line = NULL;
    size_t n = 0;
    while (getline(&line, &n, file) != -1) {
        LogEntry* log_entry = parse_log_entry(line);
        if (log_entry == NULL) continue;

        put_entry_in_hash_table(log_file_stats->traffic_by_url, log_entry->url,
                                log_entry->number_of_bytes);
        put_entry_in_hash_table(log_file_stats->requests_by_referrer,
                                log_entry->referer, 1);
        free(log_entry->referer);
        free(log_entry->url);
        free(log_entry);
    }

    free(line);
    fclose(file);
}

void* process_log_files(void* _args) {
    ProcessFilesArgs* args = _args;
    Message message;

    int msgid = msgget(QUEUE_MESSAGE_KEY, 0666 | IPC_CREAT);

    while (true) {
        memset(message.mtext, '\0', 64);
        msgrcv(msgid, &message, MESSAGE_SIZE, 1, MSG_FLAGS);
        if (!strncmp(message.mtext, DONE, strlen(DONE) + 1)) break;
        process_log_file(message.mtext, args->stats, args->number_of_files);
    }

    free(args);
    return NULL;
}

