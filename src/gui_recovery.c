/*
 * ============================================================
 *  VibeCore OS — Graphical Recovery Screen (Optimized)
 *
 *  Renders directly to the framebuffer on crash/corruption
 *  detection or when 'recovery' command is issued.
 *
 *  Keyboard-driven (no mouse needed):
 *    1 = Run FSCK
 *    2 = Restore Snapshot
 *    3 = Reboot
 *
 *  Optimizations:
 *    - Gradient row caching (compute once per row, bulk-fill)
 *    - WFI spin loop (CPU sleep between UART polls)
 *
 *  Design inspired by: Windows RE, macOS Recovery, VibeOS.
 * ============================================================
 */

#include "types.h"
#include "kernel.h"
#include "string.h"
#include "framebuffer.h"
#include "gui_recovery.h"
#include "recovery.h"
#include "interrupt.h"
#include "timer.h"

/* Dialog dimensions (centered at runtime) */
#define REC_DLG_W   640
#define REC_DLG_H   460

/* Colors */
#define REC_BG_TOP      RGB(40, 5, 5)
#define REC_DIALOG_BG   RGB(30, 15, 18)
#define REC_DIALOG_BD   RGB(120, 40, 40)
#define REC_TITLE       RGB(255, 85, 85)
#define REC_TEXT        RGB(240, 220, 220)
#define REC_SUBTEXT     RGB(170, 140, 140)
#define REC_ACCENT      RGB(255, 184, 108)
#define REC_OPTION_BG   RGB(22, 10, 12)
#define REC_OPTION_BD   RGB(80, 35, 35)
#define REC_OK          RGB(80, 250, 123)

static i32 rec_x, rec_y;

static void rec_centered(i32 y, const char *text, Color color, u8 scale)
{
    i32 len = 0;
    const char *p = text;
    while (*p) len++, p++;
    i32 tw = len * (8 * scale + 1);
    i32 x = rec_x + (REC_DLG_W - tw) / 2;
    if (x < 20) x = 20;
    framebuffer_drawstring(x, y, text, color, REC_DIALOG_BG, scale);
}

static void rec_option(i32 y, int num, const char *label)
{
    Color bg = REC_OPTION_BG;
    Color fg = REC_TEXT;
    Color bd = REC_OPTION_BD;

    i32 ox = rec_x + 50;
    i32 ow = REC_DLG_W - 100;
    framebuffer_fillrect(ox, y, ow, 38, bg);
    framebuffer_drawrect(ox, y, ow, 38, bd);

    char buf[64];
    snprintf_local(buf, sizeof(buf), "  [%d]  %s", num, label);
    framebuffer_drawstring(ox + 14, y + 10, buf, fg, bg, 1);
}

void gui_recovery_show(const char *reason)
{
    if (!framebuffer_is_ready()) {
        uart_puts("[RECOVERY] No framebuffer - using text mode.\n");
        return;
    }

    i32 sw = (i32)framebuffer_get_width();
    i32 sh = (i32)framebuffer_get_height();

    rec_x = (sw - REC_DLG_W) / 2;
    rec_y = (sh - REC_DLG_H) / 2;

    /*
     * OPTIMIZED: Background gradient - compute color once per row,
     * then bulk-fill the entire row using fast 128-bit STP writes.
     */
    for (i32 y = 0; y < sh; y++) {
        u8 r = (u8)(40 - (y * 25) / sh);
        u8 g = (u8)(5  - (y * 3)  / sh);
        u8 b = (u8)(5  - (y * 3)  / sh);
        u64 p64 = framebuffer_pack64(RGB(r, g, b));
        framebuffer_fillrow(y, p64);
    }

    /* Dialog box */
    framebuffer_fillrect(rec_x, rec_y, REC_DLG_W, REC_DLG_H, REC_DIALOG_BG);
    framebuffer_drawrect(rec_x, rec_y, REC_DLG_W, REC_DLG_H, REC_DIALOG_BD);
    framebuffer_drawrect(rec_x+1, rec_y+1, REC_DLG_W-2, REC_DLG_H-2, RGB(40,20,25));

    /* Header */
    i32 y = rec_y + 30;
    rec_centered(y, "VibeCore OS -- Recovery Environment", REC_TITLE, 2);
    y += 45;

    char sub[64];
    snprintf_local(sub, sizeof(sub), "Reason: %.40s", reason ? reason : "Manual recovery");
    rec_centered(y, sub, REC_SUBTEXT, 1);
    y += 30;

    framebuffer_drawline(rec_x+40, y, rec_x+REC_DLG_W-40, y, REC_DIALOG_BD);
    y += 20;

    /* Options */
    rec_centered(y, "Select an option:", REC_TEXT, 1);
    y += 30;
    rec_option(y, 1, "Run Filesystem Check (FSCK)");
    y += 48;
    rec_option(y, 2, "Restore Latest Snapshot");
    y += 48;
    rec_option(y, 3, "Reboot System");
    y += 48;

    /* Hint */
    y = rec_y + REC_DLG_H - 45;
    rec_centered(y, "Press 1-3 to select an option...", REC_ACCENT, 1);

    framebuffer_swap();
}

void gui_recovery_trigger(const char *reason)
{
    gui_recovery_show(reason);

    /*
     * OPTIMIZED: WFI-based polling instead of 100% CPU spin.
     * The Cortex-A53 sleeps between UART checks, waking only
     * on enabled interrupts (timer tick, UART RX, etc.).
     *
     * DEPENDENCY: Requires timer interrupts to be active
     * (configured during timer_init in kernel boot). If no
     * interrupts are enabled, the system would hang.
     */
    while (1) {
        /* Non-blocking poll with CPU sleep */
        if (!uart_has_char()) {
            asm volatile("wfi");
            continue;
        }

        char c = uart_getc();

        if (c == '1') {
            uart_puts("\n[RECOVERY] Running FSCK...\n");
            recovery_boot_scan();
            recovery_shell_fsck();

            if (framebuffer_is_ready()) {
                i32 y2 = rec_y + REC_DLG_H - 45;
                framebuffer_fillrect(rec_x+30, y2-10, REC_DLG_W-60, 30, REC_DIALOG_BG);
                rec_centered(y2-10, "FSCK complete -- Press 3 to reboot", REC_OK, 1);
                framebuffer_swap();
            }
            return;
        }

        if (c == '2') {
            uart_puts("\n[RECOVERY] Restoring snapshot...\n");
            recovery_snapshot_restore();

            if (framebuffer_is_ready()) {
                i32 y2 = rec_y + REC_DLG_H - 45;
                framebuffer_fillrect(rec_x+30, y2-10, REC_DLG_W-60, 30, REC_DIALOG_BG);
                rec_centered(y2-10, "Snapshot restored -- Press 3 to reboot", REC_OK, 1);
                framebuffer_swap();
            }
            return;
        }

        if (c == '3') {
            uart_puts("\n[RECOVERY] Rebooting...\n");
            if (framebuffer_is_ready()) {
                framebuffer_clear(RGB(0, 0, 0));
                framebuffer_drawstring(rec_x+220, 370, "Rebooting...", REC_ACCENT, RGB(0,0,0), 2);
                framebuffer_swap();
            }
            timer_sleep_ms(1000);
            while (1) { asm volatile("wfi"); }
        }
    }
}
