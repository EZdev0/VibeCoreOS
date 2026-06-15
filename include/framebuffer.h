/*
 * ============================================================
 *  VibeCore OS — Framebuffer Header
 * ============================================================
 */

#ifndef _FRAMEBUFFER_H
#define _FRAMEBUFFER_H

#include "types.h"

/* ── Initialization ──────────────────────────────────────── */
bool framebuffer_init(u32 width, u32 height, u32 depth);
void framebuffer_swap(void);
bool framebuffer_is_ready(void);

/* ── Drawing Primitives ──────────────────────────────────── */
void framebuffer_clear(Color color);
void framebuffer_putpixel(i32 x, i32 y, Color color);
void framebuffer_fillrect(i32 x, i32 y, i32 w, i32 h, Color color);
void framebuffer_drawrect(i32 x, i32 y, i32 w, i32 h, Color color);
void framebuffer_drawline(i32 x0, i32 y0, i32 x1, i32 y1, Color color);

/* ── Optimized Gradient Helpers ──────────────────────────── */
u64  framebuffer_pack64(Color color);
void framebuffer_fillrow(i32 y, u64 p64);

/* ── Text Rendering ──────────────────────────────────────── */
void framebuffer_drawchar(i32 x, i32 y, char c, Color fg, Color bg, u8 scale);
void framebuffer_drawstring(i32 x, i32 y, const char *str, Color fg, Color bg, u8 scale);

/* ── Getters ─────────────────────────────────────────────── */
u32  framebuffer_get_width(void);
u32  framebuffer_get_height(void);

#endif /* _FRAMEBUFFER_H */
