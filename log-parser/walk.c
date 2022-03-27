#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

#include "walk.h"
#include "utils.h"

#define ACCESS_LOG "access.log"

int get_log_stats_from_dir(char* log_dir, LogFileStats** stats,
                           int stats_size) {
    int number_of_files = 0;

    DIR* dir;
    struct dirent* ent;
    if ((dir = opendir(log_dir)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (!strncmp(ent->d_name, ACCESS_LOG, strlen(ACCESS_LOG))) {
                stats[number_of_files] = malloc(sizeof(LogFileStats));
                stats[number_of_files]->filename = calloc(sizeof(char), 64);
                strncpy(stats[number_of_files]->filename, log_dir,
                        strlen(log_dir));
                stats[number_of_files]->filename[strlen(log_dir)] = '/';
                strncpy(stats[number_of_files]->filename + strlen(log_dir) + 1,
                        ent->d_name, MESSAGE_SIZE - strlen(log_dir));
                stats[number_of_files]->requests_by_referrer =
                    create_hash_table();
                stats[number_of_files]->traffic_by_url = create_hash_table();
                if (++number_of_files == stats_size) break;
            }
        }
        closedir(dir);
    } else
        handle_error("cannot open log directory");

    return number_of_files;
}

