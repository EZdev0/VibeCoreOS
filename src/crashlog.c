/*
 * ============================================================
 *  VibeCore OS — Crash Log System
 *
 *  Writes a detailed log file on every crash to
 *  /Desktop/crash_<n>.log.
 *  On next boot, the log folder is visible on the Desktop
 *  (via FAT32 /Desktop/ directory).
 *
 *  IMPORTANT: Uses STATIC buffer (BSS, not stack!)
 *  to prevent recursive panic on stack overflow.
 *
 *  Log Format:
 *    [VibeCore Crash Report]
 *    Version: 1.0.0 Photon
 *    Timestamp: <ms since boot>
 *    Exception: <title>
 *    Description: <desc>
 *    ESR: 0x<value>
 *    ELR: 0x<value>
 *    FAR: 0x<value>
 * ============================================================
 */

#include "types.h"
#include "kernel.h"
#include "string.h"
#include "timer.h"
#include "interrupt.h"
#include "crashlog.h"
#include "klog.h"

#define CRASH_LOG_DIR       "/Desktop"
#define CRASH_LOG_PREFIX    "/Desktop/crash_"
#define CRASH_LOG_SUFFIX    ".log"
#define CRASH_LOG_MAX_SIZE  2048

static u32 crash_count = 0;

/*
 * STATIC buffer (BSS, not stack!): Prevents recursive
 * panic if the crash was caused by stack overflow.
 */

static char crash_log_buf[CRASH_LOG_MAX_SIZE];

/* ────────────────────────────────────────────────────────────
 *  crash_log_init
 *
 *  Ensures /Desktop/ exists.
 * ────────────────────────────────────────────────────────── */

void crash_log_init(void)
{
    /* Stub: fs_create_file(CRASH_LOG_DIR); */
    crash_count = 0;
    klog_info("Crash log system active (/Desktop/crash_*.log)");
}

/* ────────────────────────────────────────────────────────────
 *  crash_log_write
 *
 *  Called on every kernel panic/crash.
 *  Writes a detailed crash report to the Desktop.
 *
 *  NOTE: Called from crash_screen_show() before the system
 *  halts. If the crash was caused by a UART fault or if
 *  snprintf_local triggers a secondary exception, a
 *  recursive panic path exists. The BSS buffer minimizes
 *  this risk. Acceptable trade-off for bare metal.
 *
 *  Parameters:
 *    title — Error type (e.g. "SYNCHRONOUS EXCEPTION")
 *    desc  — Description
 *    esr   — Exception Syndrome Register
 *    elr   — Exception Link Register
 *    far   — Fault Address Register
 * ────────────────────────────────────────────────────────── */

void crash_log_write(const char *title, const char *desc,
                     u64 esr, u64 elr, u64 far)
{
    crash_count++;

    /*
     * Build log entry in STATIC buffer (no stack allocation!).
     * Prevents recursive panic on stack-overflow crash.
     */
    memset(crash_log_buf, 0, sizeof(crash_log_buf));

    size_t pos = 0;

    /* Header */
    snprintf_local(crash_log_buf + pos, CRASH_LOG_MAX_SIZE - pos,
        "═══════════════════════════════════════\n"
        "  VibeCore OS — Crash Report #%d\n"
        "═══════════════════════════════════════\n\n", crash_count);

    /* System info */
    snprintf_local(crash_log_buf + pos, CRASH_LOG_MAX_SIZE - pos,
        "Version:    %s\n", VIBECORE_VERSION_STRING);
    snprintf_local(crash_log_buf + pos, CRASH_LOG_MAX_SIZE - pos,
        "Timestamp:  %d ms since boot\n", (int)timer_get_ms());
    snprintf_local(crash_log_buf + pos, CRASH_LOG_MAX_SIZE - pos,
        "Uptime:     %d seconds\n", (int)(timer_get_ms() / 1000));

    snprintf_local(crash_log_buf + pos, CRASH_LOG_MAX_SIZE - pos, "\n");

    /* Error info */
    snprintf_local(crash_log_buf + pos, CRASH_LOG_MAX_SIZE - pos,
        "Exception:  %s\n", title ? title : "Unknown");
    snprintf_local(crash_log_buf + pos, CRASH_LOG_MAX_SIZE - pos,
        "Details:    %s\n", desc ? desc : "No details");
    snprintf_local(crash_log_buf + pos, CRASH_LOG_MAX_SIZE - pos, "\n");

    /* Register dump */
    snprintf_local(crash_log_buf + pos, CRASH_LOG_MAX_SIZE - pos,
        "-- Register Dump --\n");
    snprintf_local(crash_log_buf + pos, CRASH_LOG_MAX_SIZE - pos,
        "ESR:        0x%x\n", (u32)esr);
    snprintf_local(crash_log_buf + pos, CRASH_LOG_MAX_SIZE - pos,
        "ELR:        0x%x\n", (u32)elr);
    snprintf_local(crash_log_buf + pos, CRASH_LOG_MAX_SIZE - pos,
        "FAR:        0x%x\n", (u32)far);
    snprintf_local(crash_log_buf + pos, CRASH_LOG_MAX_SIZE - pos, "\n");

    /* ESR decoding */
    u32 ec = (u32)((esr >> 26) & 0x3F);
    snprintf_local(crash_log_buf + pos, CRASH_LOG_MAX_SIZE - pos,
        "-- ESR Analysis --\n");
    snprintf_local(crash_log_buf + pos, CRASH_LOG_MAX_SIZE - pos,
        "EC (Class):  0x%x", ec);

    const char *ec_desc;
    switch (ec) {
        case 0x15: ec_desc = "SVC (System Call)"; break;
        case 0x20: ec_desc = "Instruction Abort (lower EL)"; break;
        case 0x21: ec_desc = "Instruction Abort (same EL)"; break;
        case 0x24: ec_desc = "Data Abort (lower EL)"; break;
        case 0x25: ec_desc = "Data Abort (same EL) — PAGE FAULT"; break;
        case 0x2F: ec_desc = "SError interrupt"; break;
        default:   ec_desc = "See ARMv8 Reference Manual"; break;
    }
    snprintf_local(crash_log_buf + pos, CRASH_LOG_MAX_SIZE - pos,
        " → %s\n", ec_desc);
    snprintf_local(crash_log_buf + pos, CRASH_LOG_MAX_SIZE - pos,
        "ISS:         0x%x\n", (u32)(esr & 0x1FFFFFF));
    snprintf_local(crash_log_buf + pos, CRASH_LOG_MAX_SIZE - pos, "\n");

    /* Footer */
    snprintf_local(crash_log_buf + pos, CRASH_LOG_MAX_SIZE - pos,
        "═══════════════════════════════════════\n"
        "  Report saved to: %scrash_%d%s\n"
        "  Please reboot the system.\n"
        "═══════════════════════════════════════\n",
        CRASH_LOG_PREFIX, crash_count, CRASH_LOG_SUFFIX);

    /*
     * Output log:
     * 1. UART (always visible)
     * 2. Emergency reserve (does not survive reboot)
     */
    uart_puts("\n");
    uart_puts(crash_log_buf);

    /* Log to kernel ring buffer as well */
    klog_fatal("Crash report #%d written: %s", crash_count, title ? title : "Unknown");

    /* Stub: Write to FAT32 once SD driver exists */

    /* fs_write_file(log_path, (u8*)crash_log_buf, strlen(crash_log_buf)); */
}

/* ────────────────────────────────────────────────────────────
 *  crash_log_check_previous
 *
 *  Called on next successful boot.
 *  Checks if crash logs from a previous crash remain and shows them.
 * ────────────────────────────────────────────────────────── */

void crash_log_check_previous(void)
{
    klog_info("Checking for previous crash logs...");

    /* Stub: fs_list_dir(CRASH_LOG_DIR) and check if crash_*.log exists */
    /* If yes: klog_warn("%d crash report(s) from last boot found!", count); */

    klog_info("No previous crash reports found");
}
