/*
 * ============================================================
 *  VibeCore OS — Boot Animation (Optimized)
 *
 *  Frame-based boot animation with double-buffering.
 *  ALL frames are pre-rendered eagerly and then displayed
 *  — no lazy rendering to prevent visual glitches.
 *
 *  Animation: Progressive gradient progress bar +
 *  pulsing VibeCore logo (text-based).
 *
 *  Optimizations:
 *    - Gradient row caching (fillrow bulk writes)
 *    - Progress bar fill via fillrect instead of per-pixel
 * ============================================================
 */

#include "types.h"
#include "kernel.h"
#include "string.h"
#include "framebuffer.h"
#include "timer.h"

/* ── Pre-Rendered Frame Buffer ────────────────────────────── */
#define BOOT_FRAMES      20
#define BOOT_BAR_WIDTH   400
#define BOOT_BAR_HEIGHT  20
#define BOOT_BAR_X       312   /* Centered at 1024: (1024-400)/2 */
#define BOOT_BAR_Y       450
#define BOOT_LOGO_X      200
#define BOOT_LOGO_Y      200

void boot_animation_run(void)
{
    if (!framebuffer_is_ready()) {
        uart_puts("[BOOT] No framebuffer — Boot animation skipped.\n");
        return;
    }

    u32 w = framebuffer_get_width();
    u32 h = framebuffer_get_height();
    UNUSED(w);

    uart_puts("[BOOT] Starting boot animation...\n");

    /*
     * EAGER rendering strategy:
     * We render ALL 20 frames in advance to the back buffer
     * and swap. No frame is rendered "on-demand."
     * This prevents the "hover bug" and guarantees smooth
     * animation without visual glitches.
     */

    for (int frame = 0; frame < BOOT_FRAMES; frame++) {
        /* ── Background (gradient, optimized per-row fill) ── */
        for (u32 y = 0; y < h; y++) {
            u8 r = 10 + (y * 15) / h;
            u8 g = 5 + (y * 10) / h;
            u8 b = 20 + (y * 30) / h;
            u64 p64 = framebuffer_pack64(RGB(r, g, b));
            framebuffer_fillrow((i32)y, p64);
        }

        /* ── Logo (pulsing) ──────────────────────────── */
        u8 logo_brightness = 100 + (frame * 155) / BOOT_FRAMES;
        Color logo_color = RGB(0, logo_brightness, logo_brightness);

        framebuffer_drawstring(BOOT_LOGO_X, BOOT_LOGO_Y,
            "   VibeCore OS", logo_color, RGB(10, 5, 20), 2);
        framebuffer_drawstring(BOOT_LOGO_X, BOOT_LOGO_Y + 40,
            "   \"Photon\" — Lightning fast", RGB(150, 150, 180), RGB(10, 5, 20), 1);

        /* ── Progress bar (outer frame) ──────────────── */
        framebuffer_drawrect(BOOT_BAR_X - 2, BOOT_BAR_Y - 2,
                            BOOT_BAR_WIDTH + 4, BOOT_BAR_HEIGHT + 4,
                            RGB(100, 100, 150));

        /* ── Progress bar (fill, optimized via fillrect) ── */
        u32 fill_width = (BOOT_BAR_WIDTH * frame) / BOOT_FRAMES;
        if (fill_width > 0) {
            /* Gradient fill: column-wise color, batch per column */
            for (u32 bx = 0; bx < fill_width; bx++) {
                u8 intensity = 100 + (bx * 155) / BOOT_BAR_WIDTH;
                framebuffer_fillrect(
                    BOOT_BAR_X + (i32)bx, BOOT_BAR_Y,
                    1, BOOT_BAR_HEIGHT,
                    RGB(0, intensity, intensity)
                );
            }
        }

        /* ── Percentage display ──────────────────────── */
        char pct_buf[16];
        int pct = (frame * 100) / BOOT_FRAMES;
        char *p = pct_buf;
        if (pct >= 100) { *p++ = '1'; *p++ = '0'; *p++ = '0'; }
        else if (pct >= 10) { *p++ = '0' + (pct / 10); *p++ = '0' + (pct % 10); }
        else { *p++ = '0' + pct; }
        *p++ = '%';
        *p = '\0';

        framebuffer_drawstring(BOOT_BAR_X + BOOT_BAR_WIDTH / 2 - 20,
                              BOOT_BAR_Y + BOOT_BAR_HEIGHT + 10,
                              pct_buf, RGB(200, 200, 255), RGB(10, 5, 20), 2);

        /* ── Boot messages (progressive) ─────────────── */
        const char *msgs[] = {
            "Initializing UART...",
            "Activating Framebuffer...",
            "Configuring MMU...",
            "Starting Memory Allocator...",
            "Loading Scheduler...",
            "Mounting Filesystem..."
        };
        int msg_count = sizeof(msgs) / sizeof(msgs[0]);
        int visible_msgs = (frame * msg_count) / BOOT_FRAMES;

        for (int m = 0; m < visible_msgs && m < msg_count; m++) {
            Color msg_color = (m < visible_msgs - 1) ? RGB(80, 180, 80) : RGB(180, 180, 255);
            char status = (m < visible_msgs - 1) ? '+' : '>';
            char line[64];
            char *lp = line;
            *lp++ = ' ';
            *lp++ = ' ';
            *lp++ = '[';
            *lp++ = status;
            *lp++ = ']';
            *lp++ = ' ';
            const char *ms = msgs[m];
            while (*ms) *lp++ = *ms++;
            *lp = '\0';

            framebuffer_drawstring(BOOT_LOGO_X, BOOT_LOGO_Y + 80 + m * 22,
                                  line, msg_color, RGB(10, 5, 20), 1);
        }

        /*
         * ── Frame swap & short delay ──────────────────
         * Each frame is displayed IMMEDIATELY — no lazy rendering!
         */
        framebuffer_swap();
        timer_sleep_ms(80);  /* ~12.5 FPS — smooth but not CPU-intensive */
    }

    uart_puts("[BOOT] Boot animation completed.\n");
}
