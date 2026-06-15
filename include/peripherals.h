/*
 * ============================================================
 *  VibeCore OS — BCM2837 Peripheral Addresses
 *  Raspberry Pi 3B (BCM2837), MMIO base: 0x3F000000
 *
 *  Reference: BCM2835 ARM Peripherals Manual
 *  All offsets relative to MMIO base
 * ============================================================
 */

#ifndef _PERIPHERALS_H
#define _PERIPHERALS_H

#include "types.h"

/* ────────────────────────────────────────────────────────────
 *  MMIO Base Addresses
 * ──────────────────────────────────────────────────────────── */

#if defined(RPI4)
    /* Raspberry Pi 4B (BCM2711) — Low Peripheral Mode */
    #define MMIO_BASE       0xFE000000UL
#else
    /* Raspberry Pi 3B (BCM2837) */
    #define MMIO_BASE       0x3F000000UL
#endif

/* ────────────────────────────────────────────────────────────
 *  GPIO (General Purpose I/O)
 * ──────────────────────────────────────────────────────────── */

#define GPIO_BASE           (MMIO_BASE + 0x00200000)

/* GPIO Registers (each 32-bit) */
#define GPFSEL0             ((volatile u32*)(GPIO_BASE + 0x00))
#define GPFSEL1             ((volatile u32*)(GPIO_BASE + 0x04))
#define GPFSEL2             ((volatile u32*)(GPIO_BASE + 0x08))
#define GPFSEL3             ((volatile u32*)(GPIO_BASE + 0x0C))
#define GPFSEL4             ((volatile u32*)(GPIO_BASE + 0x10))
#define GPFSEL5             ((volatile u32*)(GPIO_BASE + 0x14))

#define GPSET0              ((volatile u32*)(GPIO_BASE + 0x1C))
#define GPSET1              ((volatile u32*)(GPIO_BASE + 0x20))
#define GPCLR0              ((volatile u32*)(GPIO_BASE + 0x28))
#define GPCLR1              ((volatile u32*)(GPIO_BASE + 0x2C))

#define GPLEV0              ((volatile u32*)(GPIO_BASE + 0x34))
#define GPLEV1              ((volatile u32*)(GPIO_BASE + 0x38))

#define GPPUD               ((volatile u32*)(GPIO_BASE + 0x94))
#define GPPUDCLK0           ((volatile u32*)(GPIO_BASE + 0x98))
#define GPPUDCLK1           ((volatile u32*)(GPIO_BASE + 0x9C))

/* GPIO Function Codes */
#define GPIO_FUNC_INPUT     0b000
#define GPIO_FUNC_OUTPUT    0b001
#define GPIO_FUNC_ALT0      0b100
#define GPIO_FUNC_ALT1      0b101
#define GPIO_FUNC_ALT2      0b110
#define GPIO_FUNC_ALT3      0b111
#define GPIO_FUNC_ALT4      0b011
#define GPIO_FUNC_ALT5      0b010

/* ────────────────────────────────────────────────────────────
 *  UART0 (PL011) — Primary Serial Port
 * ──────────────────────────────────────────────────────────── */

#define UART0_BASE          (MMIO_BASE + 0x00201000)

#define UART0_DR            ((volatile u32*)(UART0_BASE + 0x00))   /* Data Register */
#define UART0_RSRECR        ((volatile u32*)(UART0_BASE + 0x04))   /* Receive Status */
#define UART0_FR            ((volatile u32*)(UART0_BASE + 0x18))   /* Flag Register */
#define UART0_ILPR          ((volatile u32*)(UART0_BASE + 0x20))   /* IrDA */
#define UART0_IBRD          ((volatile u32*)(UART0_BASE + 0x24))   /* Integer Baud Rate */
#define UART0_FBRD          ((volatile u32*)(UART0_BASE + 0x28))   /* Fractional Baud Rate */
#define UART0_LCRH          ((volatile u32*)(UART0_BASE + 0x2C))   /* Line Control */
#define UART0_CR            ((volatile u32*)(UART0_BASE + 0x30))   /* Control Register */
#define UART0_IFLS          ((volatile u32*)(UART0_BASE + 0x34))   /* Interrupt FIFO Level */
#define UART0_IMSC          ((volatile u32*)(UART0_BASE + 0x38))   /* Interrupt Mask */
#define UART0_RIS           ((volatile u32*)(UART0_BASE + 0x3C))   /* Raw Interrupt Status */
#define UART0_MIS           ((volatile u32*)(UART0_BASE + 0x40))   /* Masked Interrupt Status */
#define UART0_ICR           ((volatile u32*)(UART0_BASE + 0x44))   /* Interrupt Clear */
#define UART0_DMACR         ((volatile u32*)(UART0_BASE + 0x48))   /* DMA Control */

/* UART Flag Bits */
#define UART_FR_TXFF        BIT(5)   /* TX FIFO Full */
#define UART_FR_RXFE        BIT(4)   /* RX FIFO Empty */
#define UART_FR_BUSY        BIT(3)   /* UART Busy */
#define UART_FR_TXFE        BIT(7)   /* TX FIFO Empty */

/* UART Control Bits */
#define UART_CR_UARTEN      BIT(0)   /* UART Enable */
#define UART_CR_TXE         BIT(8)   /* TX Enable */
#define UART_CR_RXE         BIT(9)   /* RX Enable */

/* UART Line Control Bits */
#define UART_LCRH_FEN       BIT(4)   /* FIFO Enable */
#define UART_LCRH_WLEN_8BIT (0b11 << 5)  /* 8-bit word length */

/* ────────────────────────────────────────────────────────────
 *  Mailbox (ARM ↔ VideoCore GPU)
 * ──────────────────────────────────────────────────────────── */

#define MBOX_BASE           (MMIO_BASE + 0x0000B880)

#define MBOX_READ           ((volatile u32*)(MBOX_BASE + 0x00))
#define MBOX_POLL           ((volatile u32*)(MBOX_BASE + 0x10))   /* Without interrupt */
#define MBOX_SENDER         ((volatile u32*)(MBOX_BASE + 0x14))
#define MBOX_STATUS         ((volatile u32*)(MBOX_BASE + 0x18))   /* Status */
#define MBOX_CONFIG         ((volatile u32*)(MBOX_BASE + 0x1C))
#define MBOX_WRITE          ((volatile u32*)(MBOX_BASE + 0x20))   /* Write register */

/* Mailbox Channels */
#define MBOX_CH_POWER       0   /* Power Management */
#define MBOX_CH_FB          1   /* Framebuffer */
#define MBOX_CH_VUART       2   /* Virtual UART */
#define MBOX_CH_VCHIQ       3   /* VCHIQ */
#define MBOX_CH_LEDS        4   /* LEDs */
#define MBOX_CH_BUTTONS     5   /* Buttons */
#define MBOX_CH_TOUCH       6   /* Touchscreen */
#define MBOX_CH_PROP        8   /* Property Tags (ARM → VC) */

/* Mailbox Status Bits */
#define MBOX_STATUS_FULL    BIT(31)   /* Mailbox full */
#define MBOX_STATUS_EMPTY   BIT(30)   /* Mailbox empty */

/* Mailbox Response */
#define MBOX_RESPONSE       0x80000000
#define MBOX_REQUEST_SUCCESS 0x80000000
#define MBOX_REQUEST_ERROR   0x80000001

/* ────────────────────────────────────────────────────────────
 *  System Timer
 * ──────────────────────────────────────────────────────────── */

#define TIMER_BASE          (MMIO_BASE + 0x00003000)

#define TIMER_CS            ((volatile u32*)(TIMER_BASE + 0x00))   /* Control/Status */
#define TIMER_CLO           ((volatile u32*)(TIMER_BASE + 0x04))   /* Counter Low (1MHz) */
#define TIMER_CHI           ((volatile u32*)(TIMER_BASE + 0x08))   /* Counter High */
#define TIMER_C0            ((volatile u32*)(TIMER_BASE + 0x0C))   /* Compare 0 */
#define TIMER_C1            ((volatile u32*)(TIMER_BASE + 0x10))   /* Compare 1 */
#define TIMER_C2            ((volatile u32*)(TIMER_BASE + 0x14))   /* Compare 2 */
#define TIMER_C3            ((volatile u32*)(TIMER_BASE + 0x18))   /* Compare 3 */

/* Timer Control Bits */
#define TIMER_CS_M0         BIT(0)   /* Match 0 */
#define TIMER_CS_M1         BIT(1)   /* Match 1 */
#define TIMER_CS_M2         BIT(2)   /* Match 2 */
#define TIMER_CS_M3         BIT(3)   /* Match 3 */

/* ────────────────────────────────────────────────────────────
 *  Interrupt Controller (ARM-specific)
 * ──────────────────────────────────────────────────────────── */

#define IRQ_BASE            (MMIO_BASE + 0x0000B200)

#define IRQ_BASIC_PENDING   ((volatile u32*)(IRQ_BASE + 0x00))
#define IRQ_PENDING_1       ((volatile u32*)(IRQ_BASE + 0x04))
#define IRQ_PENDING_2       ((volatile u32*)(IRQ_BASE + 0x08))
#define IRQ_FIQ_CONTROL     ((volatile u32*)(IRQ_BASE + 0x0C))
#define IRQ_ENABLE_1        ((volatile u32*)(IRQ_BASE + 0x10))
#define IRQ_ENABLE_2        ((volatile u32*)(IRQ_BASE + 0x14))
#define IRQ_ENABLE_BASIC    ((volatile u32*)(IRQ_BASE + 0x18))
#define IRQ_DISABLE_1       ((volatile u32*)(IRQ_BASE + 0x1C))
#define IRQ_DISABLE_2       ((volatile u32*)(IRQ_BASE + 0x20))
#define IRQ_DISABLE_BASIC   ((volatile u32*)(IRQ_BASE + 0x24))

/* Interrupt Sources for IRQ_ENABLE_1 */
#define IRQ_SYSTEM_TIMER_1  BIT(1)    /* System Timer Match 1 */
#define IRQ_SYSTEM_TIMER_3  BIT(3)    /* System Timer Match 3 */
#define IRQ_AUX             BIT(29)   /* Auxiliary (Mini-UART, SPI) */
#define IRQ_UART            BIT(57)   /* UART (in IRQ_ENABLE_2) */

/* ────────────────────────────────────────────────────────────
 *  ARM64 System Registers (accessible via MRS/MSR)
 *  Defined as inline assembly macros in boot.S
 * ──────────────────────────────────────────────────────────── */

/* Exception Level */
#define CURRENT_EL_SHIFT    2
#define CURRENT_EL_MASK     0b11

#endif /* _PERIPHERALS_H */
