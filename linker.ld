/*
 * ============================================================
 *  VibeCore OS — Linker-Script (ARM64 / aarch64)
 *  Raspberry Pi 3B (BCM2837) / QEMU raspi3b
 *  Optimized for 8GB-constrained systems.
 *
 *  Kernel is loaded to 0x80000 by the Pi firmware.
 *  .text.boot MUST be at the beginning of the binary.
 *
 *  Stack: 128 KB (saves RAM, sufficient for kernel)
 *  Heap:    1 MB   (spart RAM, mit 64KB Emergency-Reserve)
 *  Guard:   4 KB   (Stack-Schutz via Canary)
 * ============================================================
 */

ENTRY(_start)

SECTIONS
{
    /* Kernel-Startadresse */
    . = DEFINED(__kernel_base) ? __kernel_base : 0x80000;

    /* ── Boot-Code (muss zuerst kommen) ──────────────────── */
    .text.boot : {
        KEEP(*(.text.boot))
    }

    /* ── Code ────────────────────────────────────────────── */
    .text : {
        *(.text*)
    }

    /* ── Read-Only Data ──────────────────────────────────── */
    .rodata : {
        *(.rodata*)
    }

    /* ── Initialisierte Daten ────────────────────────────── */
    .data : {
        *(.data*)
    }

    /* ── Stack-Canary (separate Sektion, am BSS-Ende) ────── */
    .stack_canary : {
        . = ALIGN(64);
        KEEP(*(.stack_canary))
    }

    /* ── BSS (Null-initialisiert) ────────────────────────── */
    .bss : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(16);
        __bss_end = .;
    }

    /* ── Guard-Page (4KB, unmapped = Crash on overflow) ── */
    . = ALIGN(4096);
    __guard_page = .;
    . += 4096;

    /* ── Stack (grows downwards, 128KB) ────────────────── */
    . = ALIGN(16);
    __stack_bottom = .;
    . += 0x20000;                /* 128 KB Kernel-Stack */
    __stack_top = .;

    /* ── Heap (adaptiv, 1MB mit 64KB Emergency-Reserve) ──── */
    . = ALIGN(4096);
    __heap_start = .;
    __heap_end = . + 0x100000 + 65536;  /* 1 MB + 64KB Reserve = 1088 KB */

    /* ── Kernel-Ende ─────────────────────────────────────── */
    __kernel_end = .;

    /*
     * WICHTIG:
     *   /DISCARD/ prevents standard sections
     *   from bloating the kernel.
     */
    /DISCARD/ : {
        *(.comment)
        *(.gnu*)
        *(.note*)
        *(.eh_frame*)
        *(.debug*)
    }
}

/* Exportierte Symbole */
__bss_size = (__bss_end - __bss_start) >> 3;
