#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct MemoryChunk {
    char* memory;
    size_t size;
};

size_t write_cb(void* contents, size_t size, size_t nmemb, void* userp);
