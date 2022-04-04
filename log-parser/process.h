#include "stats.h"

#define DONE "done"

#define MSG_FLAGS MSG_NOERROR
#define QUEUE_NAME "q-logfiles"
#define QUEUE_MESSAGE_KEY 37

typedef struct _ProcessFilesArgs {
    LogFileStats** stats;
    int number_of_files;
} ProcessFilesArgs;

void* process_log_files(void* _args);
