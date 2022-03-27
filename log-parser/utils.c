#define _POSIX_C_SOURCE 200809L

#include "utils.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void put_entry_in_hash_table(hash_table ht, char* key, size_t value) {
    if (value == 0) return;
    char* _key = strdup(key);

    entry e = ht_get(ht, _key);
    if (e != NULL) {
        e->value += value;
        free(_key);
    } else {
        e = new_entry(_key);
        e->value = value;
        ht_insert(ht, e);
    }
}

LogEntry* parse_log_entry(char* line) {
    LogEntry* log_entry = malloc(sizeof(LogEntry));
    size_t len = strlen(line), it = 0;

    while (it < len && *(line + it) != '"') ++it;  // search request data
    while (it < len && *(line + it) != ' ') ++it;  // search URL
    if (it == len) goto clean;

    int start = ++it;
    while (it < len && *(line + it) != ' ') ++it;  // search end of URL
    log_entry->url = strndup(line + start, it - start);

    ++it;
    while (it < len && *(line + it) != ' ') ++it;  // search status code
    ++it;
    while (it < len && *(line + it) != ' ') ++it;  // search number of bytes
    ++it;
    if (it >= len) goto clean;

    log_entry->number_of_bytes = atoi(line + it);

    while (it < len && *(line + it) != '"') ++it;  // search referer
    if (it == len) goto clean;

    start = ++it;
    while (it < len && *(line + it) != '"') ++it;
    log_entry->referer = strndup(line + start, it - start);

    return log_entry;

clean:
    if (log_entry->url != NULL) free(log_entry->url);
    if (log_entry->referer != NULL) free(log_entry->referer);
    free(log_entry);
    return NULL;
}
