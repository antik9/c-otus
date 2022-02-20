#include "./logger.h"

#include <execinfo.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define _DEBUG "DEBUG"
#define _INFO "INFO"
#define _WARNING "WARNING"
#define _ERROR "ERROR"

#define TIME_BUFFER_SIZE 32
#define BACKTRACE_BUFFER_SIZE 256
#define BASIC_ERROR_MESSAGE_SIZE 1024

Logger* init_logger(LoggerConfig* config) {
    Logger* logger = malloc(sizeof(Logger));
    if (logger == NULL) return NULL;

    logger->level = config->level;
    logger->file = fopen(config->filename, "a");
    if (logger->file == NULL) {
        free(logger);
        return NULL;
    }
    return logger;
}

void destroy_logger(Logger* logger) {
    fclose(logger->file);
    free(logger);
}

char* level_as_str(enum Level level) {
    switch (level) {
        case DEBUG:
            return _DEBUG;
        case INFO:
            return _INFO;
        case WARNING:
            return _WARNING;
        case ERROR:
            return _ERROR;
        default:
            return NULL;
    }
}

void _log_message(Logger* logger, enum Level level, char* message,
                  char* filename, int line) {
    if (level < logger->level) return;

    time_t rawtime;
    struct tm* timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    char timebuf[TIME_BUFFER_SIZE] = {0};
    strftime(timebuf, TIME_BUFFER_SIZE, "%Y-%m-%d %H:%M:%S", timeinfo);

#define simple_log_and_return                                  \
    {                                                          \
        fprintf(logger->file, "[%s] %s (%s:%d) %s\n", timebuf, \
                level_as_str(level), filename, line, message); \
        return;                                                \
    }

    if (level < ERROR) simple_log_and_return;

    void* backtrace_buffer[BACKTRACE_BUFFER_SIZE];
    int nptrs = backtrace(backtrace_buffer, BACKTRACE_BUFFER_SIZE);
    char** calls = backtrace_symbols(backtrace_buffer, nptrs);
    if (calls == NULL) simple_log_and_return;

    char error_message[BASIC_ERROR_MESSAGE_SIZE] = {0};
    if (!snprintf(error_message, BASIC_ERROR_MESSAGE_SIZE,
                  "[%s] %s (%s:%d) %s\n", timebuf, level_as_str(level),
                  filename, line, message)) {
        free(calls);
        simple_log_and_return;
    }

    for (int i = 0; i < nptrs; ++i)
        snprintf(error_message + strlen(error_message),
                 BASIC_ERROR_MESSAGE_SIZE - strlen(error_message), "\t%s\n",
                 calls[i]);
    fprintf(logger->file, "%s", error_message);

    free(calls);
#undef simple_log_and_return
}
