#include <stdio.h>

enum Level { DEBUG, INFO, WARNING, ERROR };

typedef struct {
    enum Level level;
    char* filename;
} LoggerConfig;

struct _Logger;

typedef struct _Logger Logger;

void _log_message(Logger* logger, enum Level level, char* message,
                  char* filename, int line);

#define log_debug(logger, message) \
    _log_message(logger, DEBUG, message, __FILE__, __LINE__)

#define log_info(logger, message) \
    _log_message(logger, INFO, message, __FILE__, __LINE__)

#define log_warning(logger, message) \
    _log_message(logger, WARNING, message, __FILE__, __LINE__)

#define log_error(logger, message) \
    _log_message(logger, ERROR, message, __FILE__, __LINE__)

Logger* init_logger(LoggerConfig* config);
void destroy_logger(Logger* logger);
