#include "random.h"

#include <string.h>
#include <time.h>

void random_init(void) {
    srand(time(NULL));
}

void random_str(char* str, size_t len, const char* charset) {
    size_t charset_len = strlen(charset);

    for (int i = 0; i < len; i++) {
        size_t pos = rand() % charset_len;
        str[i] = charset[pos];
    }
}
