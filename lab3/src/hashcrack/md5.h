#pragma once

#include <stdint.h>
#include <stdlib.h>

typedef struct {
    uint32_t digest[4];
} MD5Hash;

MD5Hash md5(const uint8_t *initial_msg, size_t initial_len);

int md5_comp(const MD5Hash* left, const MD5Hash* right);

#define MD5_HEX_LEN 32

int md5_from_str(char str[MD5_HEX_LEN], MD5Hash* hash);
void md5_to_str(char str[MD5_HEX_LEN], MD5Hash* hash);
