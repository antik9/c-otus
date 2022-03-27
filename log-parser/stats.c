#include <unistd.h>
#include <stdio.h>

#include "stats.h"
#include "utils.h"

#define TOP 10

void aggregate_total_bytes(void* bytes, entry e) {
    long long* n_bytes = bytes;
    *n_bytes += e->value;
}

void aggregate_top(void* _top, entry e) {
    entry* top = _top;
    for (int i = 0; i < TOP; ++i) {
        if (top[i] != NULL) {
            if (top[i]->value < e->value) {
                entry popped_entry = top[i];
                top[i] = e;
                e = popped_entry;
            }
            continue;
        }

        top[i] = e;
        break;
    }
}

void merge_hash_table(void* _aggregation, entry e) {
    hash_table aggregation = _aggregation;
    put_entry_in_hash_table(aggregation, e->key, e->value);
}

void print_log_stats(LogFileStats** stats, int number_of_files) {
    long long n_bytes = 0;
    entry top_referers[TOP] = {NULL};
    entry top_urls[TOP] = {NULL};

    hash_table aggregated_urls = create_hash_table();
    hash_table aggregated_referers = create_hash_table();

    for (int i = 0; i < number_of_files; ++i) {
        ht_for_each(stats[i]->traffic_by_url, merge_hash_table,
                    aggregated_urls);
        ht_for_each(stats[i]->requests_by_referrer, merge_hash_table,
                    aggregated_referers);
    }

    ht_for_each(aggregated_urls, aggregate_total_bytes, &n_bytes);
    ht_for_each(aggregated_urls, aggregate_top, top_urls);
    ht_for_each(aggregated_referers, aggregate_top, top_referers);

    printf("processed %lld bytes\n", n_bytes);

    puts("\nTOP URLs by traffic:");
    for (int i = 0; i < TOP; ++i) {
        if (top_urls[i] == NULL) break;
        printf("%s: %lld bytes\n", top_urls[i]->key, top_urls[i]->value);
    }

    puts("\nTOP referers by requests:");
    for (int i = 0; i < TOP; ++i) {
        if (top_referers[i] == NULL) break;
        printf("%s: %lld requests\n", top_referers[i]->key,
               top_referers[i]->value);
    }

    destroy_hash_table(aggregated_urls);
    destroy_hash_table(aggregated_referers);
}
