/*
 * FLZ4 Uncompress utility
 *
 * Unpacks Fortinet LZ4 compressed files.
 *
 * Copyright(C)2025 Valerio Santinelli
 * Licensed under the MIT License.
 * See LICENSE file in the project root for full license information.
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <lz4file.h>

#include "lz4.h"


// This is fixed for Fortinet files. It is always 4 butes.
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
#define MAGIC_NUMBER (0x345a4c46)

static int decode_next_block(FILE* in, int compressedLen, FILE* out)
{
    char compressedBuffer[COMPRESSED_BUF_SIZE];
    char decompressedBuffer[DECOMPRESSED_BUF_SIZE];
    size_t readBytes = fread(compressedBuffer, 1, compressedLen, in);
    if (readBytes == 0)
    {
        return ERR_STREAM_READ_FAILURE;
    }
    if (readBytes != compressedLen)
    {
        return ERR_INVALID_COMPRESSED_LEN;
    }

    int decompressedLen = LZ4_decompress_safe(compressedBuffer, decompressedBuffer, compressedLen,
                                              DECOMPRESSED_BUF_SIZE);
    size_t bytesWritten = fwrite(decompressedBuffer, 1, decompressedLen, out);
    if (bytesWritten <= 0)
    {
        return ERR_STREAM_WRITE_FAILURE;
    }
    return 0;
}

static int to_little_endian(char buf[4])
{
    int res = ((unsigned char)buf[0]) |
        ((unsigned char)buf[1] << 8) |
        ((unsigned char)buf[2] << 16) |
        ((unsigned char)buf[3] << 24);
    return res;
}


static int decompress_file(FILE* f_in, FILE* f_out)
{
    assert(f_in != NULL);
    assert(f_out != NULL);

    int good_blocks = 0;
    char compressedLenBytes[4];
    if (fread(&compressedLenBytes, 1, 4, f_in) != 4)
    {
        printf("Cannot read the beginning of the file\n");
        return 1;
    }
    int magicNumber = to_little_endian(compressedLenBytes);
    if (magicNumber != MAGIC_NUMBER)
    {
        printf("Invalid magic number\n");
        return 1;
    }
    while (fread(&compressedLenBytes, 1, 4, f_in) == 4)
    {
        int compressedLen = to_little_endian(compressedLenBytes);
        if (decode_next_block(f_in, compressedLen, f_out) == 0)
        {
            good_blocks++;
        }
        else
        {
            printf("Some invalid compressed length encountered!\n");
            return 1;
        }
    }

    return 0;
}

int main(int argc, const char** argv)
{
    char inpFilename[1024] = {0};
    char decFilename[1024] = {0};

    if (argc < 2)
    {
        printf("Please specify the input filename\n");
        return 0;
    }

    snprintf(inpFilename, 1024, "%s", argv[1]);
    snprintf(decFilename, 1024, "%s.dec", argv[1]);

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

        if (ret)
        {
            printf("compression error\n");
            return 1;
        }

        printf("decompress : done\n");
    }
}
