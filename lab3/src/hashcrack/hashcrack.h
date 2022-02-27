#pragma once

#include <stdint.h>

#include "md5.h"

int crack(MD5Hash data, char* result, size_t size);
