#ifndef __STATS_H__
#define __STATS_H__

#include "../hash-table/ht.h"

typedef struct _LogFileStats {
    char* filename;
    hash_table traffic_by_url;
    hash_table requests_by_referrer;
} LogFileStats;

void print_log_stats(LogFileStats** stats, int number_of_files);
#endif
