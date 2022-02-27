#include "hashcrack.h"

#include <math.h>
#include <string.h>

#define ASCII_START 32
#define ASCII_END 126

static void inc_num_str(char* str, size_t size) {
    str[size-1]++;

    for (char* pos = str + size - 1; pos >= str; pos--) {
        if (*pos <= ASCII_END)
            break;

        *pos = ASCII_START;

        if (pos != str) {
            pos[-1]++;
        }
    }
}

#include <stdio.h>

int crack(MD5Hash data, char* result, size_t size) {
    memset(result, ASCII_START, size);

    for (int len = 0; len <= size; len++) {
        unsigned long options = pow((ASCII_END - ASCII_START + 1), len);

        for (unsigned long i = 0; i < options; i++) {
            MD5Hash hash = md5((uint8_t*) result, len);

            if (md5_comp(&data,  &hash) == 0) {
                return len;
            }

            inc_num_str(result, len);
        }
    }

    return 0;
}
