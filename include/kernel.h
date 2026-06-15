/*
 * ============================================================
 *  VibeCore OS — Kernel Main Header
 *  Global definitions, version, memory layout
 *  Optimized for 8GB-constrained Raspberry Pi
 * ============================================================
 */

#ifndef _KERNEL_H
#define _KERNEL_H

#include "types.h"
#include "peripherals.h"

/* ── OS Version ───────────────────────────────────────────── */
#define VIBECORE_VERSION_MAJOR  1
#define VIBECORE_VERSION_MINOR  0
#define VIBECORE_VERSION_PATCH  0
#define VIBECORE_CODENAME       "Photon"
#define VIBECORE_VERSION_STRING "VibeCore OS 1.0.0 \"Photon\" (aarch64) 8GB-Edition"

/* ── Memory Layout (optimized for 8GB Pi) ─────────────────── */
extern u8 __bss_start;
extern u8 __bss_end;
extern u8 __stack_top;
extern u8 __stack_bottom;
extern u8 __heap_start;
extern u8 __heap_end;
extern u8 __kernel_end;
extern u8 __guard_page;

#define KERNEL_LOAD_ADDR    0x80000UL
#define KERNEL_STACK_SIZE   0x20000UL   /* 128 KB (optimized) */
#define KERNEL_HEAP_SIZE    0x100000UL  /* 1 MB (optimized) */
#define EMERGENCY_RESERVE   0x10000UL   /* 64 KB emergency reserve */

/* ── Maximum Values (optimized for 8GB) ──────────────────── */
#define MAX_TASKS           32          /* Reduced: saves RAM */
#define MAX_TIMERS          16
#define MAX_FILE_HANDLES    64
#define MAX_PATH_LEN        256
#define MAX_FILENAME_LEN    128
#define MAX_CMD_LEN         256

/* ── Ticks & Timing ──────────────────────────────────────── */
#define TIMER_FREQ_HZ       1000        /* 1000 Hz (1ms tick) */
#define TICK_MS             1

/* ── Framebuffer ─────────────────────────────────────────── */
#define FB_DEFAULT_WIDTH    1024
#define FB_DEFAULT_HEIGHT   768
#define FB_DEFAULT_DEPTH    32
#define FB_DOUBLE_BUFFER    1

/* ── Panel (Crash Screen) ────────────────────────────────── */
extern bool crash_screen_active;

/* ── Kernel Main ─────────────────────────────────────────── */
void kernel_main(void);

/* ── Exception Dispatch ──────────────────────────────────── */
void exception_dispatch(u32 type, u64 esr, u64 elr, u64 far);
void irq_handler(void);

#endif /* _KERNEL_H */
