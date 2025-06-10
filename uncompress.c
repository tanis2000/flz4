/* LZ4file API example : compress a file
 * Modified from an example code by anjiahao
 *
 * This example will demonstrate how
 * to manipulate lz4 compressed files like
 * normal files */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <lz4file.h>

#include "lz4.h"


// #define CHUNK_SIZE (16*1024)
#define CHUNK_SIZE (4)

#define ERR_DIRECTORY_CREATE_FAILURE (-8)
#define ERR_FILE_COPY_FAILURE (-7)
#define ERR_FILE_DELETE_FAILURE (-6)
#define ERR_FILE_NOT_FOUND (-3)
#define ERR_INVALID_COMPRESSED_LEN (-1)
#define ERR_RM_DIR_FAILURE (-2)
#define ERR_STREAM_READ_FAILURE (-4)
#define ERR_STREAM_WRITE_FAILURE (-5)
#define COMPRESSED_OFFSET (4)
#define COMPRESSED_BUF_SIZE (65536)
#define DECOMPRESSED_BUF_SIZE (65536)

static size_t get_file_size(char *filename)
{
    struct stat statbuf;

    if (filename == NULL) {
        return 0;
    }

    if(stat(filename,&statbuf)) {
        return 0;
    }

    return (size_t)statbuf.st_size;
}

static int print_next_block(FILE *in, int compressedLen, FILE *out) {
    char compressedBuffer[COMPRESSED_BUF_SIZE];
    char decompressedBuffer[DECOMPRESSED_BUF_SIZE];
    size_t readBytes = fread(compressedBuffer, 1, compressedLen, in);
    if (readBytes == 0) {
        return ERR_STREAM_READ_FAILURE;
    }
    if (readBytes != compressedLen) {
        return ERR_INVALID_COMPRESSED_LEN;
    }

    int decompressedLen = LZ4_decompress_safe(compressedBuffer, decompressedBuffer, compressedLen, DECOMPRESSED_BUF_SIZE);
    size_t bytesWritten = fwrite(decompressedBuffer, 1, decompressedLen, out);
    if (bytesWritten <= 0) {
        return ERR_STREAM_WRITE_FAILURE;
    }
    return 0;
}


static int decompress_file(FILE* f_in, FILE* f_out)
{
    assert(f_in != NULL); assert(f_out != NULL);

    LZ4F_errorCode_t ret = LZ4F_OK_NoError;
    LZ4_readFile_t* lz4fRead;
    void* const buf= malloc(CHUNK_SIZE);
    if (!buf) {
        printf("error: memory allocation failed \n");
        return 1;
    }

    int good_blocks = 0;
    char compressedLenBytes[4];
    while (fread(&compressedLenBytes, 1, 4, f_in) == 4) {
        int compressedLen = ((unsigned char)compressedLenBytes[0]) |
                            ((unsigned char)compressedLenBytes[1] << 8) |
                            ((unsigned char)compressedLenBytes[2] << 16) |
                            ((unsigned char)compressedLenBytes[3] << 24);
            if (print_next_block(f_in, compressedLen, f_out) == 0) {
                good_blocks++;
            }else {
                printf("Some invalid compressed length encountered!\n");
                return 1;
        }
    }

    // ret = LZ4F_readOpen(&lz4fRead, f_in);
    // if (LZ4F_isError(ret)) {
    //     printf("LZ4F_readOpen error: %s\n", LZ4F_getErrorName(ret));
    //     free(buf);
    //     return 1;
    // }
//
//     while (1) {
//         ret = LZ4F_read(lz4fRead, buf, CHUNK_SIZE);
//         if (LZ4F_isError(ret)) {
//             printf("LZ4F_read error: %s\n", LZ4F_getErrorName(ret));
//             goto out;
//         }
//
//         /* nothing to read */
//         if (ret == 0) {
//             break;
//         }
//
//         if(fwrite(buf, 1, ret, f_out) != ret) {
//             printf("write error!\n");
//             goto out;
//         }
//     }
//
// out:
//     free(buf);
//     if (LZ4F_isError(LZ4F_readClose(lz4fRead))) {
//         printf("LZ4F_readClose: %s\n", LZ4F_getErrorName(ret));
//         return 1;
//     }
//
//     if (ret) {
//         return 1;
//     }

    return 0;
}

int main(int argc, const char **argv) {
    char inpFilename[256] = { 0 };
    char decFilename[256] = { 0 };

    if (argc < 2) {
        printf("Please specify input filename\n");
        return 0;
    }

    snprintf(inpFilename, 256, "%s", argv[1]);
    snprintf(decFilename, 256, "%s.dec", argv[1]);

    printf("inp = [%s]\n", inpFilename);
    printf("dec = [%s]\n", decFilename);

    /* decompress */
    {
        FILE* const inpFp = fopen(inpFilename, "rb");
        FILE* const outFp = fopen(decFilename, "wb");

        if (inpFp == NULL) exit(1);
        if (outFp == NULL) exit(1);

        printf("decompress : %s -> %s\n", inpFilename, decFilename);
        int ret = decompress_file(inpFp, outFp);

        fclose(outFp);
        fclose(inpFp);

        if (ret) {
            printf("compression error: %s\n", LZ4F_getErrorName(ret));
            return 1;
        }

        printf("decompress : done\n");
    }


}
