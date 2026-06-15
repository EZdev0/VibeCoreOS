/*
 * ============================================================
 *  VibeCore OS — Kernel Main (kernel_main)
 *
 *  Initializes all subsystems in ordered sequence:
 *    1. UART (debug output)
 *    2. Kernel logging (klog)
 *    3. Timer (scheduler tick)
 *    4. Framebuffer (graphics)
 *    5. Memory allocator
 *    6. Scheduler
 *    7. MMU
 *    8. Filesystem
 *    9. GUI / Desktop
 *   10. Crash Log + Setup Wizard
 *   11. Shell (interactive)
 *
 *  Called from boot.S and NEVER returns.
 * ============================================================
 */

#include "types.h"
#include "kernel.h"
#include "peripherals.h"
#include "string.h"
#include "framebuffer.h"
#include "mailbox.h"
#include "allocator.h"
#include "timer.h"
#include "mmu.h"
#include "scheduler.h"
#include "fs.h"
#include "gui.h"
#include "shell.h"
#include "boot_anim.h"
#include "recovery.h"
#include "setup.h"
#include "crashlog.h"
#include "klog.h"

/* ── Forward Declarations ─────────────────────────────────── */
void timer_init(void);
void timer_sleep_ms(u32 ms);
void mmu_init(void);
void allocator_init(void);
void scheduler_init(void);
void gui_desktop_init(void);
void shell_run(void);
void fs_init(void);

/* ────────────────────────────────────────────────────────────
 *  kernel_main
 *
 *  Main entry point after boot process.
 *  All subsystems are initialized sequentially.
 *  On error, panic() is called.
 * ────────────────────────────────────────────────────────── */

void kernel_main(void)
{
    /*
     * Phase 0: UART (debug output)
     * Initialized first so we can see error messages.
     */
    uart_init();
    uart_puts("\r\n[OK] VibeCore OS Kernel Booted!\r\n");
    uart_puts("\n\n");
    uart_puts("╔══════════════════════════════════════════════════╗\n");
    uart_puts("║      VibeCore OS 1.0 — \"Photon\" (aarch64)       ║\n");
    uart_puts("║      Boot sequence initiated...                  ║\n");
    uart_puts("╚══════════════════════════════════════════════════╝\n\n");

    /*
     * Phase 0.5: Kernel Logging (immediately after UART)
     * All subsequent boot messages go through klog.
     */
    klog_init();

    /*
     * Phase 1: Timer (must come BEFORE framebuffer/boot animation)
     */
    klog_info("Initializing system timer...");
    timer_init();
    klog_info("Timer ready.");

    /*
     * Phase 2: Framebuffer + Boot Animation
     */
    klog_info("Initializing framebuffer...");
    if (!framebuffer_init(FB_DEFAULT_WIDTH, FB_DEFAULT_HEIGHT, FB_DEFAULT_DEPTH)) {
        klog_warn("Framebuffer failed! Continuing in headless mode (UART only).");
    } else {
        /* Boot animation (EAGER: all frames pre-computed) */
        boot_animation_run();
    }

    /*
     * Phase 3: Memory Allocator
     */
    klog_info("Initializing memory allocator...");
    allocator_init();
    klog_info("Memory allocator ready.");

    /*
     * Phase 4: MMU (Memory Protection)
     * TEMP: Disabled — 1GB blocks cache MMIO (UART breaks).
     *       Needs 2MB L2 tables for Device/MMIO separation.
     */
    klog_info("MMU currently disabled (MMIO caching issue).");
    /* mmu_init(); */  /* TODO: L2 table for MMIO separation */

    /*
     * Phase 5: Scheduler
     */
    klog_info("Initializing scheduler...");
    scheduler_init();
    klog_info("Scheduler ready.");

    /*
     * Phase 5.5: CRC32 Self-Test (file integrity)
     */
    klog_info("CRC32 self-test...");
    if (!crc32_self_test()) {
        klog_error("CRC32 self-test FAILED!");
    } else {
        klog_info("CRC32 verification active.");
    }

    /*
     * Phase 6: Filesystem
     */
    klog_info("Initializing filesystem...");
    fs_init();
    klog_info("Filesystem ready.");

    /*
     * Phase 6.5: Recovery Subsystems (Journal, Trash, Versions, Hashes)
     */
    klog_info("Initializing recovery system...");
    recovery_journal_init();
    recovery_trash_init();
    recovery_versions_init();
    recovery_hashes_init();
    recovery_boot_scan();  /* Check journal, rollback incomplete TX */
    klog_info("Recovery system ready.");

    /* Initialize snapshots (after recovery subsystems) */
    recovery_snapshots_init();

    /* Auto-create boot snapshot */
    recovery_snapshot_create();

    /*
     * Phase 7: GUI / Desktop
     */
    klog_info("Starting desktop environment...");
    gui_desktop_init();
    klog_info("Desktop started.");

    /*
     * Phase 8: Crash Log (check for previous crashes)
     */
    crash_log_init();
    crash_log_check_previous();

    /*
     * Phase 8.5: Welcome Setup Wizard (first boot only)
     */
    setup_wizard_run();

    /*
     * Phase 9: Shell
     * Runs in an infinite loop and never returns.
     */
    klog_info("Starting shell...");
    uart_puts("\nVibeCore OS is ready!\n\n");

    shell_run();

    /* Should NEVER be reached */
    panic("shell_run() returned unexpectedly");
}
