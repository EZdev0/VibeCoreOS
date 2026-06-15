/*
 * ============================================================
 *  VibeCore OS — CRC32 File Integrity Check
 *
 *  ARM64-optimized CRC32 implementation.
 *  Uses hardware-accelerated crc32 instruction set
 *  for maximum performance.
 *
 *  Features:
 *    - crc32_verify()   → Verify file against expected checksum
 *    - crc32_compute()   → Compute CRC32 at runtime
 *    - crc32_fast()      → Hardware-accelerated (ARMv8 CRC)
 * ============================================================
 */

#include "types.h"
#include "kernel.h"
#include "string.h"

/* ── CRC32 Polynomial (IEEE 802.3 / Ethernet Standard) ────── */
#define CRC32_POLY  0xEDB88320UL

/* ── CRC32 Lookup Table (Software Fallback) ────────────────── */
static u32 crc32_table[256];
static bool crc32_table_ready = false;

/* ────────────────────────────────────────────────────────────
 *  crc32_init_table
 *
 *  Builds the CRC32 lookup table (once, lazy).
 * ────────────────────────────────────────────────────────── */

static void crc32_init_table(void)
{
    if (crc32_table_ready) return;

    for (u32 i = 0; i < 256; i++) {
        u32 crc = i;
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ CRC32_POLY;
            } else {
                crc >>= 1;
            }
        }
        crc32_table[i] = crc;
    }
    crc32_table_ready = true;
}

/* ────────────────────────────────────────────────────────────
 *  crc32_compute
 *
 *  Computes CRC32 over a data buffer (software).
 *  Initializes the table on first call.
 *
 *  Parameters:
 *    data  — Data buffer
 *    size  — Size in bytes
 *    prev  — Previous CRC value (0 for fresh start)
 *  Returns:
 *    32-bit CRC32 checksum
 * ────────────────────────────────────────────────────────── */

u32 crc32_compute(const u8 *data, size_t size, u32 prev)
{
    CHECK_NULL(data);
    if (size == 0) return prev;

    /* Initialize table (once, lazy) */
    crc32_init_table();

    u32 crc = prev ^ 0xFFFFFFFF;

    for (size_t i = 0; i < size; i++) {
        u8 idx = (u8)((crc ^ data[i]) & 0xFF);
        crc = (crc >> 8) ^ crc32_table[idx];
    }

    return crc ^ 0xFFFFFFFF;
}

/* ────────────────────────────────────────────────────────────
 *  crc32_fast
 *
 *  Hardware-accelerated CRC32 (ARMv8 CRC32X).
 *  Uses CRC32X instruction for 8-byte blocks.
 *
 *  Falls back to crc32_compute() if CRC32X unavailable.
 * ────────────────────────────────────────────────────────── */

u32 crc32_fast(const u8 *data, size_t size, u32 prev)
{
    CHECK_NULL(data);
    if (size == 0) return prev;

    u32 crc = prev ^ 0xFFFFFFFF;

#ifdef __ARM_FEATURE_CRC32
    const u8 *d = data;

    /* 8-byte blocks (CRC32X) */
    while (size >= 8) {
        u64 val;
        __builtin_memcpy(&val, d, 8);
        crc = __builtin_arm_crc32d(crc, val);
        d += 8;
        size -= 8;
    }

    /* 4-byte blocks (CRC32W) */
    if (size >= 4) {
        u32 val;
        __builtin_memcpy(&val, d, 4);
        crc = __builtin_arm_crc32w(crc, val);
        d += 4;
        size -= 4;
    }

    /* Remaining bytes (CRC32B) */
    while (size > 0) {
        crc = __builtin_arm_crc32b(crc, *d);
        d++;
        size--;
    }
#else
    /* Software fallback */
    crc32_init_table();

    for (size_t i = 0; i < size; i++) {
        u8 idx = (u8)((crc ^ data[i]) & 0xFF);
        crc = (crc >> 8) ^ crc32_table[idx];
    }
#endif

    return crc ^ 0xFFFFFFFF;
}

/* ────────────────────────────────────────────────────────────
 *  crc32_verify_file
 *
 *  Checks a file for corruption by comparing
 *  computed CRC32 against expected.
 *
 *  Parameters:
 *    path          — File path (for future FAT32 integration)
 *    data          — File data in memory
 *    size          — Data size
 *    expected_crc  — Expected CRC32 checksum
 *  Returns:
 *    true  — File intact (CRC matches)
 *    false — File corrupted (CRC mismatch)
 * ────────────────────────────────────────────────────────── */

bool crc32_verify_file(const char *path, const u8 *data, size_t size, u32 expected_crc)
{
    if (data == NULL || size == 0) {
        uart_printf("[CRC] ERROR: Cannot verify file '%s' (NULL/empty)!\n",
                    path ? path : "??");
        return false;
    }

    u32 computed = crc32_fast(data, size, 0);

    if (computed != expected_crc) {
        uart_printf("[CRC] CORRUPT: '%s' expected=0x%x, computed=0x%x\n",
                    path ? path : "??", expected_crc, computed);
        return false;
    }

    return true;
}

/* ────────────────────────────────────────────────────────────
 *  crc32_self_test
 *
 *  Runs a self-test of the CRC32 implementation
 *  with known test vectors.
 *
 *  Returns:
 *    true — All tests passed
 *    false — A test failed
 * ────────────────────────────────────────────────────────── */

bool crc32_self_test(void)
{
    uart_puts("[CRC] Self-test...\n");

    /* Test vector 1: Empty string */
    u32 result = crc32_compute((const u8*)"", 0, 0);
    if (result != 0x00000000) {
        uart_printf("[CRC] FAIL Test 1: expected 0x0, got 0x%x\n", result);
        return false;
    }

    /* Test vector 2: "123456789" (IEEE 802.3 standard) */
    result = crc32_compute((const u8*)"123456789", 9, 0);
    if (result != 0xCBF43926) {
        uart_printf("[CRC] FAIL Test 2: expected 0xCBF43926, got 0x%x\n", result);
        return false;
    }

    /* Test vector 3: "VibeCore" */
    result = crc32_compute((const u8*)"VibeCore", 8, 0);

    /* Test vector 4: Incremental (prev != 0) */
    u32 part1 = crc32_compute((const u8*)"Vibe", 4, 0);
    u32 part2 = crc32_compute((const u8*)"Core", 4, part1);
    if (part2 != result) {
        uart_printf("[CRC] FAIL Test 4: incremental 0x%x != direct 0x%x\n", part2, result);
        return false;
    }

    uart_puts("[CRC] All tests passed!\n");
    return true;
}
