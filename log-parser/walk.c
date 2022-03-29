#include "walk.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

#define ACCESS_LOG "access.log"

int count_files_in_dir(char* log_dir) {
    int number_of_files = 0;

    DIR* dir;
    struct dirent* ent;
    if ((dir = opendir(log_dir)) != NULL) {
        while ((ent = readdir(dir)) != NULL)
            if (!strncmp(ent->d_name, ACCESS_LOG, strlen(ACCESS_LOG)))
                ++number_of_files;
        closedir(dir);
    } else
        return -1;

    return number_of_files;
}

int get_log_stats_from_dir(char* log_dir, LogFileStats** stats,
                           int stats_size) {
    if (count_files_in_dir(log_dir) <= 0) return -1;

    int number_of_files = 0;
    DIR* dir;
    struct dirent* ent;

    if ((dir = opendir(log_dir)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (!strncmp(ent->d_name, ACCESS_LOG, strlen(ACCESS_LOG))) {
                stats[number_of_files] = malloc(sizeof(LogFileStats));
                stats[number_of_files]->filename = calloc(sizeof(char), 64);
                sprintf(stats[number_of_files]->filename, "%s/%s", log_dir,
                        ent->d_name);
                stats[number_of_files]->requests_by_referrer =
                    create_hash_table();
                stats[number_of_files]->traffic_by_url = create_hash_table();
                if (++number_of_files == stats_size) break;
            }
        }
        closedir(dir);
    } else
        return -1;

    return number_of_files;
}
