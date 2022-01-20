#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#define clean_and_return \
    {                    \
        fclose(file);    \
        return (0);      \
    }

#define read_char(next, file) \
    if (!fread(next, 1, 1, file)) clean_and_return
#define read_uint16_t(next, file) \
    if (!fread(next, 2, 1, file)) clean_and_return
#define read_uint32_t(next, file) \
    if (!fread(next, 4, 1, file)) clean_and_return

const char* signature = "\x50\x4b\x03\x04";

struct LocalFileHeader {
    uint32_t signature;
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
    uint8_t* filename;
    uint8_t* extraField;
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
            read_uint16_t(&header.versionToExtract, file);
            read_uint16_t(&header.generalPurposeBitFlag, file);
            read_uint16_t(&header.compressionMethod, file);
            read_uint16_t(&header.modificationTime, file);
            read_uint16_t(&header.modificationDate, file);
            read_uint32_t(&header.crc32, file);
            read_uint32_t(&header.compressedSize, file);
            read_uint32_t(&header.uncompressedSize, file);
            read_uint16_t(&header.filenameLength, file);
            read_uint16_t(&header.extraFieldLength, file);

            // read filename
            if (read_filename(file) < 0) clean_and_return;
        }
    }

    clean_and_return;
}
