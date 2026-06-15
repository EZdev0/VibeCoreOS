/*
 * ============================================================
 *  VibeCore OS — Type Definitions & Safety Macros
 *  Strongly typed, null-checked, modern ARM64 types
 * ============================================================
 */

#ifndef _TYPES_H
#define _TYPES_H

/* ── Integer Types (explicit width) ───────────────────────── */
typedef signed char        i8;
typedef signed short       i16;
typedef signed int         i32;
typedef signed long long   i64;

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

/* ── Architecture-specific ────────────────────────────────── */
typedef u64                size_t;
typedef i64                ssize_t;
typedef u64                uintptr_t;
typedef i64                intptr_t;
typedef u32                pid_t;

/* ── Boolean ──────────────────────────────────────────────── */
typedef enum { false = 0, true = 1 } bool;

/* ── Pointer Attributes ──────────────────────────────────── */
#define NULL        ((void*)0)
#define UNUSED(x)   ((void)(x))

/* ── Null Checks (Safety Guarantee) ──────────────────────── */
#define CHECK_NULL(ptr)             \
    do {                            \
        if ((ptr) == NULL) {        \
            panic("NULL pointer: " #ptr); \
        }                           \
    } while (0)

#define CHECK_NULL_MSG(ptr, msg)    \
    do {                            \
        if ((ptr) == NULL) {        \
            panic(msg);             \
        }                           \
    } while (0)

/* ── Range Checks ────────────────────────────────────────── */
#define CHECK_RANGE(val, min, max, msg)     \
    do {                                    \
        if ((val) < (min) || (val) > (max)) { \
            panic(msg);                     \
        }                                   \
    } while (0)

/* ── Alignment ────────────────────────────────────────────── */
#define ALIGN_UP(x, a)      (((x) + ((a) - 1)) & ~((a) - 1))
#define ALIGN_DOWN(x, a)    ((x) & ~((a) - 1))
#define IS_ALIGNED(x, a)    (((x) & ((a) - 1)) == 0)

/* ── Min/Max ──────────────────────────────────────────────── */
#define MIN(a, b)   ((a) < (b) ? (a) : (b))
#define MAX(a, b)   ((a) > (b) ? (a) : (b))

/* ── Bit Operations ──────────────────────────────────────── */
#define BIT(n)              (1UL << (n))
#define BITMASK(hi, lo)     (((1UL << ((hi) - (lo) + 1)) - 1) << (lo))
#define BIT_GET(val, bit)   (((val) >> (bit)) & 1)
#define BIT_SET(val, bit)   ((val) |= BIT(bit))
#define BIT_CLR(val, bit)   ((val) &= ~BIT(bit))

/* ── Colors (ARGB) ────────────────────────────────────────── */
typedef struct {
    u8 b, g, r, a;
} Color;

#define RGB(r, g, b)        ((Color){b, g, r, 255})
#define RGBA(r, g, b, a)    ((Color){b, g, r, a})

/* VibeCore color palette */
#define COLOR_BLACK         RGB(0, 0, 0)
#define COLOR_WHITE         RGB(255, 255, 255)
#define COLOR_RED           RGB(255, 0, 0)
#define COLOR_GREEN         RGB(0, 255, 0)
#define COLOR_BLUE          RGB(0, 0, 255)
#define COLOR_CYAN          RGB(0, 255, 255)
#define COLOR_MAGENTA       RGB(255, 0, 255)
#define COLOR_YELLOW        RGB(255, 255, 0)
#define COLOR_ORANGE        RGB(255, 165, 0)
#define COLOR_GRAY          RGB(128, 128, 128)
#define COLOR_DARK_GRAY     RGB(64, 64, 64)
#define COLOR_LIGHT_GRAY    RGB(192, 192, 192)
#define COLOR_CRASH_BG      RGB(0, 0, 170)      /* Bluescreen background */
#define COLOR_CRASH_FG      RGB(255, 255, 255)  /* Bluescreen text */

/* ── Point & Rectangle ────────────────────────────────────── */
typedef struct {
    i32 x, y;
} Point;

typedef struct {
    i32 x, y, w, h;
} Rect;

/* ── Kernel Panic Prototype ───────────────────────────────── */
void panic(const char *msg);

#endif /* _TYPES_H */
