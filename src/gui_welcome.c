/*
 * ============================================================
 *  VibeCore OS — Graphical Welcome Screen (Optimized)
 *
 *  Renders directly to the framebuffer on first boot.
 *  Dynamic screen dimensions — adapts to any resolution.
 *  Keyboard-driven: press ENTER to continue.
 *
 *  Optimizations:
 *    - Gradient row caching (compute once per row, bulk-fill)
 *    - WFI spin loop (CPU sleep between UART polls)
 *
 *  Design inspired by: VibeOS splash, macOS boot, Win11 OOBE.
 * ============================================================
 */

#include "types.h"
#include "kernel.h"
#include "string.h"
#include "framebuffer.h"
#include "gui_welcome.h"
#include "interrupt.h"

/* Dialog dimensions (centered at runtime) */
#define WELCOME_DLG_W   640
#define WELCOME_DLG_H   420

/* Colors */
#define WL_BG_TOP       RGB(15, 10, 40)
#define WL_BG_BOTTOM    RGB(5, 3, 20)
#define WL_DIALOG_BG    RGB(25, 22, 52)
#define WL_DIALOG_BD    RGB(60, 55, 100)
#define WL_TITLE        RGB(139, 233, 253)
#define WL_TEXT         RGB(220, 220, 240)
#define WL_SUBTEXT      RGB(140, 140, 170)
#define WL_ACCENT       RGB(80, 250, 123)
#define WL_WARN         RGB(255, 184, 108)
#define WL_HINT_BG      RGB(18, 16, 38)
#define WL_HINT_BD      RGB(45, 40, 80)

/* Dynamic dialog position - computed once at runtime */
static i32 dlg_x, dlg_y;

static void draw_centered(i32 y, const char *text, Color color, u8 scale)
{
    i32 len = 0;
    const char *p = text;
    while (*p) len++, p++;
    i32 tw = len * (8 * scale + 1);
    i32 x = dlg_x + (WELCOME_DLG_W - tw) / 2;
    if (x < 20) x = 20;
    framebuffer_drawstring(x, y, text, color, WL_DIALOG_BG, scale);
}

void gui_welcome_show(void)
{
    if (!framebuffer_is_ready()) {
        uart_puts("[WELCOME] No framebuffer - skipping.\n");
        return;
    }

    i32 sw = (i32)framebuffer_get_width();
    i32 sh = (i32)framebuffer_get_height();

    dlg_x = (sw - WELCOME_DLG_W) / 2;
    dlg_y = (sh - WELCOME_DLG_H) / 2;

    /*
     * OPTIMIZED: Background gradient - compute color once per row,
     * then bulk-fill the entire row using fast 128-bit STP writes.
     * Instead of per-pixel math (sw×sh divisions), we compute
     * the color only sh times.
     */
    for (i32 y = 0; y < sh; y++) {
        u8 r = (u8)(15 - (y * 10) / sh);
        u8 g = (u8)(10 - (y * 7)  / sh);
        u8 b = (u8)(40 - (y * 20) / sh);
        u64 p64 = framebuffer_pack64(RGB(r, g, b));
        framebuffer_fillrow(y, p64);
    }

    /* Dialog box */
    framebuffer_fillrect(dlg_x, dlg_y, WELCOME_DLG_W, WELCOME_DLG_H, WL_DIALOG_BG);
    framebuffer_drawrect(dlg_x, dlg_y, WELCOME_DLG_W, WELCOME_DLG_H, WL_DIALOG_BD);
    framebuffer_drawrect(dlg_x+1, dlg_y+1, WELCOME_DLG_W-2, WELCOME_DLG_H-2, RGB(35,30,65));

    /* Logo */
    i32 y = dlg_y + 40;
    draw_centered(y, "VibeCore OS", WL_TITLE, 3);
    y += 52;
    draw_centered(y, "\"Photon\" -- Lightning Fast", WL_ACCENT, 1);
    y += 30;


    char buf[64];
    snprintf_local(buf, sizeof(buf), "Version %d.%d.%d  |  aarch64  |  Cortex-A53",
                   VIBECORE_VERSION_MAJOR, VIBECORE_VERSION_MINOR, VIBECORE_VERSION_PATCH);
    draw_centered(y, buf, WL_SUBTEXT, 1);
    y += 40;

    /* Divider */
    framebuffer_drawline(dlg_x+40, y, dlg_x+WELCOME_DLG_W-40, y, WL_DIALOG_BD);
    y += 20;

    /* Config info */
    draw_centered(y, "System Configuration", WL_TEXT, 1);
    y += 30;
    draw_centered(y, "Username:   vibecore", WL_SUBTEXT, 1);
    y += 22;
    draw_centered(y, "Hostname:   vibecore-pi", WL_SUBTEXT, 1);
    y += 22;
    draw_centered(y, "Filesystem: FAT32  |  RAM: 1 GB  |  Heap: 1 MB", WL_SUBTEXT, 1);
    y += 40;

    /* Feature badges */
    draw_centered(y, "Recovery Journal  |  CRC32 Verify  |  Snapshots  |  System Protect", WL_ACCENT, 1);
    y += 40;

    /* Hint bar */
    i32 hy = dlg_y + WELCOME_DLG_H - 65;
    framebuffer_fillrect(dlg_x+30, hy, WELCOME_DLG_W-60, 45, WL_HINT_BG);
    framebuffer_drawrect(dlg_x+30, hy, WELCOME_DLG_W-60, 45, WL_HINT_BD);
    draw_centered(hy+12, "Press ENTER to continue...", WL_WARN, 1);

    framebuffer_swap();

    /*
     * OPTIMIZED: Use WFI (Wait For Interrupt) between UART polls
     * instead of 100% CPU spin. The Cortex-A53 enters a low-power
     * state and wakes on ANY enabled interrupt (UART RX, timer, etc.).
     *
     * DEPENDENCY: Requires at least one interrupt source to be active
     * (e.g. the system timer). If no interrupts are enabled at all,
     * WFI would never wake and the system would hang. The timer
     * interrupt is configured during kernel boot (timer_init).
     */
    uart_puts("\n[WELCOME] Press ENTER to continue to shell...\n");
    while (1) {
        /* Check if UART has pending data */
        if (uart_has_char()) {
            char c = uart_getc();
            if (c == '\r' || c == '\n') break;
        } else {
            /* Yield CPU until next interrupt (UART RX, timer, etc.) */
            asm volatile("wfi");
        }
    }
    uart_puts("[WELCOME] Continuing to shell...\n");
}
