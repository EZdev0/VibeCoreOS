/*
 * ============================================================
 *  VibeCore OS — GUI / Desktop Environment (Optimized)
 *
 *  Provides:
 *    - Desktop background (vertical gradient)
 *    - Taskbar (bottom, with clock and system info)
 *    - Window Manager (minimal)
 *    - Desktop Icons
 *    - Crash-Screen Integration
 *
 *  Renders directly on the framebuffer.
 *
 *  Optimizations:
 *    - Gradient row caching (compute once per row, bulk-fill)
 *    - All text in English
 * ============================================================
 */

#include "types.h"
#include "kernel.h"
#include "peripherals.h"
#include "string.h"
#include "framebuffer.h"
#include "gui.h"

/* ── GUI Colors ──────────────────────────────────────────── */
#define GUI_BG_COLOR        RGB(30, 30, 46)     /* Dark blue-gray */
#define GUI_TASKBAR_COLOR   RGB(17, 17, 27)     /* Darker */
#define GUI_TASKBAR_BORDER  RGB(69, 71, 90)     /* Bright top line */
#define GUI_WINDOW_TITLE    RGB(49, 50, 68)     /* Window titlebar */
#define GUI_WINDOW_BG       RGB(40, 42, 54)     /* Window background */
#define GUI_TEXT_COLOR      RGB(248, 248, 242)  /* Light text */
#define GUI_TEXT_DIM        RGB(98, 114, 164)   /* Dimmed text */
#define GUI_ACCENT          RGB(139, 233, 253)  /* Cyan accent */
#define GUI_ACCENT2         RGB(80, 250, 123)   /* Green accent */
#define GUI_WARNING         RGB(255, 184, 108)  /* Orange warning */
#define GUI_CLOCK_COLOR     RGB(189, 147, 249)  /* Purple for clock */

/* ── Taskbar Constants ───────────────────────────────────── */
#define TASKBAR_HEIGHT      40
#define TASKBAR_ICON_SIZE   28
#define TASKBAR_CLOCK_X     900

/* ────────────────────────────────────────────────────────────
 *  gui_desktop_init
 *
 *  Initializes the desktop environment and draws
 *  the initial screen.
 * ────────────────────────────────────────────────────────── */

void gui_desktop_init(void)
{
    if (!framebuffer_is_ready()) {
        uart_puts("[GUI] No framebuffer - desktop not available.\n");
        return;
    }

    u32 w = framebuffer_get_width();
    u32 h = framebuffer_get_height();

    uart_puts("[GUI] Desktop environment starting...\n");

    /*
     * OPTIMIZED: Desktop background gradient.
     * Compute color once per row, then bulk-fill.
     * Top: dark blue → Bottom: purple-black.
     */
    for (u32 y = 0; y < h; y++) {
        u8 r = 30 + (y * 30) / h;       /* 30 → 60 */
        u8 g = 30 + (y * 20) / h;       /* 30 → 50 */
        u8 b = 46 + (y * 50) / h;       /* 46 → 96 */
        u64 p64 = framebuffer_pack64(RGB(r, g, b));
        framebuffer_fillrow((i32)y, p64);
    }

    /* Taskbar */
    gui_draw_taskbar();

    /* Desktop Icons */
    gui_draw_icon(20, 100, "Terminal",   '[', GUI_ACCENT, "Terminal");
    gui_draw_icon(20, 170, "Files",      'F', GUI_ACCENT, "Files");
    gui_draw_icon(20, 240, "Editor",     'E', GUI_ACCENT2, "Editor");
    gui_draw_icon(20, 310, "Settings",   '*', GUI_WARNING, "Settings");
    gui_draw_icon(20, 380, "Info",       'i', GUI_CLOCK_COLOR, "Info");

    /* Welcome Window */
    gui_draw_window(200, 150, 500, 300, "VibeCore OS 1.0 \"Photon\"");

    /* Window content */
    framebuffer_drawstring(220, 240,
        "Welcome to VibeCore OS!\n\n"
        "  * ARM64 Bare-Metal OS\n"
        "  * BORE-inspired Scheduler\n"
        "  * Hardware-Accelerated Graphics\n"
        "  * FAT32 Filesystem\n"
        "  * Crash Screen with Diagnostics\n"
        "  * Memory Protection (MMU)\n\n"
        "  Press a key in the terminal...",
        GUI_TEXT_COLOR, GUI_WINDOW_BG, 1);

    /* System Info Widget (top right) */
    framebuffer_fillrect(w - 220, 10, 210, 80, RGB(17, 17, 27));
    framebuffer_drawrect(w - 220, 10, 210, 80, GUI_TASKBAR_BORDER);
    framebuffer_drawstring(w - 210, 20, "System", GUI_ACCENT, RGB(17, 17, 27), 1);
    framebuffer_drawstring(w - 210, 40, "Kernel: ARM64 aarch64", GUI_TEXT_DIM, RGB(17, 17, 27), 1);
    framebuffer_drawstring(w - 210, 55, "RAM: 1 GB", GUI_TEXT_DIM, RGB(17, 17, 27), 1);
    framebuffer_drawstring(w - 210, 70, "CPU: Cortex-A53", GUI_TEXT_DIM, RGB(17, 17, 27), 1);

    framebuffer_swap();
    uart_puts("[GUI] Desktop initialized.\n");
}

/* ────────────────────────────────────────────────────────────
 *  gui_draw_taskbar
 *
 *  Draws the taskbar at the bottom of the screen.
 * ────────────────────────────────────────────────────────── */

void gui_draw_taskbar(void)
{
    u32 w = framebuffer_get_width();
    u32 h = framebuffer_get_height();
    u32 y = h - TASKBAR_HEIGHT;

    /* Taskbar background */
    framebuffer_fillrect(0, y, w, TASKBAR_HEIGHT, GUI_TASKBAR_COLOR);

    /* Top border line */
    framebuffer_drawline(0, y, w, y, GUI_TASKBAR_BORDER);

    /* Start button */
    framebuffer_fillrect(5, y + 4, 80, 32, RGB(98, 114, 164));
    framebuffer_drawstring(18, y + 12, "VibeCore", GUI_TEXT_COLOR, RGB(98, 114, 164), 1);

    /* Clock */
    framebuffer_drawstring(TASKBAR_CLOCK_X, y + 12, "14:32", GUI_CLOCK_COLOR, GUI_TASKBAR_COLOR, 1);

    /* System tray */
    framebuffer_drawstring(w - 80, y + 12, "OK", GUI_ACCENT2, GUI_TASKBAR_COLOR, 1);
}

/* ────────────────────────────────────────────────────────────
 *  gui_draw_icon
 *
 *  Draws a desktop icon at position (x, y).
 * ────────────────────────────────────────────────────────── */

void gui_draw_icon(i32 x, i32 y, const char *label, char symbol, Color color, const char *tooltip)
{
    UNUSED(tooltip);

    /* Icon background (square) */
    framebuffer_fillrect(x, y, 48, 48, RGB(40, 42, 54));
    framebuffer_drawrect(x, y, 48, 48, color);

    /* Symbol centered */

    const char sym_str[2] = {symbol, '\0'};
    framebuffer_drawstring(x + 16, y + 14, sym_str, color, RGB(40, 42, 54), 2);

    /* Label */
    framebuffer_drawstring(x - 5, y + 54, label, GUI_TEXT_COLOR,
                          (Color){30, 30, 46, 255}, 1);
}

/* ────────────────────────────────────────────────────────────
 *  gui_draw_window
 *
 *  Draws a window with a titlebar and interior area.
 * ────────────────────────────────────────────────────────── */

void gui_draw_window(i32 x, i32 y, i32 w, i32 h, const char *title)
{
    /* Window background */
    framebuffer_fillrect(x, y, w, h, GUI_WINDOW_BG);

    /* Titlebar */
    framebuffer_fillrect(x, y, w, 30, GUI_WINDOW_TITLE);

    /* Window frame */
    framebuffer_drawrect(x, y, w, h, GUI_TASKBAR_BORDER);
    /* Slightly thicker border */
    framebuffer_drawrect(x + 1, y + 1, w - 2, h - 2, RGB(30, 30, 40));

    /* Title text */
    if (title) {
        framebuffer_drawstring(x + 10, y + 7, title, GUI_TEXT_COLOR, GUI_WINDOW_TITLE, 1);
    }

    /* Close button */
    framebuffer_fillrect(x + w - 28, y + 4, 22, 22, RGB(255, 85, 85));
    framebuffer_drawstring(x + w - 22, y + 7, "X", GUI_TEXT_COLOR, RGB(255, 85, 85), 1);

    /* Minimize button */
    framebuffer_fillrect(x + w - 54, y + 4, 22, 22, RGB(255, 184, 108));
    framebuffer_drawstring(x + w - 48, y + 7, "_", GUI_TEXT_COLOR, RGB(255, 184, 108), 1);
}

/* ────────────────────────────────────────────────────────────
 *  gui_update_clock
 *
 *  Updates the clock in the taskbar.
 * ────────────────────────────────────────────────────────── */

void gui_update_clock(u32 hours, u32 minutes)
{
    if (!framebuffer_is_ready()) return;


    char buf[16];
    char *p = buf;

    if (hours < 10) { *p++ = '0'; }
    if (hours >= 10) { *p++ = '0' + (hours / 10) % 10; }
    *p++ = '0' + hours % 10;
    *p++ = ':';
    *p++ = '0' + (minutes / 10) % 10;
    *p++ = '0' + minutes % 10;
    *p = '\0';

    u32 h = framebuffer_get_height();

    /* Overwrite old clock */
    framebuffer_fillrect(TASKBAR_CLOCK_X, h - TASKBAR_HEIGHT + 10, 50, 20, GUI_TASKBAR_COLOR);
    framebuffer_drawstring(TASKBAR_CLOCK_X, h - TASKBAR_HEIGHT + 12, buf,
                          GUI_CLOCK_COLOR, GUI_TASKBAR_COLOR, 1);
    framebuffer_swap();
}
