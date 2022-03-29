#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdlib.h>

#include "../hash-table/ht.h"

#define handle_error(msg)     \
    do {                      \
        fprintf(stderr, msg); \
        exit(EXIT_FAILURE);   \
    } while (0)

#define MESSAGE_SIZE 64
typedef struct _Message {
    long mtype;
    char mtext[MESSAGE_SIZE];
} Message;

typedef struct _LogEntry {
    char* url;
    char* referer;
    long long number_of_bytes;
} LogEntry;

LogEntry* parse_log_entry(char* line);
void put_entry_in_hash_table(hash_table ht, char* key, size_t value);
#endif
