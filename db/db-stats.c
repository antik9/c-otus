#include <float.h>
#include <getopt.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>

#define handle_error(msg)     \
    do {                      \
        fprintf(stderr, msg); \
        exit(EXIT_FAILURE);   \
    } while (0)

#define USAGE   \
    "usage: \n" \
    "\t./db-stats --database <db> --table <table> --column <column>\n"

typedef struct _Stats {
    double sum;
    double square_sum;
    double max;
    double min;
    size_t count;
} Stats;

int calculate(void* data, int argc, char** argv, char** column_names) {
    Stats* stats = data;

    double next = 0;
    if (!sscanf(argv[0], "%lf", &next)) return -1;

    if (stats->min > next) stats->min = next;
    if (stats->max < next) stats->max = next;
    stats->sum += next;
    stats->square_sum += next * next;
    stats->count++;

    return 0;
}

void print_stats(char* db_name, char* table_name, char* column_name) {
    sqlite3* db;
    char* err_message = NULL;
    int conn;

    conn = sqlite3_open(db_name, &db);

    if (conn) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return;
    }

    char query[1024] = {0};
    snprintf(query, 1024, "SELECT \"%s\" FROM \"%s\"", column_name, table_name);

    Stats stats = {
        .count = 0,
        .sum = 0.0,
        .min = DBL_MAX,
        .max = -DBL_MAX,
    };
    if (sqlite3_exec(db, query, calculate, &stats, &err_message) != 0) {
        fprintf(stderr, "SQL error: %s\n", err_message);
        sqlite3_free(err_message);
        goto finalize;
    }

    double average = stats.count > 0 ? stats.sum / stats.count : 0;
    printf("%-8s = %.02lf\n", "sum", stats.sum);
    printf("%-8s = %.02lf\n", "average", average);
    printf("%-8s = %.02lf\n", "max", stats.max);
    printf("%-8s = %.02lf\n", "min", stats.min);
    printf("%-8s = %.02lf\n", "variance",
           stats.square_sum / stats.count - average * average);

finalize:
    sqlite3_close(db);
}

int main(int argc, char* argv[]) {
    static struct option long_options[] = {
        {"database", required_argument, 0, 'd'},
        {"table", required_argument, 0, 't'},
        {"column", required_argument, 0, 'c'},
        {0, 0, 0, 0}};

    int opt, option_index;
    char *db_name = NULL, *table_name = NULL, *column_name = NULL;

    while ((opt = getopt_long(argc, argv, "d:t:c:012", long_options,
                              &option_index)) != -1) {
        switch (opt) {
            case 'd':
                db_name = optarg;
                break;
            case 't':
                table_name = optarg;
                break;
            case 'c':
                column_name = optarg;
                break;
            default:
                handle_error(USAGE);
        }
    }

    if (db_name == NULL || table_name == NULL || column_name == NULL)
        handle_error(USAGE);

    print_stats(db_name, table_name, column_name);
}
