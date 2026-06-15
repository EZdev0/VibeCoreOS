/*
 * ============================================================
 *  VibeCore OS — System Timer Driver
 *
 *  Uses the BCM2837 System Timer (1 MHz).
 *  Provides:
 *    - timer_init()     → Configure periodic IRQs
 *    - timer_get_ticks() → Current timer counter (microseconds)
 *    - timer_usleep()   → Busy-wait in microseconds
 *    - scheduler_tick() → Periodic scheduler tick
 * ============================================================
 */

#include "types.h"
#include "peripherals.h"
#include "kernel.h"
#include "string.h"
#include "scheduler.h"

/* ── Global Tick Counters ─────────────────────────────────── */
static volatile u64 system_ticks   = 0;   /* Count since boot */
static volatile u32 tick_interval  = 0;   /* Timer compare value for 1ms */

/* ────────────────────────────────────────────────────────────
 *  timer_init
 *
 *  Initializes the system timer for periodic
 *  scheduler interrupts (1000 Hz = 1ms tick).
 *
 *  Timer runs at 1 MHz → 1 tick = 1 µs
 *  For 1000 Hz: interval = 1,000,000 / 1000 = 1000 µs = 1ms
 * ────────────────────────────────────────────────────────── */

void timer_init(void)
{
    tick_interval = 1000000 / TIMER_FREQ_HZ;  /* 1000 µs = 1ms */

    /* Read current counter value */
    u32 current = *TIMER_CLO;

    /* Set compare value: current + interval */
    *TIMER_C1 = current + tick_interval;

    /* Enable Timer Match 1 as IRQ */
    *IRQ_ENABLE_BASIC = IRQ_SYSTEM_TIMER_1;

    uart_printf("[TIMER] Initialized: %d Hz, interval=%d µs\n",
                TIMER_FREQ_HZ, tick_interval);
}

/* ────────────────────────────────────────────────────────────
 *  scheduler_tick
 *
 *  Called by irq_handler() on every timer interrupt.
 *  Updates tick counter and triggers the scheduler.
 * ────────────────────────────────────────────────────────── */

void scheduler_tick(void)
{
    system_ticks++;

    /* Set next compare value for 1ms later */
    *TIMER_C1 += tick_interval;

    /* Scheduler (if multiple tasks) */
    scheduler_preempt();
}

/* ────────────────────────────────────────────────────────────
 *  timer_get_ticks
 *
 *  Returns microseconds since boot.
 *  (System Timer: 64-bit, read in two 32-bit halves)
 * ────────────────────────────────────────────────────────── */

u64 timer_get_ticks(void)
{
    u32 hi1 = *TIMER_CHI;
    u32 lo  = *TIMER_CLO;
    u32 hi2 = *TIMER_CHI;

    /* If counter overflowed during read */
    /* cppcheck-suppress knownConditionTrueFalse */
    if (hi1 != hi2) {
        lo = *TIMER_CLO;
    }

    return ((u64)hi2 << 32) | lo;
}

/* ────────────────────────────────────────────────────────────
 *  timer_get_ms
 *
 *  Milliseconds since boot.
 * ────────────────────────────────────────────────────────── */

u64 timer_get_ms(void)
{
    return system_ticks;  /* 1 tick = 1ms */
}

/* ────────────────────────────────────────────────────────────
 *  timer_usleep
 *
 *  Busy-wait for the given number of microseconds.
 * ────────────────────────────────────────────────────────── */

void timer_usleep(u32 us)
{
    u64 start = timer_get_ticks();
    while ((timer_get_ticks() - start) < us) {
        asm volatile("nop");
    }
}

/* ────────────────────────────────────────────────────────────
 *  timer_sleep_ms
 *
 *  Busy-wait in milliseconds.
 * ────────────────────────────────────────────────────────── */

void timer_sleep_ms(u32 ms)
{
    timer_usleep(ms * 1000);
}
