#ifndef SHARED_RANDOM
#define SHARED_RANDOM

#include <stdlib.h>

#define CHARSET_NUMERIC "0123456789"
#define CHARSET_HEX CHARSET_NUMERIC "abcdef"
#define CHARSET_ALPHA_LOWER  "abcdefghijklmnopqrstuvwxyz"
#define CHARSET_ALPHA_UPPPER "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define CHARSET_ALPHA_NOCASE CHARSET_ALPHA_LOWER CHARSET_ALPHA_UPPPER
#define CHARSET_SIGNS "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"
#define CHARSET_SPACES " \t\n\r\x0b\x0c"
#define CHARSET_ANY CHARSET_NUMERIC CHARSET_ALPHA_NOCASE CHARSET_SIGNS CHARSET_SPACES

void random_init(void);

void random_str(char* str, size_t len, const char* charset);

#endif
