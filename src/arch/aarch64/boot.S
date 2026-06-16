/*
 * ============================================================
 *  VibeCore OS — Boot-Assembly (ARM64 / aarch64)
 *
 *  Ablauf:
 *    1. Alle Cores bis auf Core 0 in WFE-Schleife parken
 *    2. Stack-Pointer initialisieren
 *    3. BSS-Sektion nullen
 *    4. Exception-Level prüfen & ggf. wechseln
 *    5. Zu kernel_main() in C springen
 * ============================================================
 */

#include "kernel.h"

.section .text.boot

.globl _start
_start:
    /*
     * ── Core-Auswahl: Nur Core 0 soll booten ────────────
     * MPIDR_EL1 enthält die Core-ID in den unteren Bits.
     */
    mrs     x1, mpidr_el1
    and     x1, x1, #0xFF
    cbnz    x1, .Lskip_dtb

    /* Save DTB pointer ONLY on Core 0 to prevent race condition */
    ldr     x1, =dtb_ptr
    str     x0, [x1]
    b       core0_entry

.Lskip_dtb:
    /* Secondary cores go to spin loop */
    b       secondary_spin

    /* Cores 1-3: Warten auf Freigabe (Spinloop) */
secondary_spin:
    wfe
    b       secondary_spin

core0_entry:
    /*
     * ── Exception-Level prüfen ─────────────────────────
     * Wir brauchen EL1 für die Kernel-Ausführung.
     * Falls wir in EL2 oder EL3 starten (QEMU), wechseln wir.
     */
    mrs     x0, CurrentEL
    lsr     x0, x0, #2
    cmp     x0, #3
    beq     el3_entry
    cmp     x0, #2
    beq     el2_entry
    b       el1_entry

    /* ── EL3 → EL2 → EL1 ──────────────────────────────── */
el3_entry:
    /* SCR_EL3: NS=1 (Non-Secure), HCE=1 (HVC enable) */
    mrs     x0, scr_el3
    orr     x0, x0, #(1 << 0)       /* NS */
    orr     x0, x0, #(1 << 8)       /* HCE */
    msr     scr_el3, x0

    /* Setze EL2 als Return-Level */
    mov     x0, #0x3c9               /* EL2h, DAIF masked */
    msr     spsr_el3, x0
    adr     x0, el2_entry
    msr     elr_el3, x0
    eret

el2_entry:
    /* HCR_EL2: RW=1 (AArch64 für EL1) */
    mrs     x0, hcr_el2
    orr     x0, x0, #(1 << 31)      /* RW */
    msr     hcr_el2, x0

    /* Setze EL1 als Return-Level */
    mov     x0, #0x3c5               /* EL1h, DAIF masked */
    msr     spsr_el2, x0
    adr     x0, el1_entry
    msr     elr_el2, x0
    eret

el1_entry:
    /*
     * ── BSS nullen ────────────────────────────────────
     * Globale Variablen (nicht initialisiert) werden auf 0
     * gesetzt, bevor C-Code läuft.
     */
    ldr     x0, =__bss_start
    ldr     x1, =__bss_end
    sub     x2, x1, x0
    lsr     x2, x2, #3               /* Größe in 8-Byte-Blöcken */
    cbz     x2, bss_done

bss_loop:
    str     xzr, [x0], #8
    sub     x2, x2, #1
    cbnz    x2, bss_loop

bss_done:
    /*
     * ── Stack-Pointer setzen ─────────────────────────
     * Stack wächst nach UNTEN (Full Descending).
     */
    ldr     x0, =__stack_top
    mov     sp, x0

    /*
     * ── Frame-Pointer initialisieren ──────────────────
     */
    mov     x29, xzr                  /* FP = 0 */
    mov     x30, xzr                  /* LR = 0 */

    /*
     * ── Vektortabelle einrichten ──────────────────────
     * VBAR_EL1 zeigt auf unsere Exception-Vektor-Tabelle.
     */
    ldr     x0, =exception_vector_table
    msr     vbar_el1, x0

    /*
     * ── SIMD/FP-Register freigeben ────────────────────
     * CPACR_EL1: FPEN=3 (EL0+EL1 Zugriff erlaubt)
     */
    mrs     x0, cpacr_el1
    orr     x0, x0, #(3 << 20)
    msr     cpacr_el1, x0

    /* Synchronisiere Instruction-Cache */
    isb

    /*
     * ── Zu C-Code springen ───────────────────────────
     * kernel_main() kehrt NIE zurück.
     */
    bl      kernel_main

    /* Falls kernel_main doch zurückkehrt: Panic */
    b       kernel_panic_halt

/*
 * ============================================================
 *  Exception-Vektor-Tabelle (ARMv8-A)
 *
 *  Vier Eintritts-Typen × vier Exception-Levels:
 *  1. Synchronous  (z.B. Syscall, Page-Fault)
 *  2. IRQ          (Hardware-Interrupt)
 *  3. FIQ          (Fast Interrupt)
 *  4. SError       (System Error)
 *
 *  Jede Tabelle MUSS auf 2KB (2048 Bytes) aligniert sein.
 *  Jeder Eintrag ist 128 Bytes (32 Instruktionen) groß.
 * ============================================================
 */

.balign 0x800   /* 2048-Byte-Alignment für VBAR */
.globl exception_vector_table
exception_vector_table:

    /* ── EL1t (EL1 mit SP_EL0) ──────────────────────────── */
    b   exception_handler_sync        /* Synchronous */
    .balign 0x80
    b   exception_handler_irq         /* IRQ */
    .balign 0x80
    b   exception_handler_fiq         /* FIQ */
    .balign 0x80
    b   exception_handler_serror      /* SError */
    .balign 0x80

    /* ── EL1h (EL1 mit SP_EL1) ──────────────────────────── */
    b   exception_handler_sync
    .balign 0x80
    b   exception_handler_irq
    .balign 0x80
    b   exception_handler_fiq
    .balign 0x80
    b   exception_handler_serror
    .balign 0x80

    /* ── EL0 (64-bit User-Mode) ─────────────────────────── */
    b   exception_handler_sync
    .balign 0x80
    b   exception_handler_irq
    .balign 0x80
    b   exception_handler_fiq
    .balign 0x80
    b   exception_handler_serror
    .balign 0x80

    /* ── EL0 (32-bit User-Mode) ─────────────────────────── */
    b   exception_handler_sync
    .balign 0x80
    b   exception_handler_irq
    .balign 0x80
    b   exception_handler_fiq
    .balign 0x80
    b   exception_handler_serror
    .balign 0x80

/*
 * ── Exception-Handler ─────────────────────────────────────
 *  Sichern alle Register, rufen C-Handler auf,
 *  stellen Register wieder her und kehren zurück.
 */

.macro EXCEPTION_SAVE
    sub     sp, sp, #(32 * 8)        /* 256 Bytes für Register */
    stp     x0, x1, [sp, #16 * 0]
    stp     x2, x3, [sp, #16 * 1]
    stp     x4, x5, [sp, #16 * 2]
    stp     x6, x7, [sp, #16 * 3]
    stp     x8, x9, [sp, #16 * 4]
    stp     x10, x11, [sp, #16 * 5]
    stp     x12, x13, [sp, #16 * 6]
    stp     x14, x15, [sp, #16 * 7]
    stp     x16, x17, [sp, #16 * 8]
    stp     x18, x19, [sp, #16 * 9]
    stp     x20, x21, [sp, #16 * 10]
    stp     x22, x23, [sp, #16 * 11]
    stp     x24, x25, [sp, #16 * 12]
    stp     x26, x27, [sp, #16 * 13]
    stp     x28, x29, [sp, #16 * 14]
    add     x21, sp, #(32 * 8)       /* SP vor Exception */
    mrs     x22, elr_el1
    mrs     x23, spsr_el1
    stp     x30, x21, [sp, #16 * 15] /* LR (x30) + SP */
.endm

.macro EXCEPTION_RESTORE
    ldp     x30, x21, [sp, #16 * 15]
    msr     elr_el1, x22
    msr     spsr_el1, x23
    ldp     x0, x1, [sp, #16 * 0]
    ldp     x2, x3, [sp, #16 * 1]
    ldp     x4, x5, [sp, #16 * 2]
    ldp     x6, x7, [sp, #16 * 3]
    ldp     x8, x9, [sp, #16 * 4]
    ldp     x10, x11, [sp, #16 * 5]
    ldp     x12, x13, [sp, #16 * 6]
    ldp     x14, x15, [sp, #16 * 7]
    ldp     x16, x17, [sp, #16 * 8]
    ldp     x18, x19, [sp, #16 * 9]
    ldp     x20, x21, [sp, #16 * 10]
    ldp     x22, x23, [sp, #16 * 11]
    ldp     x24, x25, [sp, #16 * 12]
    ldp     x26, x27, [sp, #16 * 13]
    ldp     x28, x29, [sp, #16 * 14]
    add     sp, sp, #(32 * 8)
    eret
.endm

exception_handler_sync:
    EXCEPTION_SAVE
    mov     x0, #0                    /* Type: SYNC */
    mrs     x1, esr_el1              /* Exception Syndrome */
    mrs     x2, elr_el1              /* Exception Link Register */
    mrs     x3, far_el1              /* Fault Address Register */
    bl      exception_dispatch
    EXCEPTION_RESTORE

exception_handler_irq:
    EXCEPTION_SAVE
    mov     x0, #1                    /* Type: IRQ */
    bl      irq_handler
    EXCEPTION_RESTORE

exception_handler_fiq:
    EXCEPTION_SAVE
    mov     x0, #2                    /* Type: FIQ */
    bl      exception_dispatch
    EXCEPTION_RESTORE

exception_handler_serror:
    EXCEPTION_SAVE
    mov     x0, #3                    /* Type: SERROR */
    mrs     x1, esr_el1
    bl      exception_dispatch
    EXCEPTION_RESTORE

/*
 * ── Kernel-Panic-Halt (Endlosschleife) ────────────────────
 */
.globl kernel_panic_halt
kernel_panic_halt:
    wfi
    b       kernel_panic_halt

.section .data
.balign 8
.globl dtb_ptr
dtb_ptr:
    .quad 0
