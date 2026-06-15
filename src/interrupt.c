/*
 * ============================================================
 *  VibeCore OS — Interrupt Handling & Exception Dispatch
 *
 *  Handles:
 *    - Synchronous Exceptions (Page-Faults, Syscalls)
 *    - IRQs (Hardware Interrupts: Timer, UART)
 *    - FIQs (Fast Interrupts — reserved)
 *    - SErrors (Critical System Errors)
 *
 *  Crash Screen is activated on critical errors.
 * ============================================================
 */

#include "types.h"
#include "peripherals.h"
#include "kernel.h"
#include "string.h"
#include "framebuffer.h"
#include "mailbox.h"
#include "interrupt.h"
#include "scheduler.h"
#include "timer.h"
#include "crashlog.h"
#include "klog.h"

/* Forward declaration from timer.c */
void scheduler_tick(void);

/* ── Crash Screen State ───────────────────────────────────── */
bool crash_screen_active = false;

/* ── Stack Canary (for -fstack-protector) ────────────────── */
/*    0xDEADC0DE00C0FFEE: In bare-metal without ASLR, a       */
/*    fixed canary is still effective against typical          */
/*    buffer-overflow attacks via stack frames.                */
u64 __stack_chk_guard = 0xDEADC0DE00C0FFEEULL;

/* ────────────────────────────────────────────────────────────
 *  exception_dispatch
 *
 *  Central exception handler.
 *  Called from assembly exception vectors.
 *
 *  type: 0=SYNC, 1=IRQ, 2=FIQ, 3=SERROR
 * ────────────────────────────────────────────────────────── */

void exception_dispatch(u32 type, u64 esr, u64 elr, u64 far)
{
    switch (type) {
    case 0: /* Synchronous Exception */
        klog_fatal("SYNCHRONOUS EXCEPTION | ESR=0x%x ELR=0x%x FAR=0x%x",
                   (u32)esr, (u32)elr, (u32)far);
        crash_screen_show("SYNCHRONOUS EXCEPTION",
                          "Unexpected synchronous exception",
                          esr, elr, far);
        break;

    case 1: /* IRQ */
        /* IRQs are handled separately via irq_handler() */
        break;

    case 2: /* FIQ */
        klog_fatal("FIQ EXCEPTION | ESR=0x%x ELR=0x%x FAR=0x%x",
                   (u32)esr, (u32)elr, (u32)far);
        crash_screen_show("FIQ EXCEPTION",
                          "Fast Interrupt Request (FIQ)",
                          esr, elr, far);
        break;

    case 3: /* SError */
        klog_fatal("SYSTEM ERROR (SERROR) | ESR=0x%x ELR=0x%x FAR=0x%x",
                   (u32)esr, (u32)elr, (u32)far);
        crash_screen_show("SYSTEM ERROR (SERROR)",
                          "Critical hardware error",
                          esr, elr, far);
        break;

    default:
        klog_fatal("UNKNOWN EXCEPTION type=%d | ESR=0x%x ELR=0x%x FAR=0x%x",
                   type, (u32)esr, (u32)elr, (u32)far);
        crash_screen_show("UNKNOWN EXCEPTION",
                          "Unknown exception type",
                          esr, elr, far);
        break;
    }
}

/* ────────────────────────────────────────────────────────────
 *  irq_handler
 *
 *  Dispatches hardware interrupts to the appropriate
 *  handlers (Timer, UART, etc.).
 * ────────────────────────────────────────────────────────── */

void irq_handler(void)
{
    /* Memory barrier — ensure all prior writes commit before MMIO access */
    asm volatile("dmb sy" ::: "memory");

    /* Check base IRQ pending register */
    u32 basic_pending = *IRQ_BASIC_PENDING;
    u32 pending1 = *IRQ_PENDING_1;
    u32 pending2 = *IRQ_PENDING_2;

    /*
     * System Timer Match 1 (Scheduler Tick)
     * IRQ #1 in IRQ_PENDING_1
     */
    if (basic_pending & IRQ_SYSTEM_TIMER_1) {
        /* Timer-CS: Write-1-to-clear */
        *TIMER_CS = TIMER_CS_M1;

        /* Trigger scheduler tick */
        scheduler_tick();
        return;
    }

    /*
     * System Timer Match 3 (optional, e.g. for sleep/wakeup)
     */
    if (basic_pending & IRQ_SYSTEM_TIMER_3) {
        *TIMER_CS = TIMER_CS_M3;
        return;
    }

    /*
     * AUX (Mini-UART, SPI) — Base IRQ bit 29
     */
    if (basic_pending & IRQ_AUX) {
        /* Not yet implemented */
        return;
    }

    /* Unknown IRQ — clear the source to prevent infinite loop.
     * Level-triggered IRQs on RPi will re-fire immediately if
     * not acknowledged. Write pending registers to dismiss them. */
    klog_warn("Unhandled IRQ: basic=0x%x pend1=0x%x pend2=0x%x",
              basic_pending, pending1, pending2);
    *IRQ_BASIC_PENDING = basic_pending;
    *IRQ_PENDING_1 = pending1;
    *IRQ_PENDING_2 = pending2;
    asm volatile("dmb sy" ::: "memory");
    UNUSED(pending1);
    UNUSED(pending2);
}

/* ────────────────────────────────────────────────────────────
 *  crash_screen_show
 *
 *  Displays a detailed crash screen (bluescreen)
 *  with error diagnostics.
 *
 *  Shows:
 *    - Error type and description
 *    - Exception Syndrome Register (ESR)
 *    - Exception Link Register (ELR — fault address)
 *    - Fault Address Register (FAR)
 *    - Stack trace (symbolic)
 *    - System status
 *
 *  IMPORTANT: crash_log_write is called BEFORE the system
 *  halts. If the crash was caused by a UART fault or
 *  snprintf_local triggers a secondary exception during log
 *  writing, a recursive panic path exists. This is an
 *  acceptable risk for bare metal — the BSS buffer minimizes
 *  the window.
 * ────────────────────────────────────────────────────────── */

void crash_screen_show(const char *title, const char *desc,
                       u64 esr, u64 elr, u64 far)
{
    crash_screen_active = true;

    /* Write crash log BEFORE showing screen and halting */
    crash_log_write(title, desc, esr, elr, far);

    if (!framebuffer_is_ready()) {
        /* Fallback: UART output */
        uart_puts("\n\n");
        uart_puts("========================================\n");
        uart_puts("  VIBECORE OS — KERNEL PANIC\n");
        uart_puts("========================================\n");
        uart_printf("  %s\n", title);
        uart_printf("  %s\n\n", desc);
        uart_printf("  ESR: 0x%x\n", (u32)esr);
        uart_printf("  ELR: 0x%x\n", (u32)elr);
        uart_printf("  FAR: 0x%x\n", (u32)far);
        uart_puts("========================================\n");
        uart_puts("  System halted. Reboot required.\n");
        uart_puts("========================================\n");
    } else {
        /* Graphical crash screen */
        framebuffer_clear(COLOR_CRASH_BG);

        char buf[128];
        i32 y = 40;
        const i32 lx = 60;   /* Left text position */
        /* ── Title (white, bold) ────────────────────── */
        framebuffer_drawstring(lx, y, "VIBECORE OS — KERNEL PANIC",
                              COLOR_CRASH_FG, COLOR_CRASH_BG, 2);
        y += 40;

        /* ── Horizontal line ─────────────────────────── */
        framebuffer_drawline(lx, y, 900, y, COLOR_CRASH_FG);
        y += 20;

        /* ── Error type ──────────────────────────────── */
        framebuffer_drawstring(lx, y, "Error Type:", COLOR_YELLOW, COLOR_CRASH_BG, 2);
        framebuffer_drawstring(lx + 200, y, title, COLOR_CRASH_FG, COLOR_CRASH_BG, 2);
        y += 30;

        framebuffer_drawstring(lx, y, "Description:", COLOR_YELLOW, COLOR_CRASH_BG, 2);
        framebuffer_drawstring(lx + 200, y, desc, COLOR_CRASH_FG, COLOR_CRASH_BG, 2);
        y += 30;

        /* ── Blank line ─────────────────────────────── */
        y += 20;

        /* ── Register Dump ──────────────────────────── */
        framebuffer_drawstring(lx, y, "-- Register Dump --", COLOR_CYAN, COLOR_CRASH_BG, 2);
        y += 30;

        snprintf_local(buf, sizeof(buf), "ESR (Exception Syndrome):  0x%x", (u32)esr);
        framebuffer_drawstring(lx, y, buf, COLOR_CRASH_FG, COLOR_CRASH_BG, 1);
        y += 22;

        snprintf_local(buf, sizeof(buf), "ELR (Exception Link):      0x%x", (u32)elr);
        framebuffer_drawstring(lx, y, buf, COLOR_CRASH_FG, COLOR_CRASH_BG, 1);
        y += 22;

        snprintf_local(buf, sizeof(buf), "FAR (Fault Address):       0x%x", (u32)far);
        framebuffer_drawstring(lx, y, buf, COLOR_CRASH_FG, COLOR_CRASH_BG, 1);
        y += 40;

        /* ── ESR Decoding ────────────────────────────── */
        framebuffer_drawstring(lx, y, "-- ESR Analysis --", COLOR_CYAN, COLOR_CRASH_BG, 2);
        y += 30;

        u32 esr_ec = (u32)((esr >> 26) & 0x3F);
        snprintf_local(buf, sizeof(buf), "Exception Class (EC):      0x%x", esr_ec);
        framebuffer_drawstring(lx, y, buf, COLOR_CRASH_FG, COLOR_CRASH_BG, 1);
        y += 22;

        /* EC decoding */
        const char *ec_desc;
        switch (esr_ec) {
            case 0x00: ec_desc = "Unknown reason"; break;
            case 0x01: ec_desc = "Trapped WF* instruction"; break;
            case 0x15: ec_desc = "System Call (SVC)"; break;
            case 0x20: ec_desc = "Instruction Abort (lower EL)"; break;
            case 0x21: ec_desc = "Instruction Abort (same EL)"; break;
            case 0x24: ec_desc = "Data Abort (lower EL)"; break;
            case 0x25: ec_desc = "Data Abort (same EL) — PAGE FAULT"; break;
            case 0x2F: ec_desc = "SError interrupt"; break;
            default:   ec_desc = "See ARMv8 Reference Manual"; break;
        }
        snprintf_local(buf, sizeof(buf), "  -> %s", ec_desc);
        framebuffer_drawstring(lx, y, buf, COLOR_LIGHT_GRAY, COLOR_CRASH_BG, 1);
        y += 22;

        u32 iss = (u32)(esr & 0x1FFFFFF);
        snprintf_local(buf, sizeof(buf), "ISS (Syndrome):             0x%x", iss);
        framebuffer_drawstring(lx, y, buf, COLOR_CRASH_FG, COLOR_CRASH_BG, 1);
        y += 40;

        /* ── Footer ──────────────────────────────────── */
        framebuffer_drawline(lx, y, 900, y, COLOR_CRASH_FG);
        y += 20;

        framebuffer_drawstring(lx, y, "System halted. Please restart the system.",
                              COLOR_ORANGE, COLOR_CRASH_BG, 2);
        y += 30;

        snprintf_local(buf, sizeof(buf), "VibeCore OS v%d.%d.%d — %s",
                      VIBECORE_VERSION_MAJOR, VIBECORE_VERSION_MINOR,
                      VIBECORE_VERSION_PATCH, VIBECORE_CODENAME);
        framebuffer_drawstring(lx, y, buf, COLOR_GRAY, COLOR_CRASH_BG, 1);

        /* Swap buffers so the crash screen is visible */
        framebuffer_swap();
    }

    /* Halt system — mask all IRQs then WFI sleep
     * Without masking, the CPU keeps waking on pending interrupts
     * (level-triggered) causing cascading crash attempts. */
    *IRQ_DISABLE_BASIC = 0xFFFFFFFF;
    *IRQ_DISABLE_1 = 0xFFFFFFFF;
    *IRQ_DISABLE_2 = 0xFFFFFFFF;
    asm volatile("dsb sy; isb" ::: "memory");
    while (1) {
        asm volatile("wfi");
    }
}

/* ────────────────────────────────────────────────────────────
 *  __stack_chk_fail
 *
 *  Called by -fstack-protector when a stack canary
 *  overflow is detected.
 * ────────────────────────────────────────────────────────── */

void __stack_chk_fail(void)
{
    klog_fatal("STACK SMASHING DETECTED — Buffer overflow!");
    crash_screen_show("STACK SMASHING DETECTED",
                      "Stack canary was overwritten — Buffer overflow!",
                      0, 0, 0);
}

/* ────────────────────────────────────────────────────────────
 *  panic
 *
 *  Simple kernel panic (without detailed registers).
 *  Called by CHECK_NULL and similar macros.
 * ────────────────────────────────────────────────────────── */

void panic(const char *msg)
{
    if (msg == NULL) msg = "Unknown panic";
    klog_fatal("KERNEL PANIC: %s", msg);
    crash_screen_show("KERNEL PANIC", msg, 0, 0, 0);
}

/* ────────────────────────────────────────────────────────────
 *  snprintf_local
 *
 *  Local snprintf implementation for crash screen.
 *  Supports %s, %d, %u, %x.
 *  Returns: number of characters written (excluding \0).
 * ────────────────────────────────────────────────────────── */

static void append_char(char *buf, size_t *pos, size_t max, char c)
{
    /* Guard against max == 0 (would underflow to SIZE_MAX) */
    if (max == 0) return;
    if (*pos < max - 1) {
        buf[(*pos)++] = c;
        buf[*pos] = '\0';
    }
}

int snprintf_local(char *buf, size_t max, const char *fmt, ...)
{
    if (buf == NULL || fmt == NULL || max == 0) return 0;

    *buf = '\0';
    size_t pos = 0;

    __builtin_va_list args;
    __builtin_va_start(args, fmt);

    while (*fmt && pos < max - 1) {
        if (*fmt != '%') {
            append_char(buf, &pos, max, *fmt++);
            continue;
        }
        fmt++;
        if (*fmt == '\0') break;

        switch (*fmt) {
        case 's': {
            const char *s = __builtin_va_arg(args, const char*);
            if (s) while (*s && pos < max - 1) append_char(buf, &pos, max, *s++);
            break;
        }
        case 'd': {
            i64 n = (i64)__builtin_va_arg(args, i64);
            if (n < 0) { append_char(buf, &pos, max, '-'); n = -n; }
            if (n == 0) { append_char(buf, &pos, max, '0'); break; }
            char tmp[21]; int ti = 20; tmp[ti] = '\0';
            while (n > 0) { tmp[--ti] = '0' + (char)(n % 10); n /= 10; }
            while (tmp[ti] && pos < max - 1) append_char(buf, &pos, max, tmp[ti++]);
            break;
        }
        case 'u': {
            u64 n = (u64)__builtin_va_arg(args, u64);
            if (n == 0) { append_char(buf, &pos, max, '0'); break; }
            char tmp[21]; int ti = 20; tmp[ti] = '\0';
            while (n > 0) { tmp[--ti] = '0' + (char)(n % 10); n /= 10; }
            while (tmp[ti] && pos < max - 1) append_char(buf, &pos, max, tmp[ti++]);
            break;
        }
        case 'x': {
            u64 n = (u64)__builtin_va_arg(args, u64);
            if (n == 0) { append_char(buf, &pos, max, '0'); break; }
            char tmp[17]; int ti = 16; tmp[ti] = '\0';
            const char *hex = "0123456789abcdef";
            while (n > 0) { tmp[--ti] = hex[n & 0xF]; n >>= 4; }
            while (tmp[ti] && pos < max - 1) append_char(buf, &pos, max, tmp[ti++]);
            break;
        }
        default:
            append_char(buf, &pos, max, *fmt);
            break;
        }
        fmt++;
    }

    __builtin_va_end(args);
    return (int)pos;
}
