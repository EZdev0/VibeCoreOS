# VibeCore OS — Architecture Documentation

> **Version:** 1.0.0 "Photon"  
> **Architecture:** ARM64 (aarch64)  
> **Target:** Raspberry Pi 3B (BCM2837) / QEMU `raspi3b`  
> **Kernel Size:** 60 KB  
> **License:** Proprietary (VibeCore Labs)  
> **Updated:** June 2026

---

## Table of Contents

1. [Overview & Philosophy](#1-overview--philosophy)
2. [Boot Process](#2-boot-process)
3. [Memory Layout](#3-memory-layout)
4. [Initialization Sequence](#4-initialization-sequence)
5. [Subsystems](#5-subsystems)
   - [UART](#uart)
   - [Mailbox Interface](#mailbox-interface)
   - [Framebuffer & GPU Graphics](#framebuffer--gpu-graphics)
   - [Exception Handling & Crash Screen](#exception-handling--crash-screen)
   - [MMU & Memory Protection](#mmu--memory-protection)
   - [BORE Scheduler](#bore-scheduler)
   - [Memory Allocator](#memory-allocator)
   - [Kernel Logging (klog)](#kernel-logging-klog)
   - [FAT32 Filesystem](#fat32-filesystem)
   - [Recovery System (8 Subsystems)](#recovery-system-8-subsystems)
   - [Crash Log](#crash-log)
   - [GUI / Desktop](#gui--desktop)
   - [Shell](#shell)
6. [Build System](#6-build-system)
7. [Testing & Analysis](#7-testing--analysis)
8. [Installation on Raspberry Pi](#8-installation-on-raspberry-pi)
9. [Security Concept](#9-security-concept)
10. [Performance Optimizations](#10-performance-optimizations)
11. [Roadmap](#11-roadmap)

---

## 1. Overview & Philosophy

VibeCore OS is a **from-scratch bare-metal operating system** for the ARM64 architecture. It runs directly on hardware — **no Linux, no existing kernel**.

### Design Principles

| Principle | Implementation |
|-----------|---------------|
| **Speed** | BORE scheduler, 128-bit STP framebuffer, -O3 compilation |
| **Stability** | NULL checks on ALL operations, graceful OOM, crash screen with ESR diagnostics |
| **Security** | Stack protection (-fstack-protector-strong), CRC32 integrity, system file protection |
| **Modern UX** | Graphical welcome/recovery screens, desktop environment, framebuffer rendering |

### Why "From Scratch"?

- Full control over every CPU instruction
- No legacy code, no unnecessary drivers
- Optimized for specific hardware (Raspberry Pi 3B, Cortex-A53)
- Educational: understand how an OS really works

---

## 2. Boot Process

```
Power-On / Reset
      │
      ▼
  GPU firmware loads kernel8.img → 0x80000
      │
      ▼
  boot.S: _start
      ├── Core selection (MPIDR_EL1)
      │     ├── Core 0 → core0_entry
      │     └── Cores 1-3 → WFE spin loop
      ├── Exception level check
      │     ├── EL3 → EL2 → EL1 (via ERET)
      │     ├── EL2 → EL1 (via ERET)
      │     └── EL1 → direct
      ├── Zero BSS (__bss_start → __bss_end)
      ├── Set stack pointer (SP = __stack_top)
      ├── Set VBAR_EL1 = exception_vector_table
      ├── Enable FP/SIMD (CPACR_EL1)
      │
      └── BL kernel_main()   ← JUMP TO C CODE
```

### Key Details

- **`kernel8.img`**: Signals 64-bit (AArch64) kernel to Pi firmware
- **Core Parking**: Only Core 0 boots; Cores 1-3 wait in WFE
- **Exception Level**: VibeCore requires EL1. If starting in EL2/EL3, ERET chain drops to EL1

---

## 3. Memory Layout

```
0x00000000 ┌──────────────────────────────────┐
           │  GPU reserved / Peripherals      │
0x00080000 ├──────────────────────────────────┤
           │  .text.boot (boot.S entry)       │
           │  .text (kernel code)             │
           │  .rodata (constants)             │
           │  .data (initialized globals)     │
           │  .bss (zero-initialized)         │
           ├──────────────────────────────────┤
           │  GUARD PAGE (4 KB, unmapped)     │  ← Stack overflow
           ├──────────────────────────────────┤
           │  KERNEL STACK (128 KB)           │  ← Grows downward
           ├──────────────────────────────────┤
           │  KERNEL HEAP (1 MB)              │  ← kmalloc
           │  EMERGENCY RESERVE (64 KB)       │  ← Crash diagnostics
           └──────────────────────────────────┘
                                                          
0x3F000000 ┌──────────────────────────────────┐
           │  MMIO (Peripherals)             │  ← GPIO, UART, Mailbox
0x40000000 └──────────────────────────────────┘
```

---

## 4. Initialization Sequence

```c
void kernel_main(void) {
    uart_init();            // Phase 0:  Debug serial (115200 8N1)
    klog_init();            // Phase 0.5: Kernel logging ring buffer
    timer_init();           // Phase 1:  System timer (1 kHz)
    framebuffer_init();     // Phase 2:  Mailbox-driven display (1024×768)
    boot_animation_run();   // Phase 2.5: Eager-rendered 20-frame sequence
    allocator_init();       // Phase 3:  Graceful OOM allocator
    mmu_init();             // Phase 4:  ARMv8 page tables (identity map)
    scheduler_init();       // Phase 5:  BORE-inspired scheduler
    crc32_self_test();      // Phase 5.5: CRC32 verification engine
    fs_init();              // Phase 6:  FAT32 RAM-disk stub
    recovery_init();        // Phase 6.5: Journal, Trash, Versioning
    snapshot_auto_create(); // Phase 6.6: Auto-snapshot at boot
    gui_desktop_init();     // Phase 7:  Desktop environment
    crash_log_init();       // Phase 8:  Previous crash check
    gui_welcome_show();     // Phase 8.5: Graphical welcome screen
    shell_run();            // Phase 9:  Interactive shell (infinite loop)
}
```

---

## 5. Subsystems

### UART

- **File:** `src/uart.c`
- **Baud rate:** 115200 (PL011, 3 MHz clock: IBRD=1, FBRD=40)
- **Configuration:** 8N1, FIFO enabled, TX+RX enabled
- **GPIO:** Pins 14 (TXD) + 15 (RXD) = ALT0
- **Functions:**
  - `uart_init()` — Initialize UART0
  - `uart_putc()` / `uart_getc()` — Blocking I/O
  - `uart_has_char()` — Non-blocking RX check (for WFI polling)
  - `uart_printf()` — Formatted output (%s, %d, %u, %x, %p, %c)

### Mailbox Interface

- **File:** `src/mailbox.c`, `include/mailbox.h`
- **Protocol:** ARM CPU ↔ VideoCore GPU communication
- **Channel 8:** Property tags (framebuffer, power, clocks)
- **Framebuffer tags:** SET_PHYSICAL_DIM (0x00048003), SET_VIRTUAL_DIM (0x00048004), ALLOCATE_BUFFER (0x00040001), SET_VIRTUAL_OFFSET (0x00048009)

### Framebuffer & GPU Graphics

- **File:** `src/framebuffer.c`
- **Resolution:** 1024×768, 32-bit ARGB (dynamic via mailbox)
- **Double-buffering:** Front + Back, swap via mailbox SET_VIRTUAL_OFFSET
- **Optimized primitives:**
  - `framebuffer_clear()` — 128-bit ARMv8 STP assembly (4 pixels/instruction)
  - `framebuffer_fillrect()` — 64-bit row blit (2 pixels/write)
  - `framebuffer_fillrow()` — 128-bit STP single-row fill (for gradients)
  - `framebuffer_pack64()` / `framebuffer_pack32()` — Shared pixel packing
  - `framebuffer_drawchar()` — Scaled via fillrect instead of per-pixel loops
  - `framebuffer_drawstring()` — Line-aware text rendering
- **Design:** All operations on uncached device memory — no D-cache pollution

### Exception Handling & Crash Screen

- **File:** `src/interrupt.c`, `src/boot.S`
- **Vector table:** 16 entries (4 exception types × 4 exception levels) at VBAR_EL1
- **Exception types:**
  - **Synchronous:** Page faults, syscalls, undefined instructions
  - **IRQ:** Hardware interrupts (timer, UART)
  - **FIQ:** Reserved (fast interrupt)
  - **SError:** Critical hardware errors
- **Crash screen (blue screen):**
  - Displays: error type, description, ESR, ELR, FAR
  - Decodes Exception Class (EC) from ESR
  - System halts after display (WFI loop)
- **Protection:** `__stack_chk_guard` + `__stack_chk_fail` for stack canary

### MMU & Memory Protection

- **File:** `src/mmu.c`
- **Configuration:**
  - 4 KB translation granule
  - 3-level page table (L0→L1→L2→L3, identity mapping)
  - MAIR: Device-nGnRE + Normal-WB attributes
  - TCR: 48-bit VA, 48-bit PA
- **Protection:**
  - Device regions: PXN + UXN (no code execution)
  - RAM: PXN (no user code in kernel space)
  - Guard pages at heap end (planned)

### BORE Scheduler

- **File:** `src/scheduler.c`
- **Inspired by:** CachyOS BORE (Burst-Oriented Response Enhancer)
- **Algorithm:**
  - Round-robin with priority queue (5 levels: IDLE→REALTIME)
  - Burst credit system: interactive tasks earn credits
  - Credits > threshold (50) → priority boost (+1)
  - Anti-starvation: Score = priority × 1000 − runtime%1000
  - Preemption: timer tick (1000 Hz)
- **Task states:** READY, RUNNING, BLOCKED, SLEEPING, ZOMBIE, DEAD
- **Max tasks:** 32

### Memory Allocator

- **File:** `src/allocator.c`
- **Type:** Bump allocator (boot phase) + tracking
- **Functions:**
  - `kmalloc(size)` — Allocate zeroed, 16-byte-aligned memory (returns NULL on OOM)
  - `kmalloc_or_panic(size)` — Critical allocation (panic on OOM)
  - `kzalloc(size)` — Explicit zero (redundancy for safety)
  - `kcalloc(count, size)` — Array allocation with overflow check
  - `kfree(ptr)` — Tracking (no real free yet)
- **Safety:**
  - Overflow check (heap boundary)
  - Alignment validation after allocation
  - Watermark monitoring at 75%/90%/95%
  - 64 KB emergency reserve for crash diagnostics
  - Stack canary (periodic check via `allocator_check_stack()`)

### Kernel Logging (klog)

- **File:** `src/klog.c`, `include/klog.h`
- **5 log levels:** DEBUG, INFO, WARN, ERROR, FATAL
- **Ring buffer:** 16 KB BSS (128 entries × 128 bytes)
- **Auto-flush:** WARN and above go to UART immediately
- **Query:** `log` / `dmesg` shell command
- **Integration:** All subsystems use klog for boot messages, exceptions, recovery events

### FAT32 Filesystem

- **File:** `src/fs.c`
- **Implementation:**
  - BPB parsing (OEM, label, cluster size)
  - FAT region / data region calculation
  - Root directory cluster localization
  - Mount/unmount with signature check (0x55AA)
- **Operations:** list, create, read, write (stubs — RAM-disk only)
- **Reference:** Microsoft FAT32 specification

### Recovery System (8 Subsystems)

- **File:** `src/recovery.c`, `include/recovery.h`

| # | Subsystem | Path | Description |
|---|-----------|------|-------------|
| 1 | **Journal** | `/.journal` | 8 KB ring buffer, 512 entries, atomic TX with commit markers |
| 2 | **Trash Bin** | `/.trash/` | Directory entry move with .meta sidecar (path + timestamp) |
| 3 | **Versioning** | `/.versions/` | Copy-on-write before each write, stores `name_timestamp` |
| 4 | **CRC32 Verify** | `/.hashes` | Hash index, verified on every `fs_read_file()` |
| 5 | **Boot Recovery** | — | Scans journal on mount, rolls back incomplete transactions |
| 6 | **System Protection** | — | Immutable file list — blocks delete AND write |
| 7 | **Snapshots** | `/.snapshots/` | Incrementing counter backups, auto-created at boot |
| 8 | **Auto-Recovery** | — | FSCK on CRC failure, no interactive blocking |

### Crash Log

- **File:** `src/crashlog.c`, `include/crashlog.h`
- Structured report: version, timestamp, ESR/ELR/FAR dump, ESL class decoding
- **Static BSS buffer** — prevents recursive panic on stack overflow
- `crash_log_check_previous()` — Detects logs from previous boot

### GUI / Desktop

- **File:** `src/gui.c`, `src/gui_welcome.c`, `src/gui_recovery.c`, `src/boot_anim.c`
- **Components:**
  - **Desktop background:** Dynamic gradient (dark blue → purple-black), row-cached
  - **Taskbar:** 40 px, start button, clock, system tray
  - **Desktop icons:** Terminal, Files, Editor, Settings, Info
  - **Windows:** Title bar, frame, close/minimize buttons
  - **Welcome screen:** Dark blue gradient, centered dialog, ENTER to continue
  - **Recovery screen:** Dark red gradient, keyboard options 1/2/3
  - **Boot animation:** 20 eager-rendered frames with progress bar
- **Rendering:** Direct framebuffer, no widget library
- **Optimized:** Gradient row caching, WFI-based input polling

### Shell

- **File:** `src/shell.c`
- **Commands:** help, clear, info, mem, tasks, fs, version, gui, log/dmesg, crash, reboot, fsck, trash, versions, recover, recovery, snapshot, protect
- **Input:** Character-by-character with backspace support

---

## 6. Build System

### Prerequisites

```bash
sudo apt install gcc-aarch64-linux-gnu binutils-aarch64-linux-gnu qemu-system-arm
# Optional:
sudo apt install cppcheck flawfinder clang-tidy dosfstools mtools zenity
```

### Targets

| Command | Description |
|---------|-------------|
| `make` | Build kernel8.img + kernel.dump |
| `make -j$(nproc)` | Parallel build (CPU-limit via `nice`) |
| `make run` | QEMU (serial console) |
| `make run-gui` | QEMU with GTK display |
| `make debug` | QEMU with GDB server (:1234) |
| `make iso` | Bootable FAT32 disk image (vibecore.img) |
| `make install` | Flash to SD card (UAC-style auth popup) |
| `make cppcheck` | Static C analysis |
| `make clang-tidy` | Code quality linting |
| `make flawfinder` | Security audit |
| `make analyze` | cppcheck + flawfinder |
| `make clean` | Remove build artifacts |

### Auth Helper

Root operations (`make install`, `make iso`) use `scripts/auth-helper.sh`:
- Detects zenity → pkexec → SSH_ASKPASS → terminal fallback
- Shows UAC-style graphical password dialog
- 5-minute session cache (one prompt per target)

---

## 7. Testing & Analysis

See [TESTING.md](../TESTING.md) for the full testing guide.

**Overview:**
```
┌─────────────────────────────────────┐
│  Hardware-in-the-Loop (Raspberry Pi)│
├─────────────────────────────────────┤
│  QEMU Emulation (aarch64)           │
├─────────────────────────────────────┤
│  Static Analysis (cppcheck, clang)   │
├─────────────────────────────────────┤
│  Compilation (-Wall -Wextra -Werror) │
└─────────────────────────────────────┘
```

---

## 8. Installation on Raspberry Pi

```bash
# 1. Format SD card (FAT32)
sudo mkfs.vfat /dev/mmcblk0p1

# 2. Build kernel
make -j$(nproc)

# 3. Install (UAC-style auth popup)
make install SDCARD=/dev/mmcblk0

# 4. Or create ISO (128 MB bootable image)
make iso
```

**Required files on boot partition:**
- `kernel8.img` — Kernel binary
- `config.txt` — `enable_uart=1\nkernel=kernel8.img\narm_64bit=1\n`
- `bcm2710-rpi-3-b.dtb` — Device tree (from Raspberry Pi Foundation)
- `start4.elf`, `fixup4.dat` — GPU firmware (from Raspberry Pi Foundation)

---

## 9. Security Concept

| Measure | Implementation |
|---------|---------------|
| **NULL checks** | `CHECK_NULL(ptr)` on all API functions |
| **Range checks** | `CHECK_RANGE(val, min, max)` for buffers |
| **Alignment checks** | `IS_ALIGNED()` after allocation |
| **Stack protection** | `-fstack-protector-strong` + `__stack_chk_fail()` |
| **NULL pointer optimization** | `-fno-delete-null-pointer-checks` |
| **Frame pointer** | `-fno-omit-frame-pointer` (stack traces) |
| **Strict alignment** | `-mstrict-align` (traps unaligned access) |
| **MMU protection** | PXN/UXN on device regions |
| **CRC32 integrity** | Auto-verify on every `fs_read_file()` |
| **System file protection** | `recovery_is_protected()` blocks delete + write |
| **Journal atomicity** | Commit markers prevent partial writes |
| **OOM resilience** | `kmalloc()` returns NULL, no panic |
| **Stack canary** | Boundary check between stack and heap |
| **Crash isolation** | System halts on crash (no data corruption) |
| **Recursive panic guard** | Crash log uses BSS buffer, not stack |

---

## 10. Performance Optimizations

| Optimization | Speedup | Technique |
|-------------|---------|-----------|
| 128-bit STP clear | ~8× | ARMv8 Store-Pair, 4× unrolled (16 pixels/loop) |
| 64-bit fillrect | ~2× | 2 pixels per write, odd-pixel handling |
| Gradient row caching | ~w× (width) | Compute once per row, bulk-fill row |
| Scaled font → fillrect | ~scale²× | 1 fillrect call per glyph bit vs scale×scale putpixels |
| WFI polling | ~99% CPU saved | Wait-For-Interrupt instead of busy-spin |
| Makefile parallel | ~nproc× | `-j$(nproc)` with nice/ionice limits |
| Pre-packed pixels | 1 shift/OR per call | `pack64()`/`pack32()` shared helpers |
| Global English | — | Zero German text in codebase |
| Auth session cache | — | 5-minute single-prompt window |

**Benchmark (framebuffer_clear, 1024×768):**
| Method | Time (1.2 GHz Cortex-A53) |
|--------|--------------------------|
| Per-pixel (original) | ~12.3 ms |
| 4-pixel loop (before) | ~5.1 ms |
| 128-bit STP (optimized) | ~1.6 ms |

---

## 11. Roadmap

### Current (v1.0.0) ✅
- [x] Boot + UART + Framebuffer (STP-optimized)
- [x] Graphical welcome/recovery screens
- [x] BORE scheduler + graceful OOM allocator
- [x] MMU + memory protection
- [x] FAT32 (RAM-disk stub)
- [x] GUI desktop (taskbar, windows, icons)
- [x] Shell (17+ commands)
- [x] 8-subsystem recovery engine
- [x] Kernel logging + crash logs
- [x] CRC32 hardware-accelerated verification
- [x] Parallel build + UAC auth popup
- [x] Static code analysis (cppcheck, flawfinder, clang-tidy)

### Future
- [ ] SD card driver (EMMC controller)
- [ ] FAT32 persistent write support
- [ ] USB HID keyboard driver
- [ ] Multi-tasking (real context switches)
- [ ] User mode (EL0) + syscalls
- [ ] Network stack (USB Ethernet)

---

## References

- **BCM2835 ARM Peripherals Manual** — Broadcom
- **ARMv8-A Architecture Reference Manual** — ARM Ltd.
- **CachyOS Wiki** — wiki.cachyos.org
- **RPi OS Tutorials** — s-matyukevich.github.io/raspberry-pi-os
- **OSDev Wiki** — wiki.osdev.org
- **FAT32 Specification (FATGEN103)** — Microsoft

---

> *"Lightning fast. Bare metal. Unstoppable."* — VibeCore Team, 2026
