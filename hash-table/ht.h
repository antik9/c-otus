#ifndef __HASH_TABLE_H__
#define __HASH_TABLE_H__

struct entry_ {
    char* key;
    long long value;
};
typedef struct entry_* entry;

struct hash_table_ {
    entry* entries;
    unsigned size;
    unsigned occupied;
};
typedef struct hash_table_* hash_table;

hash_table create_hash_table();
void destroy_hash_table(hash_table ht);

entry new_entry(char* key);

entry ht_get(hash_table ht, char* key);
void ht_insert(hash_table ht, entry e);
void ht_delete(hash_table ht, char* key);
void ht_print_values(hash_table ht);
void ht_for_each(hash_table ht, void(callback)(void* state, entry e),
                 void* state);
#endif
