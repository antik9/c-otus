#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#define read_char(next, file) \
    if (!fread(next, 1, 1, file)) goto exit

const char* signature = "\x50\x4b\x03\x04";

struct __attribute__((__packed__)) LocalFileHeader {
    uint16_t versionToExtract;
    uint16_t generalPurposeBitFlag;
    uint16_t compressionMethod;
    uint16_t modificationTime;
    uint16_t modificationDate;
    uint32_t crc32;
    uint32_t compressedSize;
    uint32_t uncompressedSize;
    uint16_t filenameLength;
    uint16_t extraFieldLength;
} header;

void error(char* msg) {
    puts(msg);
    exit(1);
}

int read_filename(FILE* file) {
    char* buf = malloc(header.filenameLength + 1);
    buf[header.filenameLength] = '\0';

    if (!fread(buf, 1, header.filenameLength, file)) {
        free(buf);
        return -1;
    }
    puts(buf);
    free(buf);
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc != 2) error("usage:\n    rarjpeg <file>");

    char* filename = argv[1];
    FILE* file = fopen(filename, "r");
    if (file == NULL) error("cannot open the file");

    char next;
    while (fread(&next, 1, 1, file)) {
        while (1) {
            // check signature
            if (next != signature[0]) break;
            read_char(&next, file);
            if (next != signature[1]) continue;
            read_char(&next, file);
            if (next != signature[2]) continue;
            read_char(&next, file);
            if (next != signature[3]) continue;

            // read header struct
            if (!fread(&header, 26, 1, file)) goto exit;

            // read filename
            if (read_filename(file) < 0) goto exit;
        }
    }

exit:
    fclose(file);
    return (0);
}
