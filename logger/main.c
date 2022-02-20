#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "./logger.h"

#define NUM_THREADS 10

void inner(Logger* logger) { log_error(logger, "with stack"); }

void* log_messages(void* param) {
    Logger* logger = param;
    for (int i = 0; i < 10; i++) {
        log_warning(logger, "it's a warning");
        log_info(logger, "it's an info");
        log_debug(logger, "shouldn't be logged");
        inner(logger);
    }
    return 0;
}

int main() {
    LoggerConfig config = {
        .level = INFO,
        .filename = "/tmp/custom.log.1",
    };

    Logger* logger = init_logger(&config);
    if (logger == NULL) {
        fprintf(stderr, "cannot initialize logger\n");
        exit(1);
    }

    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; ++i)
        pthread_create(&threads[i], NULL, log_messages, logger);

    for (int i = 0; i < NUM_THREADS; ++i) pthread_join(threads[i], NULL);

    destroy_logger(logger);
    return 0;
}
