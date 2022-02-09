#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ht.h"

#define MAX_WORD_SIZE 1024

void error(char* msg) {
    puts(msg);
    exit(1);
}

void put_word_in_hash_table(hash_table ht, char* buf, size_t word_length) {
    if (word_length == 0) return;
    char* word = strndup(buf, word_length);

    entry e = ht_get(ht, word);
    if (e != NULL) {
        ++e->value;
        free(word);
    } else {
        e = new_entry(word);
        e->value = 1;
        ht_insert(ht, e);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) error("usage\n    ./wc <file>");

    char* filename = argv[1];
    FILE* file = fopen(filename, "r");
    if (file == NULL) error("cannot open the file");

    char buf[MAX_WORD_SIZE] = {'\0'};
    hash_table ht = create_hash_table();

    char next;
    size_t word_length = 0;
    while (next = fgetc(file), true) {
        if (feof(file)) break;
        if (!isalnum(next)) {
            put_word_in_hash_table(ht, buf, word_length);
            word_length = 0;
            continue;
        }
        if (word_length == MAX_WORD_SIZE) {
            put_word_in_hash_table(ht, buf, word_length);
            word_length = 0;
        }
        buf[word_length++] = next;
    }
    put_word_in_hash_table(ht, buf, word_length);

    ht_print_values(ht);
    destroy_hash_table(ht);
    fclose(file);
    return 0;
}
