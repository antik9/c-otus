#ifndef __SERVE_H__
#define __SERVE_H__

typedef struct _ServeArgs {
    char* address;
    char* directory;
} ServeArgs;

void* serve();
#endif
