/*
 * ============================================================
 *  VibeCore OS — Framebuffer Driver (Optimized)
 *
 *  Provides:
 *    - Double-buffering (flicker-free rendering)
 *    - 128-bit ARMv8 STP bulk writes (4 pixels/instruction)
 *    - Fast 64-bit row fills
 *    - Bitmap font rendering (scaled via fast fillrect)
 *    - Screen clear & color fill
 *    - VSync (optional, via Mailbox)
 *
 *  Works directly on the GPU-allocated framebuffer
 *  memory (physical address). Uses uncached device memory —
 *  all writes go straight to the bus.
 * ============================================================
 */

#include "types.h"
#include "peripherals.h"
#include "kernel.h"
#include "mailbox.h"
#include "string.h"
#include "framebuffer.h"

/* ── Framebuffer Info (global) ────────────────────────────── */
typedef struct {
    u32 width;          /* Physical width */
    u32 height;         /* Physical height */
    u32 virt_width;     /* Virtual width (for double-buffering) */
    u32 virt_height;    /* Virtual height */
    u32 pitch;          /* Bytes per line */
    u32 depth;          /* Bits per pixel */
    u32 pixel_order; /* cppcheck-suppress unusedStructMember */
    u8  *buffer;        /* Physical framebuffer address */
    u32 buffer_size;    /* Total buffer size */
    u32 back_offset;    /* Offset to back buffer */
    bool initialized;
} FramebufferInfo;

static FramebufferInfo fb = {0};

#define FB_FRONT    ((u32*)fb.buffer)
#define FB_BACK     ((u32*)(fb.buffer + fb.back_offset))

/* ────────────────────────────────────────────────────────────
 *  framebuffer_init
 *
 *  Initializes the framebuffer via Mailbox Property Tag.
 *  Requests 2× virtual height for double-buffering.
 * ────────────────────────────────────────────────────────── */

bool framebuffer_init(u32 width, u32 height, u32 depth)
{
    /*
     * Pre-allocated framebuffer in BSS (4MB for 1024×768×2×4).
     * Some QEMU versions reject FB_ALLOCATE; pre-allocation is safer.
     */
    static u8 __attribute__((aligned(4096))) fb_buffer[4 * 1024 * 1024];

    /*
     * Mailbox Tag Buffer (16-byte aligned).
     * 24 entries = 24 × 4 = 96 bytes.
     */
    volatile u32 __attribute__((aligned(16))) mbox[26];
    u32 *buf = (u32*)mbox;

    memset((void*)buf, 0, sizeof(mbox));

    buf[0] = sizeof(mbox);
    buf[1] = 0;         /* Request code */

    /* ── Set physical width/height ───────────────────── */
    buf[2] = MBOX_TAG_FB_SET_PHYSICAL_DIM;
    buf[3] = 8;
    buf[4] = 8;
    buf[5] = width;
    buf[6] = height;

    /* ── Set virtual width/height ────────────────────── */
    buf[7] = MBOX_TAG_FB_SET_VIRTUAL_DIM;
    buf[8] = 8;
    buf[9] = 8;
    buf[10] = width;
    buf[11] = height * 2;

    /* ── Set depth ───────────────────────────────────── */
    buf[12] = MBOX_TAG_FB_SET_DEPTH;
    buf[13] = 4;
    buf[14] = 4;
    buf[15] = depth;

    /* ── Set pixel order ─────────────────────────────── */
    buf[16] = MBOX_TAG_FB_SET_PIXEL_ORDER;
    buf[17] = 4;
    buf[18] = 4;
    buf[19] = 1;        /* RGB */

    /* ── Allocate buffer (GPU returns base + size) ───── */
    buf[20] = MBOX_TAG_FB_ALLOCATE;
    buf[21] = 8;            /* Value buffer: 8 bytes response */
    buf[22] = 0;            /* Request: bit 31=0, alignment in value */
    buf[23] = 4096;         /* Alignment hint: 4KB page */
    /* buf[24] will be filled by GPU with buffer size */

    /* End-Tag at buf[25] (will be zero from memset) */

    /* Send to GPU */
    if (!mailbox_call((u32*)buf, MBOX_CH_PROP)) {
        uart_puts("[FB] Mailbox call failed! Trying fallback...\n");
        /*
         * Fallback: skip FB_ALLOCATE, use pre-allocated BSS buffer.
         * Only request dimensions + depth + pixel order.
         */
        memset((void*)buf, 0, sizeof(mbox));

        buf[0] = sizeof(mbox);
        buf[1] = 0;

        /* Only request dimensions + get pitch, no allocate */
        buf[2] = MBOX_TAG_FB_SET_PHYSICAL_DIM;
        buf[3] = 8; buf[4] = 8;
        buf[5] = width; buf[6] = height;

        buf[7] = MBOX_TAG_FB_SET_VIRTUAL_DIM;
        buf[8] = 8; buf[9] = 8;
        buf[10] = width; buf[11] = height * 2;

        buf[12] = MBOX_TAG_FB_SET_DEPTH;
        buf[13] = 4; buf[14] = 4;
        buf[15] = depth;

        buf[16] = MBOX_TAG_FB_SET_PIXEL_ORDER;
        buf[17] = 4; buf[18] = 4;
        buf[19] = 1;

        /* Request pitch from GPU (no FB_ALLOCATE needed) */
        buf[20] = MBOX_TAG_FB_GET_PITCH;
        buf[21] = 4;
        buf[22] = 0;        /* Request */
        /* buf[23] = GPU writes pitch here */

        if (!mailbox_call((u32*)buf, MBOX_CH_PROP)) {
            uart_puts("[FB] Fallback also failed! Headless mode.\n");
            return false;
        }

        /* Read from fallback response */
        fb.width       = buf[5];
        fb.height      = buf[6];
        fb.virt_width  = buf[10];
        fb.virt_height = buf[11];
        fb.depth       = buf[15];
        fb.pitch       = buf[23];  /* GPU returns pitch */
        fb.buffer      = fb_buffer;
        fb.buffer_size = sizeof(fb_buffer);
        fb.back_offset = fb.pitch * fb.height;
        fb.initialized = true;

        uart_printf("[FB] %dx%d, %d-bit (pre-allocated, pitch=%d)\n",
                    fb.width, fb.height, fb.depth, fb.pitch);
        framebuffer_clear(COLOR_BLACK);
        framebuffer_swap();
        return true;
    }

    /* GPU-allocated path: read results from response.
     * FB_ALLOCATE returns: buf[23]=base_addr, buf[24]=size */
    fb.width       = buf[5];
    fb.height      = buf[6];
    fb.virt_width  = buf[10];
    fb.virt_height = buf[11];
    fb.depth       = buf[15];
    fb.pitch       = width * (depth / 8);  /* Fallback calculation */
    fb.buffer      = (u8*)(uintptr_t)(buf[23] & 0x3FFFFFFF);  /* GPU addr → ARM */
    fb.buffer_size = buf[24];              /* Allocated size */
    fb.back_offset = fb.pitch * fb.height;
    fb.initialized = true;

    uart_printf("[FB] %dx%d, %d-bit, pitch=%d, buffer=0x%x (GPU allocated)\n",
                fb.width, fb.height, fb.depth, fb.pitch, (u64)fb.buffer);

    /* Clear screens */
    framebuffer_clear(COLOR_BLACK);
    framebuffer_swap();

    return true;
}

/* ────────────────────────────────────────────────────────────
 *  framebuffer_swap
 *
 *  Swaps front and back buffers (via Mailbox offset tag).
 *  Without VSync: immediate swap.
 * ────────────────────────────────────────────────────────── */

void framebuffer_swap(void)
{
    if (!fb.initialized) return;

    volatile u32 __attribute__((aligned(16))) mbox[8];
    u32 *buf = (u32*)mbox;
    memset((void*)buf, 0, sizeof(mbox));

    /* Swap current offset between front and back */
    static u32 current_y_offset = 0;
    current_y_offset = (current_y_offset == 0) ? fb.height : 0;

    buf[0] = 7 * 4;
    buf[1] = 0;
    buf[2] = MBOX_TAG_FB_SET_VIRTUAL_OFFSET;
    buf[3] = 8;
    buf[4] = 0;
    buf[5] = 0;             /* X-Offset */
    buf[6] = current_y_offset;  /* Y-Offset */
    buf[7] = 0x00000000;

    if (!mailbox_call((u32*)buf, MBOX_CH_PROP)) {
        uart_puts("[FB] Swap failed!\n");
    }
}

/* ────────────────────────────────────────────────────────────
 *  framebuffer_clear  [OPTIMIZED — 128-bit STP assembly]
 *
 *  Clears the back buffer with a solid color.
 *  Uses ARMv8 Store-Pair instruction:
 *    STP xN, xN, [addr], #16  → writes 16 bytes (4 pixels)
 *    per instruction. Cortex-A53 dual-issues this with
 *    the loop branch for ~2 cycles per 4 pixels.
 *
 *  For 1024×768 = 786,432 pixels → ~393,000 instructions
 *  → ~1.6ms at 1.2 GHz (vs ~12ms for per-pixel writes).
 * ────────────────────────────────────────────────────────── */

void framebuffer_clear(Color color)
{
    if (!fb.initialized || fb.buffer == NULL) return;

    /* Pre-pack using the shared helper */
    u64 p64    = framebuffer_pack64(color);
    u64 *ptr   = (u64 *)FB_BACK;

    u32 total_pixels = fb.width * fb.height;
    u32 count  = total_pixels / 2;   /* 2 pixels per u64 */

    /*
     * ARMv8 STP loop: store pair of 64-bit registers.
     * Each iteration writes 16 bytes = 4 pixels.
     * Unrolled 4×: 64 bytes (16 pixels) per loop iteration.
     */
    u32 blocks = count / 8;    /* 8 u64 per block = 16 pixels per block */
    u32 remain = count & 7;

    while (blocks--) {
        asm volatile (
            "stp %[v], %[v], [%[p]], #16\n\t"
            "stp %[v], %[v], [%[p]], #16\n\t"
            "stp %[v], %[v], [%[p]], #16\n\t"
            "stp %[v], %[v], [%[p]], #16\n\t"
            : [p] "+r" (ptr)
            : [v] "r" (p64)
            : "memory"
        );
    }
    /* Tail: 1-7 u64 remaining (2-14 pixels) */
    while (remain--) {
        *ptr++ = p64;
    }

    /* Handle trailing pixel if w*h is odd (rare, but correct) */
    if (total_pixels & 1) {
        u32 *tail = (u32 *)ptr;
        *tail = (u32)p64;  /* Lower 32 bits = one pixel */
    }
}

/* ────────────────────────────────────────────────────────────
 *  framebuffer_pack32  (Internal — single pixel pack)
 *
 *  Returns a 32-bit pixel from a Color. Faster than
 *  framebuffer_pack64() when only one pixel is needed
 *  (e.g. putpixel, drawline).
 * ────────────────────────────────────────────────────────── */

static inline u32 framebuffer_pack32(Color color)
{
    return ((u32)color.a << 24) | ((u32)color.r << 16)
         | ((u32)color.g << 8)  | (u32)color.b;
}

/* ────────────────────────────────────────────────────────────
 *  framebuffer_putpixel
 *
 *  Sets a single pixel (with clipping).
 * ────────────────────────────────────────────────────────── */

void framebuffer_putpixel(i32 x, i32 y, Color color)
{
    if (!fb.initialized || fb.buffer == NULL) return;
    if (x < 0 || y < 0 || (u32)x >= fb.width || (u32)y >= fb.height) return;

    u32 pixel = framebuffer_pack32(color);
    u32 *buf = FB_BACK;
    buf[y * (fb.pitch / 4) + x] = pixel;
}

/* ────────────────────────────────────────────────────────────
 *  framebuffer_fillrect  [OPTIMIZED — 64-bit row blit]
 *
 *  Fills a rectangle with clipping.
 *  Uses 64-bit stores: 2 pixels per write.
 *  Fallback to single-pixel for odd-width rects.
 * ────────────────────────────────────────────────────────── */

void framebuffer_fillrect(i32 x, i32 y, i32 w, i32 h, Color color)
{
    if (!fb.initialized || fb.buffer == NULL) return;

    /* Clipping */
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > (i32)fb.width)  w = fb.width - x;
    if (y + h > (i32)fb.height) h = fb.height - y;
    if (w <= 0 || h <= 0) return;

    /* Pre-pack using the shared helper */
    u64 p64    = framebuffer_pack64(color);
    u32 pixel  = (u32)p64;

    u32 stride = fb.pitch / 4;
    u32 *buf   = FB_BACK + (y * stride) + x;

    /* Only use 64-bit stores if the buffer is 8-byte aligned.
     * On Device-nGnRnE memory (MMU off / GPU framebuffer),
     * unaligned 64-bit accesses cause alignment faults. */
    bool use64 = IS_ALIGNED((uintptr_t)buf, 8);

    for (i32 row = 0; row < h; row++) {
        if (use64) {
            u64 *line64 = (u64 *)buf;
            i32 col = 0;

            /* Fast path: 64-bit writes (2 pixels at a time) */
            for (; col <= w - 2; col += 2) {
                *line64++ = p64;
            }
            /* Odd trailing pixel */
            if (col < w) {
                /* cppcheck-suppress unreadVariable */
                /* cppcheck-suppress unreadVariable */ /* cppcheck-suppress unreadVariable */ ((u32 *)line64)[0] = pixel;
            }
        } else {
            /* Safe path: 32-bit writes (always aligned on Device memory) */
            for (i32 col = 0; col < w; col++) {
                buf[col] = pixel;
            }
        }
        buf += stride;
    }
}

/* ────────────────────────────────────────────────────────────
 *  framebuffer_fillrow  [OPTIMIZED — single-row gradient helper]
 *
 *  Fills one row of the screen with a pre-packed 64-bit
 *  pixel value. Used by gradient backgrounds to avoid
 *  recalculating color per pixel.
 *
 *  Row y is clipped; pixel is a 64-bit double-pixel value
 *  (use framebuffer_pack64(color) to pre-pack).
 * ────────────────────────────────────────────────────────── */

void framebuffer_fillrow(i32 y, u64 p64)
{
    if (!fb.initialized || fb.buffer == NULL) return;
    if (y < 0 || (u32)y >= fb.height) return;

    u64 *ptr  = (u64 *)(FB_BACK + (y * (fb.pitch / 4)));
    u32 count = fb.width / 2;   /* 2 pixels per u64 */

    u32 blocks = count / 8;
    u32 remain = count & 7;

    while (blocks--) {
        asm volatile (
            "stp %[v], %[v], [%[p]], #16\n\t"
            "stp %[v], %[v], [%[p]], #16\n\t"
            "stp %[v], %[v], [%[p]], #16\n\t"
            "stp %[v], %[v], [%[p]], #16\n\t"
            : [p] "+r" (ptr)
            : [v] "r" (p64)
            : "memory"
        );
    }
    while (remain--) {
        *ptr++ = p64;
    }
}

/* Public: pack a Color into 64-bit double-pixel for gradient use */
u64 framebuffer_pack64(Color color)
{
    u32 pixel = ((u32)color.a << 24) | ((u32)color.r << 16)
              | ((u32)color.g << 8)  | (u32)color.b;
    return ((u64)pixel << 32) | pixel;
}

/* ────────────────────────────────────────────────────────────
 *  framebuffer_drawline (Bresenham)
 *
 *  Draws a line between two points.
 * ────────────────────────────────────────────────────────── */

void framebuffer_drawline(i32 x0, i32 y0, i32 x1, i32 y1, Color color)
{
    i32 dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    i32 dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    i32 sx = (x0 < x1) ? 1 : -1;
    i32 sy = (y0 < y1) ? 1 : -1;
    i32 err = dx - dy;

    while (1) {
        framebuffer_putpixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;

        i32 e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}

/* ────────────────────────────────────────────────────────────
 *  framebuffer_drawrect
 *
 *  Draws the outline of a rectangle.
 * ────────────────────────────────────────────────────────── */

void framebuffer_drawrect(i32 x, i32 y, i32 w, i32 h, Color color)
{
    framebuffer_drawline(x, y, x + w - 1, y, color);             /* Top */
    framebuffer_drawline(x, y + h - 1, x + w - 1, y + h - 1, color); /* Bottom */
    framebuffer_drawline(x, y, x, y + h - 1, color);             /* Left */
    framebuffer_drawline(x + w - 1, y, x + w - 1, y + h - 1, color); /* Right */
}

/* ────────────────────────────────────────────────────────────
 *  framebuffer_drawchar  [OPTIMIZED — scaled via fillrect]
 *
 *  Draws a single character (8×16 bitmap font).
 *  Uses framebuffer_fillrect for scaled rendering instead
 *  of nested per-pixel putpixel loops — eliminates
 *  scale×scale bounds checks per glyph bit.
 * ────────────────────────────────────────────────────────── */

void framebuffer_drawchar(i32 x, i32 y, char c, Color fg, Color bg, u8 scale)
{
    UNUSED(bg);

    /* 8×8 Bitmap-Font (hardcoded) */
    static const u8 chr_A[8] = {0x18,0x3C,0x66,0x7E,0x66,0x66,0x66,0x00};
    static const u8 chr_B[8] = {0x7C,0x66,0x7C,0x66,0x66,0x7C,0x66,0x00};
    static const u8 chr_C[8] = {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00};
    static const u8 chr_D[8] = {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00};
    static const u8 chr_E[8] = {0x7E,0x60,0x7C,0x60,0x60,0x60,0x7E,0x00};
    static const u8 chr_F[8] = {0x7E,0x60,0x7C,0x60,0x60,0x60,0x60,0x00};
    static const u8 chr_G[8] = {0x3C,0x66,0x60,0x6E,0x66,0x66,0x3C,0x00};
    static const u8 chr_H[8] = {0x66,0x66,0x7E,0x66,0x66,0x66,0x66,0x00};
    static const u8 chr_I[8] = {0x7E,0x18,0x18,0x18,0x18,0x18,0x7E,0x00};
    static const u8 chr_J[8] = {0x1E,0x0C,0x0C,0x0C,0x6C,0x6C,0x38,0x00};
    static const u8 chr_K[8] = {0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00};
    static const u8 chr_L[8] = {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00};
    static const u8 chr_M[8] = {0xC6,0xEE,0xFE,0xD6,0xC6,0xC6,0xC6,0x00};
    static const u8 chr_N[8] = {0x66,0x76,0x7E,0x6E,0x66,0x66,0x66,0x00};
    static const u8 chr_O[8] = {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00};
    static const u8 chr_P[8] = {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00};
    static const u8 chr_Q[8] = {0x3C,0x66,0x66,0x66,0x6E,0x3C,0x06,0x00};
    static const u8 chr_R[8] = {0x7C,0x66,0x66,0x7C,0x78,0x6C,0x66,0x00};
    static const u8 chr_S[8] = {0x3C,0x66,0x38,0x0C,0x06,0x66,0x3C,0x00};
    static const u8 chr_T[8] = {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00};
    static const u8 chr_U[8] = {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00};
    static const u8 chr_V[8] = {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00};
    static const u8 chr_W[8] = {0xC6,0xC6,0xC6,0xD6,0xFE,0xEE,0xC6,0x00};
    static const u8 chr_X[8] = {0x66,0x3C,0x18,0x18,0x18,0x3C,0x66,0x00};
    static const u8 chr_Y[8] = {0x66,0x66,0x3C,0x18,0x18,0x18,0x18,0x00};
    static const u8 chr_Z[8] = {0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00};
    static const u8 chr_0[8] = {0x3C,0x66,0x6E,0x7E,0x76,0x66,0x3C,0x00};
    static const u8 chr_1[8] = {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00};
    static const u8 chr_2[8] = {0x3C,0x66,0x06,0x0C,0x18,0x30,0x7E,0x00};
    static const u8 chr_3[8] = {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00};
    static const u8 chr_4[8] = {0x0C,0x1C,0x3C,0x6C,0x7E,0x0C,0x0C,0x00};
    static const u8 chr_5[8] = {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00};
    static const u8 chr_6[8] = {0x3C,0x60,0x7C,0x66,0x66,0x66,0x3C,0x00};
    static const u8 chr_7[8] = {0x7E,0x06,0x0C,0x18,0x30,0x30,0x30,0x00};
    static const u8 chr_8[8] = {0x3C,0x66,0x3C,0x66,0x66,0x66,0x3C,0x00};
    static const u8 chr_9[8] = {0x3C,0x66,0x3E,0x06,0x0C,0x18,0x30,0x00};
    static const u8 chr_SPACE[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    static const u8 chr_DOT[8]   = {0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x00};
    static const u8 chr_COLON[8] = {0x00,0x00,0x18,0x00,0x00,0x18,0x00,0x00};
    static const u8 chr_DASH[8]  = {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00};
    static const u8 chr_SLASH[8] = {0x02,0x06,0x0C,0x18,0x30,0x60,0x40,0x00};
    static const u8 chr_BSLASH[8]= {0x40,0x60,0x30,0x18,0x0C,0x06,0x02,0x00};
    static const u8 chr_LPAREN[8]= {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00};
    static const u8 chr_RPAREN[8]= {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00};
    static const u8 chr_EXCL[8]  = {0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x00};
    static const u8 chr_QUEST[8] = {0x3C,0x66,0x06,0x0C,0x18,0x00,0x18,0x00};
    static const u8 chr_AT[8]    = {0x3C,0x66,0x6E,0x6A,0x6E,0x60,0x3C,0x00};
    static const u8 chr_HASH[8]  = {0x24,0x24,0x7E,0x24,0x7E,0x24,0x24,0x00};
    static const u8 chr_DOLLAR[8]= {0x18,0x3E,0x60,0x3C,0x06,0x7C,0x18,0x00};
    static const u8 chr_PERCENT[8]={0x62,0x66,0x0C,0x18,0x30,0x66,0x46,0x00};
    static const u8 chr_CARET[8] = {0x18,0x3C,0x66,0x00,0x00,0x00,0x00,0x00};
    static const u8 chr_AMP[8]   = {0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00};
    static const u8 chr_STAR[8]  = {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00};
    static const u8 chr_PLUS[8]  = {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00};
    static const u8 chr_EQ[8]    = {0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00};
    static const u8 chr_LT[8]    = {0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00};
    static const u8 chr_GT[8]    = {0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x00};
    static const u8 chr_USCORE[8]= {0x00,0x00,0x00,0x00,0x00,0x00,0x7E,0x00};
    static const u8 chr_TILDE[8] = {0x00,0x00,0x00,0x76,0xDC,0x00,0x00,0x00};
    static const u8 chr_PIPE[8]  = {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00};
    static const u8 chr_LBRACK[8]= {0x1C,0x18,0x18,0x18,0x18,0x18,0x1C,0x00};
    static const u8 chr_RBRACK[8]= {0x38,0x18,0x18,0x18,0x18,0x18,0x38,0x00};
    static const u8 chr_QUOTE[8] = {0x6C,0x6C,0x6C,0x00,0x00,0x00,0x00,0x00};
    static const u8 chr_SEMIC[8] = {0x00,0x00,0x18,0x00,0x00,0x18,0x30,0x00};
    static const u8 chr_COMMA[8] = {0x00,0x00,0x00,0x00,0x00,0x18,0x30,0x00};

    const u8 *glyph = NULL;

    if (c >= 'A' && c <= 'Z') {
        const u8 *table[] = {chr_A,chr_B,chr_C,chr_D,chr_E,chr_F,chr_G,chr_H,
                            chr_I,chr_J,chr_K,chr_L,chr_M,chr_N,chr_O,chr_P,
                            chr_Q,chr_R,chr_S,chr_T,chr_U,chr_V,chr_W,chr_X,
                            chr_Y,chr_Z};
        glyph = table[c - 'A'];
    } else if (c >= 'a' && c <= 'z') {
        const u8 *table[] = {chr_A,chr_B,chr_C,chr_D,chr_E,chr_F,chr_G,chr_H,
                            chr_I,chr_J,chr_K,chr_L,chr_M,chr_N,chr_O,chr_P,
                            chr_Q,chr_R,chr_S,chr_T,chr_U,chr_V,chr_W,chr_X,
                            chr_Y,chr_Z};
        glyph = table[c - 'a'];
    } else if (c >= '0' && c <= '9') {
        const u8 *table[] = {chr_0,chr_1,chr_2,chr_3,chr_4,chr_5,chr_6,chr_7,chr_8,chr_9};
        glyph = table[c - '0'];
    } else {
        switch (c) {
            case ' ': glyph = chr_SPACE; break;
            case '.': glyph = chr_DOT; break;
            case ':': glyph = chr_COLON; break;
            case '-': glyph = chr_DASH; break;
            case '/': glyph = chr_SLASH; break;
            case '\\': glyph = chr_BSLASH; break;
            case '(': glyph = chr_LPAREN; break;
            case ')': glyph = chr_RPAREN; break;
            case '!': glyph = chr_EXCL; break;
            case '?': glyph = chr_QUEST; break;
            case '@': glyph = chr_AT; break;
            case '#': glyph = chr_HASH; break;
            case '$': glyph = chr_DOLLAR; break;
            case '%': glyph = chr_PERCENT; break;
            case '^': glyph = chr_CARET; break;
            case '&': glyph = chr_AMP; break;
            case '*': glyph = chr_STAR; break;
            case '+': glyph = chr_PLUS; break;
            case '=': glyph = chr_EQ; break;
            case '<': glyph = chr_LT; break;
            case '>': glyph = chr_GT; break;
            case '_': glyph = chr_USCORE; break;
            case '~': glyph = chr_TILDE; break;
            case '|': glyph = chr_PIPE; break;
            case '[': glyph = chr_LBRACK; break;
            case ']': glyph = chr_RBRACK; break;
            case '"': glyph = chr_QUOTE; break;
            case ';': glyph = chr_SEMIC; break;
            case ',': glyph = chr_COMMA; break;
            default:  glyph = chr_SPACE; break;
        }
    }

    if (glyph == NULL) return;

    /*
     * OPTIMIZED: Instead of nested scale*scale per-pixel calls,
     * use framebuffer_fillrect for scaled glyphs.
     */
    if (scale == 1) {
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                if (glyph[row] & (1 << (7 - col))) {
                    framebuffer_putpixel(x + col, y + row, fg);
                }
            }
        }
    } else {
        /* Use fillrect for scaled rendering — 1 call per bit */
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                if (glyph[row] & (1 << (7 - col))) {
                    framebuffer_fillrect(
                        x + col * scale, y + row * scale,
                        scale, scale, fg
                    );
                }
            }
        }
    }
}

/* ────────────────────────────────────────────────────────────
 *  framebuffer_drawstring
 *
 *  Draws a string at position (x, y).
 * ────────────────────────────────────────────────────────── */

void framebuffer_drawstring(i32 x, i32 y, const char *str, Color fg, Color bg, u8 scale)
{
    if (str == NULL) return;
    i32 cx = x;
    while (*str) {
        if (*str == '\n') {
            cx = x;
            y += 8 * scale + 2;
        } else {
            framebuffer_drawchar(cx, y, *str, fg, bg, scale);
            cx += 8 * scale + 1;
        }
        str++;
    }
}

/* ────────────────────────────────────────────────────────────
 *  Getters for external use
 * ────────────────────────────────────────────────────────── */

u32 framebuffer_get_width(void)  { return fb.width; }
u32 framebuffer_get_height(void) { return fb.height; }
bool framebuffer_is_ready(void)   { return fb.initialized; }
