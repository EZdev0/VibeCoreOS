#ifndef _CRC32_H
#define _CRC32_H

#include "types.h"

u32 crc32_compute(const void *data, size_t length);
bool crc32_self_test(void);

#endif
