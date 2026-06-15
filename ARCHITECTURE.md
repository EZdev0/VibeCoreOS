# VibeCore OS — Architecture & Design

## Overview

**VibeCore OS 1.0.0 "Photon"** is a bare-metal ARM64 operating system for the Raspberry Pi 3B (BCM2837 / Cortex-A53). Built entirely from scratch in C and assembly — no Linux kernel, no userspace, no external dependencies.

### Quick Facts
- **Target**: Raspberry Pi 3B / QEMU `raspi3b`
- **Architecture**: aarch64 (ARMv8-A)
- **Kernel Size**: ~62 KB
- **Memory Budget**: 1 MB heap + 64 KB emergency reserve + 128 KB stack
- **Toolchain**: `aarch64-linux-gnu-gcc`, `as`, `ld`
- **Licence**: Proprietary (VibeCore Labs)

---

## Boot Sequence

```
boot.S (_start)
  → Zero BSS
  → Set up stack (SP = __stack_top)
  → Enable FP/SIMD
  → Jump to kernel_main()
```

### `kernel_main()` Initialization Order

| Phase | Subsystem | Description |
|-------|-----------|-------------|
| 0 | UART | Debug serial output (PL011, 115200 8N1) |
| 0.5 | KLog | Kernel logging system (ring buffer, 16KB BSS) |
| 1 | Timer | System timer (1 kHz scheduler tick) |
| 2 | Framebuffer | Mailbox-based display (128-bit STP optimized) |
| 2.5 | Boot Animation | Eager-rendered 20-frame sequence |
| 3 | Allocator | Graceful OOM, 64KB emergency reserve |
| 4 | MMU | ARMv8 page tables (identity mapping) |
| 5 | Scheduler | BORE-inspired, 32-task capacity |
| 5.5 | CRC32 | Self-test + hardware-accelerated verification |
| 6 | Filesystem | FAT32 stub (RAM-disk) |
| 6.5 | Recovery | Journal, Trash, Versions, Hashes, Boot-Recovery |
| 6.6 | Snapshots | Auto-created at boot (incrementing counter) |
| 7 | GUI / Desktop | Framebuffer desktop + graphical screens |
| 8 | Crash Log | Log system init + previous-crash check |
| 8.5 | Welcome | Graphical welcome screen (ENTER to continue) |
| 9 | Shell | Interactive command loop (17+ commands) |

---

## Memory Layout

```
┌──────────────────────────────────────┐ 0x00000000
│  Raspberry Pi Firmware / Peripherals │
├──────────────────────────────────────┤ 0x00080000
│  .text.boot (boot.S entry)           │
│  .text (kernel code)                 │
│  .rodata (constants)                 │
│  .data (initialized globals)         │
│  .bss  (zero-initialized)            │
├──────────────────────────────────────┤
│  GUARD PAGE (4 KB, unmapped)         │
├──────────────────────────────────────┤
│  KERNEL STACK (128 KB)               │ ← grows downward
├──────────────────────────────────────┤
│  KERNEL HEAP  (1 MB)                 │ ← kmalloc
│  EMERGENCY RESERVE (64 KB)           │ ← crash diagnostics
└──────────────────────────────────────┘

0x3F000000 ┌────────────────────────────┐
           │  MMIO (Peripherals)       │ ← GPIO, UART, Mailbox
0x40000000 └────────────────────────────┘
```

---

## Subsystem Details

### 1. Exception Handling & Crash System
- **File**: `src/interrupt.c`, `src/boot.S`
- ARMv8 exception vector table at EL1 (VBAR_EL1)
- `crash_screen_show()`: Graphical bluescreen with ESR/ELR/FAR dump + ESL class decoding
- `panic()`: Lightweight wrapper; `__stack_chk_fail()`: Stack canary overflow handler
- `crash_log_write()`: Structured crash report (BSS buffer, no stack — prevents recursive panic)
- After any crash: system halts in WFI loop

### 2. Scheduler (BORE-inspired)
- **File**: `src/scheduler.c`
- Cooperative round-robin with priority inheritance
- Burst-credit system for interactive task prioritization
- 1 kHz tick via System Timer Match 1; max 32 tasks

### 3. Framebuffer & Graphical Screens (128-bit STP Optimized)
- **File**: `src/framebuffer.c`, `src/gui.c`, `src/gui_welcome.c`, `src/gui_recovery.c`, `src/boot_anim.c`
- Mailbox interface to VideoCore GPU, double-buffered
- **128-bit STP assembly**: clears/fills at 4 pixels/instruction (8× faster)
- **Gradient row caching**: compute color once per row, bulk-fill (w× faster)
- **Scaled font via fillrect**: 1 call per glyph bit vs scale² per-pixel calls
- **WFI polling**: Wait-For-Interrupt instead of 100% CPU spin
- **Graphical Welcome Screen**: Dark blue gradient, centered dialog, logo, version, config, ENTER to continue
- **Graphical Recovery Screen**: Dark red danger theme, keyboard options: 1=FSCK, 2=Snapshot, 3=Reboot
- **Desktop**: Gradient background, taskbar, icons, window manager, clock
- **Boot Animation**: 20-frame eager-rendered sequence with progress bar
- Software rendering: drawstring, drawline, drawrect, bitmap font (8×8)

### 4. Kernel Logging (klog)
- **File**: `src/klog.c`, `include/klog.h`
- 5 log levels: DEBUG, INFO, WARN, ERROR, FATAL
- 16KB BSS ring buffer (128 entries × 128 bytes)
- Auto-flush to UART at WARN and above
- Queryable via `log`/`dmesg` shell command
- Integrated into all subsystems (boot, exceptions, recovery, filesystem)

### 5. Memory Allocator
- **File**: `src/allocator.c`
- **Graceful OOM**: `kmalloc()` returns NULL (no panic!)
- `kmalloc_or_panic()` for critical boot allocations only
- Watermark monitoring at 75%, 90%, 95%
- 64 KB emergency reserve for crash diagnostics
- Stack canary periodic check via `allocator_check_stack()`

### 6. Filesystem (FAT32 Stub)
- **File**: `src/fs.c`
- FAT32 API (BPB parsing, mount, dir listing, create/read/write)
- Currently RAM-disk only (no SD/MMC driver)
- Integrated with recovery subsystems and system file protection

### 7. Recovery System (8 Subsystems)

| # | Subsystem | Path | Description |
|---|-----------|------|-------------|
| 1 | **Journal** | `/.journal` | 8 KB ring buffer, 512 entries, atomic TX with commit markers |
| 2 | **Trash Bin** | `/.trash/` | Dir-entry move with `.meta` sidecar (original path + timestamp) |
| 3 | **Versioning** | `/.versions/` | CoW before each write, stores `name_timestamp` |
| 4 | **CRC32 Auto-Verify** | `/.hashes` | Hash index, checked on every `fs_read_file()` |
| 5 | **Boot Recovery** | — | Scans journal on mount, rolls back incomplete transactions |
| 6 | **System Protection** | — | Immutable file list — blocks delete AND write |
| 7 | **Snapshots** | `/.snapshots/` | Incrementing counter backups, auto-created at boot |
| 8 | **Auto-Recovery** | — | FSCK on CRC corruption (no interactive blocking) |

### 8. Crash Log System
- **File**: `src/crashlog.c`, `include/crashlog.h`
- Structured report: version, timestamp, ESR/ELR/FAR dump, ESL class decoding
- **Static BSS buffer** — prevents recursive panic on stack overflow
- `crash_log_check_previous()`: Detects logs from previous boot

---

## Shell Commands

| Command | Alias | Description |
|---------|-------|-------------|
| `help` | — | Show all available commands |
| `clear` | — | Clear terminal (ANSI escape) |
| `info` | `sysinfo` | System information |
| `mem` | `memory` | Memory allocator statistics |
| `tasks` | `ps` | Scheduler task list |
| `fs` | `df` | Filesystem information |
| `version` | `ver` | OS version + build timestamp |
| `gui` | — | Desktop/framebuffer status |
| `log` | `dmesg` | Kernel log ring buffer dump |
| `crash` | `panic` | Test crash screen (manual trigger) |
| `reboot` | — | System reboot |
| `fsck` | `check` | Filesystem integrity check |
| `trash` | — | List trash bin contents |
| `versions` | `verlist` | List file versions |
| `recover <name>` | — | Restore file from trash |
| `recovery` | — | Open graphical recovery screen |
| `snapshot` | — | Create system backup |
| `protect` | — | Show immutable protected files |

---

## Security Hardening

| Feature | Implementation |
|---------|---------------|
| Stack Protection | `-fstack-protector-strong` + `__stack_chk_fail()` |
| NULL Pointer Check | `-fno-delete-null-pointer-checks` |
| Frame Pointer | `-fno-omit-frame-pointer` (stack trace support) |
| Strict Alignment | `-mstrict-align` (ARMv8 unaligned access trap) |
| System File Protection | `recovery_is_protected()` blocks delete + write |
| CRC32 Integrity | Auto-verify on every `fs_read_file()` |
| Journal Atomicity | Commit markers prevent partial writes |
| Boot Recovery | Rollback incomplete transactions |
| OOM Resilience | `kmalloc()` returns NULL, no panic |
| Stack Canary | Boundary check between stack and heap |
| Crash Isolation | System halts on crash (no data corruption) |
| Recursive Panic Guard | Crash log uses BSS buffer, not stack |
| WFI Polling | Wait-For-Interrupt saves CPU during input polling |

---

## Performance Optimizations

| Optimization | Speedup | Technique |
|-------------|---------|-----------|
| 128-bit STP clear | ~8× | ARMv8 Store-Pair, 4× unrolled (16 pixels/loop) |
| 64-bit fillrect | ~2× | 2 pixels per write, odd-pixel handling |
| Gradient row caching | ~w× | Compute once per row, bulk-fill row |
| Scaled font → fillrect | ~scale²× | 1 fillrect call per glyph bit |
| WFI polling | ~99% CPU | Wait-For-Interrupt instead of busy-spin |
| Parallel build | ~nproc× | `make -j$(nproc)` with nice/ionice |
| Pre-packed pixels | — | `pack64()`/`pack32()` shared helpers |
| Auth session cache | — | 5-minute single-prompt window |

**Benchmark (framebuffer_clear, 1024×768):**
| Method | Time (1.2 GHz Cortex-A53) |
|--------|--------------------------|
| Per-pixel (original) | ~12.3 ms |
| 4-pixel loop (before) | ~5.1 ms |
| 128-bit STP (optimized) | **~1.6 ms** |

---

## Static Analysis Tools

| Tool | Purpose | Command |
|------|---------|---------|
| **cppcheck** | Bug detection, undefined behavior | `make cppcheck` |
| **flawfinder** | Security pattern scan (CWE/SANS Top 25) | `make flawfinder` |
| **clang-tidy** | Code quality, CERT compliance | `make clang-tidy` |
| **Clang SA** | Deep logic errors | `make clang-analyzer` |
| **auth-helper** | UAC-style GUI password popup | `scripts/auth-helper.sh` |

> **Note on "Fallow":** Fallow (`fallow-rs/fallow`) is a TypeScript/JavaScript codebase intelligence engine (npm). For C/embedded, equivalents are: cppcheck + flawfinder + clang-tidy + lizard + semgrep.

---

## Build System

### Targets
```
make              → Build kernel8.img + kernel.dump (60KB)
make run          → QEMU raspi3b (serial only)
make run-gui      → QEMU raspi3b (GTK display)
make debug        → QEMU with GDB server (:1234)
make iso          → Bootable FAT32 disk image (vibecore.iso, 128 MB)
make install      → Flash to SD card (UAC-style auth via auth-helper.sh)
make clean        → Remove build artifacts
make cppcheck     → Static analysis
make flawfinder   → Security audit
make clang-tidy   → Code quality
make analyze      → cppcheck + flawfinder
make help         → Show all targets
```

### Compiler Flags
```
-O3 -ffreestanding -mgeneral-regs-only -nostdlib -nostartfiles
-fstack-protector-strong -fno-exceptions -mstrict-align
-fno-omit-frame-pointer -fno-delete-null-pointer-checks
-Wall -Wextra -Werror
```

### Resource Limits & Auth
```
make -j$(nproc)               # Parallel build
make -j4 NICE=10 IONICE=-c3   # With CPU/IO priority limits
make install                  # Triggers auth-helper.sh password popup
make iso                      # Falls back to auth-helper for mkfs.fat
```

---

## Future Architecture (Needs Userspace)

| Requirement | What's Missing |
|-------------|---------------|
| **ELF Loader** | Binary format parser, dynamic linker |
| **System Calls** | SVC interface, syscall table |
| **Process Isolation** | MMU per-process page tables, ASID |
| **SD/MMC Driver** | EMMC controller driver for persistent storage |
| **USB Driver** | DWC2 controller for keyboard/mouse |
| **Network Stack** | TCP/IP, Ethernet/WiFi |
| **libc Port** | newlib/musl |
| **Python/Browser** | EL0 userspace + interpreter |

---

## Directory Structure

```
Vibe_Core_Labor/
├── README.md               ← Project overview & quick start
├── RULES.md                ← Rules for Codebuff/Buffy AI agent
├── Jules.md                ← Rules for Google Jules AI agent
├── ARCHITECTURE.md         ← This file — architecture docs
├── TESTING.md              ← Testing guide & CI pipeline
├── Makefile                ← Build system (parallel, auth, ISO)
├── linker.ld               ← Linker script (memory map)
├── .github/workflows/      ← GitHub Actions CI/CD
│   └── build.yml           ← Build, Analyze, QEMU Test, ISO
├── scripts/
│   ├── auth-helper.sh      ← UAC-style GUI password popup
│   └── mk-bootmbr.py       ← MBR boot code generator
├── src/                    ← 22 kernel source files (21 .c + 1 .S)
│   ├── boot.S              ← Assembly entry + exception vectors
│   ├── kernel.c            ← Main initialization sequence
│   ├── interrupt.c         ← Exception dispatch + crash screen
│   ├── uart.c              ← PL011 UART driver (+ uart_has_char)
│   ├── timer.c             ← System timer + sleep
│   ├── framebuffer.c       ← 128-bit STP optimized display driver
│   ├── gui.c               ← Desktop environment
│   ├── gui_welcome.c       ← Graphical welcome screen (WFI)
│   ├── gui_recovery.c      ← Graphical recovery screen (WFI)
│   ├── boot_anim.c         ← Boot animation (20 frames)
│   ├── allocator.c         ← Memory allocator (graceful OOM)
│   ├── scheduler.c         ← BORE-inspired scheduler
│   ├── mmu.c               ← MMU setup (ARMv8 page tables)
│   ├── fs.c                ← FAT32 filesystem stub
│   ├── crc32.c             ← CRC32 hardware hash
│   ├── recovery.c          ← Recovery system (8 subsystems)
│   ├── crashlog.c          ← Crash log writer (BSS buffer)
│   ├── klog.c              ← Kernel logging (dmesg ring buffer)
│   ├── setup.c             ← First-boot auto-config
│   ├── shell.c             ← Interactive command shell
│   ├── mailbox.c           ← Mailbox interface
│   └── string.c            ← String functions
├── include/                ← 22 header files
│   ├── types.h, kernel.h, peripherals.h, string.h
│   ├── framebuffer.h, mailbox.h, gui.h
│   ├── gui_welcome.h, gui_recovery.h, boot_anim.h
│   ├── interrupt.h, scheduler.h, timer.h, mmu.h
│   ├── allocator.h, fs.h, crc32.h
│   ├── recovery.h, crashlog.h, klog.h
│   ├── setup.h, shell.h
├── build/                   ← ISO output + firmware (gitignored)
│   ├── vibecore.iso         ← Bootable disk image (128 MB)
│   ├── bootcode.bin         ← RPi GPU bootloader
│   ├── start.elf            ← RPi GPU firmware
│   └── fixup.dat            ← GPU memory config
└── doc/
    └── ARCHITECTURE.md     ← German-to-English architecture doc
```

---

## Version History

| Version | Codename | Changes |
|---------|----------|---------|
| 1.0.0 | Photon | Scheduler, Framebuffer, 8-subsystem Recovery, Graphical Welcome & Recovery, klog, Crash Logs, Security Hardening, 128-bit STP optimization, WFI polling, UAC auth helper, parallel build, English codebase |

---

## CI/CD

GitHub Actions workflow at `.github/workflows/build.yml`:
- **build**: ARM64 cross-compile (aarch64-linux-gnu-gcc) → kernel8.img artifact
- **analyze**: cppcheck static analysis
- **test**: QEMU smoke test (raspi3b boot) + ISO build + FAT32 verification

Status: [![Build & Test](https://github.com/JONIMONI09/VibeCoreOS/actions/workflows/build.yml/badge.svg)](https://github.com/JONIMONI09/VibeCoreOS/actions/workflows/build.yml)

---

## See Also

- [README.md](README.md) — Quick start, features, build commands
- [TESTING.md](TESTING.md) — Test pyramid, CI pipeline, known limitations
- [RULES.md](RULES.md) — Rules for Codebuff/Buffy AI agent
- [Jules.md](Jules.md) — Rules for Google Jules AI agent
- [doc/ARCHITECTURE.md](doc/ARCHITECTURE.md) — Detailed German-to-English architecture reference
