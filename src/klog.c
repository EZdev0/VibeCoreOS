/*
 * ============================================================
 *  VibeCore OS — Kernel Logging System Implementation
 *
 *  Ring buffer in BSS: survives across exceptions.
 *  All entries timestamped with ms-since-boot.
 *
 *  Format: "[  1234] INFO  | subsystem: message\n"
 *           ^7 ts  ^5 lvl  ^ rest
 * ============================================================
 */

#include "types.h"
#include "kernel.h"
#include "string.h"
#include "timer.h"
#include "klog.h"
#include "interrupt.h"

/* ── Ring Buffer ──────────────────────────────────────────── */

static char  klog_ring[KLOG_RING_SIZE][KLOG_MAX_MSG_LEN];
static u32   klog_write_idx = 0;
static u32   klog_total     = 0;
static bool  klog_ready     = false;

static const char *level_names[] = {
    "DEBUG",
    "INFO ",
    "WARN ",
    "ERROR",
    "FATAL"
};

/* ────────────────────────────────────────────────────────────
 *  klog_init
 *
 *  Zeroes ring buffer, marks logging as ready.
 *  Must be called AFTER uart_init() and timer_init().
 * ────────────────────────────────────────────────────────── */

void klog_init(void)
{
    memset(klog_ring, 0, sizeof(klog_ring));
    klog_write_idx = 0;
    klog_total     = 0;
    klog_ready     = true;



    klog_info("Kernel logging system initialized");

}

/* ────────────────────────────────────────────────────────────
 *  klog
 *
 *  Core logging function. Formats message with timestamp
 *  and level prefix, writes to ring buffer and optionally UART.
 *
 *  UART output policy:
 *    KLOG_DEBUG → ring buffer only (no UART spam)
 *    KLOG_INFO  → ring buffer only (quiet by default)
 *    KLOG_WARN  → ring buffer + UART
 *    KLOG_ERROR → ring buffer + UART
 *    KLOG_FATAL → ring buffer + UART
 * ────────────────────────────────────────────────────────── */


#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wanalyzer-va-list-exhausted"
#endif
void klog(KLogLevel level, const char *fmt, ...)
{
    if (!klog_ready || fmt == NULL) return;


    char buf[KLOG_MAX_MSG_LEN];
    memset(buf, 0, sizeof(buf));

    /* Timestamp prefix */
    size_t pos = snprintf_local(buf, sizeof(buf), "[%6d] %-5s | ",
                             (int)timer_get_ms(),
                             level_names[level]);

    /* User message */
    if (pos < (int)sizeof(buf) - 4) {
        __builtin_va_list args;
        __builtin_va_start(args, fmt);

        /* Simple %s/%d/%u/%x formatter inline */
        while (*fmt && pos < (int)sizeof(buf) - 4) {
            if (*fmt != '%') {
                buf[pos++] = *fmt++;
                continue;
            }
            fmt++;
            switch (*fmt) {
            case 's': {
                const char *s = __builtin_va_arg(args, char*);
                if (s) while (*s && pos < (int)sizeof(buf) - 2) buf[pos++] = *s++;
                break;
            }
            case 'd': {
                i64 n = (i64)__builtin_va_arg(args, int);
                if (n < 0) { buf[pos++] = '-'; n = -n; }
                if (n == 0) { buf[pos++] = '0'; break; }

                char tmp[21]; int ti = 20; tmp[ti] = '\0';
                while (n > 0 && ti > 0) { tmp[--ti] = '0' + (char)(n % 10); n /= 10; }
                while (tmp[ti] && pos < (int)sizeof(buf) - 2) buf[pos++] = tmp[ti++];
                break;
            }
            case 'u': {
                u64 n = (u64)__builtin_va_arg(args, unsigned int);
                if (n == 0) { buf[pos++] = '0'; break; }

                char tmp[21]; int ti = 20; tmp[ti] = '\0';
                while (n > 0 && ti > 0) { tmp[--ti] = '0' + (char)(n % 10); n /= 10; }
                while (tmp[ti] && pos < (int)sizeof(buf) - 2) buf[pos++] = tmp[ti++];
                break;
            }
            case 'x': {
                u64 n = (u64)__builtin_va_arg(args, unsigned int);
                if (n == 0) { buf[pos++] = '0'; break; }

                char tmp[17]; int ti = 16; tmp[ti] = '\0';
                const char *hex = "0123456789abcdef";
                while (n > 0 && ti > 0) { tmp[--ti] = hex[n & 0xF]; n >>= 4; }
                while (tmp[ti] && pos < (int)sizeof(buf) - 2) buf[pos++] = tmp[ti++];
                break;
            }
            default:
                buf[pos++] = *fmt;
                break;
            }
            fmt++;
        }

        __builtin_va_end(args);
    }

    /* Terminate + newline */
    if (pos >= (int)sizeof(buf) - 2) pos = (int)sizeof(buf) - 3;
    buf[pos++] = '\n';
    buf[pos] = '\0';

    /* Write to ring buffer (always) */

    for(size_t i=0; i<KLOG_MAX_MSG_LEN-1 && buf[i]!='\0'; i++) { klog_ring[klog_write_idx % KLOG_RING_SIZE][i] = buf[i]; } klog_ring[klog_write_idx % KLOG_RING_SIZE][KLOG_MAX_MSG_LEN - 1] = '\0';
    klog_write_idx++;
    klog_total++;

    /* Write to UART for WARN and above */
    if (level >= KLOG_WARN) {
        uart_puts(buf);
    }
}

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif


/* ────────────────────────────────────────────────────────────
 *  klog_dump
 *
 *  Dumps the entire ring buffer to UART.
 *  Called by the 'log'/'dmesg' shell command.
 * ────────────────────────────────────────────────────────── */

void klog_dump(void)
{
    uart_puts("\n");
    uart_puts("══════════════════════════════════════════════\n");
    uart_puts("  VibeCore OS — Kernel Log (dmesg)\n");
    uart_printf("  Total entries: %d  |  Ring size: %d\n",
                klog_total, KLOG_RING_SIZE);
    uart_puts("══════════════════════════════════════════════\n");

    if (klog_total == 0) {
        uart_puts("  (no log entries)\n");
        return;
    }

    /* Determine start index (ring buffer may have wrapped) */
    u32 start = 0;
    u32 count = klog_total;
    if (klog_total > KLOG_RING_SIZE) {
        start = (klog_write_idx - KLOG_RING_SIZE) % KLOG_RING_SIZE;
        count = KLOG_RING_SIZE;
    }

    for (u32 i = 0; i < count; i++) {
        u32 idx = (start + i) % KLOG_RING_SIZE;
        if (klog_ring[idx][0] != '\0') {
            uart_puts("  ");
            uart_puts(klog_ring[idx]);
        }
    }

    uart_puts("══════════════════════════════════════════════\n\n");
}

/* ────────────────────────────────────────────────────────────
 *  klog_entry_count / klog_get_entry / klog_get_total_entries
 * ────────────────────────────────────────────────────────── */

u32 klog_entry_count(void)
{
    return klog_total < KLOG_RING_SIZE ? klog_total : KLOG_RING_SIZE;
}

const char *klog_get_entry(int index)
{
    if (index < 0) return NULL;
    u32 start = (klog_total > KLOG_RING_SIZE)
              ? (klog_write_idx - KLOG_RING_SIZE) % KLOG_RING_SIZE
              : 0;
    u32 count = klog_entry_count();
    if ((u32)index >= count) return NULL;
    u32 idx = (start + (u32)index) % KLOG_RING_SIZE;
    return klog_ring[idx][0] ? klog_ring[idx] : NULL;
}

int klog_get_total_entries(void)
{
    return (int)klog_total;
}
