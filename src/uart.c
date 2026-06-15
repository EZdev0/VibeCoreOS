/*
 * ============================================================
 *  VibeCore OS — UART / PL011 Driver
 *
 *  Provides:
 *    - uart_init()       → Initializes UART0 at 115200 baud
 *    - uart_putc()       → Send single character
 *    - uart_getc()       → Receive single character (blocking)
 *    - uart_has_char()   → Non-blocking RX check (for WFI polling)
 *    - uart_puts()       → Send null-terminated string
 *    - uart_printf()     → Formatted output (minimal)
 *
 *  Tested on: BCM2837 (RPi 3B) / QEMU raspi3b
 * ============================================================
 */

#include "types.h"
#include "peripherals.h"
#include "kernel.h"
#include "string.h"

/* ── Global UART lock for thread-safe output ─────────────── */
static volatile bool uart_lock = false;

/* ────────────────────────────────────────────────────────────
 *  uart_init
 *
 *  Configures UART0 (PL011):
 *    - 115200 baud (3 MHz UART clock)
 *    - 8 data bits, 1 stop bit, no parity
 *    - FIFO enabled
 *    - TX and RX enabled
 * ────────────────────────────────────────────────────────── */

void uart_init(void)
{
#if defined(VIRT)
    /* Virt board PL011 doesn't need GPIO pin multiplexing. Just enable it. */
    *UART0_CR = 0;
    *UART0_ICR = 0x7FF;
    *UART0_IBRD = 26;
    *UART0_FBRD = 3;
    *UART0_LCRH = (0b11 << 5);
    *UART0_CR = (1 << 0) | (1 << 8) | (1 << 9);
    return;
#endif
    register u32 temp;

    /* 1. Disable UART */
    *UART0_CR = 0;


    /* 2. Configure GPIO pins 14 & 15 for UART0 (ALT0) */
    temp = *GPFSEL1;
    temp &= ~((0b111 << 12) | (0b111 << 15));  /* Clear pins 14,15 */
    temp |= (GPIO_FUNC_ALT0 << 12) | (GPIO_FUNC_ALT0 << 15);
    *GPFSEL1 = temp;

    /* 3. Disable GPIO pull-up/down */
    *GPPUD = 0;
    temp = 150; while (temp--) { asm volatile("nop"); }  /* 150 cycle wait */
    *GPPUDCLK0 = (1 << 14) | (1 << 15);
    temp = 150; while (temp--) { asm volatile("nop"); }
    *GPPUDCLK0 = 0;

    /* 4. Disable interrupts */
    *UART0_IMSC = 0;

    /* 5. Baud rate: 115200 (UART_CLK = 3 MHz) */
    *UART0_IBRD = 1;
    *UART0_FBRD = 40;

    /* 6. Line control: 8N1, FIFO enable */
    *UART0_LCRH = UART_LCRH_WLEN_8BIT | UART_LCRH_FEN;

    /* 7. Enable UART: TX, RX, UART */
    *UART0_CR = UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE;

    uart_lock = false;
}

/* ────────────────────────────────────────────────────────────
 *  uart_putc
 *
 *  Sends a single character over UART0.
 *  Blocks until TX FIFO has space.
 * ────────────────────────────────────────────────────────── */

void uart_putc(char c)
{
    /* Wait until TX FIFO not full */
    while (*UART0_FR & UART_FR_TXFF) {
        asm volatile("nop");
    }

    /* Carriage return before line feed (for real serial terminals) */
    if (c == '\n') {
        *UART0_DR = '\r';
        while (*UART0_FR & UART_FR_TXFF) {
            asm volatile("nop");
        }
    }

    *UART0_DR = (u32)(u8)c;
}

/* ────────────────────────────────────────────────────────────
 *  uart_puts
 * ────────────────────────────────────────────────────────── */

void uart_puts(const char *str)
{
    CHECK_NULL(str);
    while (*str) {
        uart_putc(*str++);
    }
}

/* ────────────────────────────────────────────────────────────
 *  uart_getc
 *
 *  Reads a character from UART0 (blocking).
 * ────────────────────────────────────────────────────────── */

char uart_getc(void)
{
    /* Wait until RX FIFO not empty */
    while (*UART0_FR & UART_FR_RXFE) {
        asm volatile("nop");
    }
    return (char)(*UART0_DR & 0xFF);
}

/* ────────────────────────────────────────────────────────────
 *  uart_has_char
 *
 *  Non-blocking check: returns true if UART0 RX FIFO
 *  has pending data. Used with WFI polling to avoid
 *  100% CPU spin in welcome/recovery screens.
 * ────────────────────────────────────────────────────────── */

bool uart_has_char(void)
{
    /* RX FIFO empty flag: 0 = data available, 1 = empty */
    return !(*UART0_FR & UART_FR_RXFE);
}

/* ────────────────────────────────────────────────────────────
 *  uart_printf (minimale formatierte Ausgabe)
 *
 * Supported formats:
 *    %s  → String
 *    %c  → Character
 *    %d  → signed decimal (i64)
 *    %u  → unsigned decimal (u64)
 *    %x  → hex (lowercase)
 *    %X  → hex (uppercase)
 *    %p  → pointer (0x...)
 *    %%  → Prozentzeichen
 *
 *  Uses __builtin_va_* for safe varargs (freestanding).
 * ────────────────────────────────────────────────────────── */

void uart_printf(const char *fmt, ...)
{
    CHECK_NULL(fmt);

    __builtin_va_list args;
    __builtin_va_start(args, fmt);

    while (*fmt) {
        if (*fmt != '%') {
            uart_putc(*fmt++);
            continue;
        }

        fmt++; /* skip '%' */

        if (*fmt == '\0') break;

        switch (*fmt) {
        case '%':
            uart_putc('%');
            break;

        case 's': {
            const char *s = __builtin_va_arg(args, const char*);
            if (s == NULL) {
                uart_puts("(null)");
            } else {
                uart_puts(s);
            }
            break;
        }

        case 'c':
            uart_putc((char)(u8)__builtin_va_arg(args, int));
            break;

        case 'd': {
            i64 n = (i64)__builtin_va_arg(args, i64);
            if (n < 0) {
                uart_putc('-');
                n = -n;
            }
            uart_putu(n);
            break;
        }

        case 'u':
            uart_putu((u64)__builtin_va_arg(args, u64));
            break;

        case 'x':
            uart_puthex((u64)__builtin_va_arg(args, u64), false);
            break;

        case 'X':
            uart_puthex((u64)__builtin_va_arg(args, u64), true);
            break;

        case 'p':
            uart_puts("0x");
            uart_puthex((u64)__builtin_va_arg(args, u64), false);
            break;

        default:
            uart_putc('%');
            uart_putc(*fmt);
            break;
        }
        fmt++;
    }

    __builtin_va_end(args);
}
