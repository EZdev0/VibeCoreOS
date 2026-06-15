/*
 * ============================================================
 *  VibeCore OS — CRC32 Header
 * ============================================================
 */

#ifndef _CRC32_H
#define _CRC32_H

#include "types.h"

u32  crc32_compute(const u8 *data, size_t size, u32 prev);
u32  crc32_fast(const u8 *data, size_t size, u32 prev);
bool crc32_verify_file(const char *path, const u8 *data, size_t size, u32 expected_crc);
bool crc32_self_test(void);

#endif /* _CRC32_H */
