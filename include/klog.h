/*
 * ============================================================
 *  VibeCore OS — Kernel Logging System (klog)
 *
 *  Provides structured, leveled logging with a ring buffer
 *  that survives across crashes. Ring buffer is in BSS
 *  (no heap allocation) for crash-safety.
 *
 *  Log Levels (inspired by Linux kmsg):
 *    KLOG_DEBUG  — Developer debug messages
 *    KLOG_INFO   — Normal operational messages
 *    KLOG_WARN   — Warning conditions
 *    KLOG_ERROR  — Error conditions (non-fatal)
 *    KLOG_FATAL  — Fatal errors (pre-crash)
 *
 *  Features:
 *    - Ring buffer (4KB, ~64-128 entries)
 *    - Timestamp prefix (ms since boot)
 *    - Auto-flush to UART at KLOG_WARN and above
 *    - Queryable via 'log' / 'dmesg' shell command
 *    - Included in crash reports
 * ============================================================
 */

#ifndef _KLOG_H
#define _KLOG_H

#include "types.h"

/* Log levels */
typedef enum {
    KLOG_DEBUG = 0,
    KLOG_INFO  = 1,
    KLOG_WARN  = 2,
    KLOG_ERROR = 3,
    KLOG_FATAL = 4
} KLogLevel;

/* Maximum length of a single log entry */
#define KLOG_MAX_MSG_LEN    128

/* Ring buffer: 128 entries × 128 bytes = 16 KB */
#define KLOG_RING_SIZE      128

/* ── API ─────────────────────────────────────────────────── */

/* Initialize the logging subsystem */
void klog_init(void);

/* Core logging function */
void klog(KLogLevel level, const char *fmt, ...);

/* Convenience macros */
#define klog_debug(fmt, ...)  klog(KLOG_DEBUG, fmt, ##__VA_ARGS__)
#define klog_info(fmt, ...)   klog(KLOG_INFO,  fmt, ##__VA_ARGS__)
#define klog_warn(fmt, ...)   klog(KLOG_WARN,  fmt, ##__VA_ARGS__)
#define klog_error(fmt, ...)  klog(KLOG_ERROR, fmt, ##__VA_ARGS__)
#define klog_fatal(fmt, ...)  klog(KLOG_FATAL, fmt, ##__VA_ARGS__)

/* Dump the ring buffer to UART (called by 'log' shell command) */
void klog_dump(void);

/* Return number of entries currently in the ring buffer */
u32 klog_entry_count(void);

/* Export ring buffer for crash reports */
const char *klog_get_entry(int index);
int  klog_get_total_entries(void);

#endif /* _KLOG_H */
