#include "ht.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_OCCUPATION_SHARE 0.5
#define HASH_TABLE_GROW_MULTIPLICATOR 2
#define THOMBSTONE -1
#define INITIAL_SIZE 1024
#define SHIFT 37

hash_table create_hash_table_(size_t size) {
    hash_table ht = malloc(sizeof(struct hash_table_));
    ht->occupied = 0;
    ht->size = size;
    ht->entries = calloc(size, sizeof(entry));
    return ht;
}

hash_table create_hash_table() { return create_hash_table_(INITIAL_SIZE); }

void destroy_hash_table(hash_table ht) {
    if (ht != NULL) {
        for (size_t i = 0; i < ht->size; ++i) {
            if (ht->entries[i] != NULL) {
                free(ht->entries[i]->key);
                free(ht->entries[i]);
            }
        }
        free(ht->entries);
        free(ht);
    }
}

void ht_reallocate(hash_table ht) {
    hash_table temp =
        create_hash_table_(ht->size * HASH_TABLE_GROW_MULTIPLICATOR);
    for (size_t i = 0; i < ht->size; ++i) {
        entry e = ht->entries[i];
        if (e == NULL) continue;
        if (e->value == THOMBSTONE) {
            free(e->key);
            free(e);
            continue;
        }
        ht_insert(temp, e);
    }
    free(ht->entries);
    ht->entries = temp->entries;
    ht->size = temp->size;
    ht->occupied = temp->occupied;
    free(temp);
}

void ht_print_values(hash_table ht) {
    if (ht == NULL) return;
    for (size_t i = 0; i < ht->size; ++i) {
        if (ht->entries[i] != NULL)
            printf("%s: %lld\n", ht->entries[i]->key, ht->entries[i]->value);
    }
}

entry new_entry(char* key) {
    entry e = malloc(sizeof(struct entry_));
    e->key = key;
    e->value = 0;
    return e;
}

int hash(char* key) {
    const int p = 31;
    int hash_value = 0, p_pow = 1;
    for (size_t i = 0; i < strlen(key); ++i) {
        hash_value += (key[i] - 'a' + 1) * p_pow;
        p_pow *= p;
    }
    return hash_value;
}

int ht_get_key_position(hash_table ht, char* key) {
    int position = (hash(key) + SHIFT) % ht->size;
    while (ht->entries[position] != NULL &&
           ht->entries[position]->value != THOMBSTONE) {
        entry e = ht->entries[position];
        if (strcmp(e->key, key) == 0) break;
        position = (position + 1) % ht->size;
    }
    return position;
}

entry ht_get(hash_table ht, char* key) {
    int position = ht_get_key_position(ht, key);
    return ht->entries[position];
}

void ht_insert(hash_table ht, entry e) {
    if (ht->occupied >= (double)ht->size * MAX_OCCUPATION_SHARE) {
        ht_reallocate(ht);
    }

    int position = ht_get_key_position(ht, e->key);
    if (ht->entries[position] != NULL) {
        free(e->key);
        free(ht->entries[position]);
    } else
        ++ht->occupied;

    ht->entries[position] = e;
}

void ht_delete(hash_table ht, char* key) {
    entry e = ht_get(ht, key);
    if (e != NULL) {
        e->value = THOMBSTONE;
    }
}

void ht_for_each(hash_table ht, void(callback)(void* state, entry e),
                 void* state) {
    if (ht == NULL) return;
    for (size_t i = 0; i < ht->size; ++i)
        if (ht->entries[i] != NULL) callback(state, ht->entries[i]);
}
