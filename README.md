# VibeCore OS 1.0.0 — "Photon"

[![Build & Test](https://github.com/JONIMONI09/VibeCoreOS/actions/workflows/build.yml/badge.svg)](https://github.com/JONIMONI09/VibeCoreOS/actions/workflows/build.yml)

**Bare-metal ARM64 operating system for Raspberry Pi 3B/4B** — built entirely from scratch in C and ARMv8 Assembly. No Linux kernel, no userspace, no external dependencies. Just pure bare-metal code, running directly on the Cortex-A53/A72.

```
╔══════════════════════════════════════════════════╗
║      VibeCore OS 1.0 — "Photon" (aarch64)       ║
║      Lightning fast. Bare metal. Unstoppable.    ║
╚══════════════════════════════════════════════════╝
         Kernel: 62 KB  |  aarch64  |  Cortex-A53/A72
```

---

## 🚀 Quick Start

```bash
# Prerequisites
sudo apt install gcc-aarch64-linux-gnu qemu-system-arm

# Build (62 KB kernel)
make -j$(nproc)

# Run in QEMU (terminal)
make run

# Run in QEMU with graphics window
make run-gui

# Create SD image (128 MB, bootable)
make iso

# Verify SD image
make iso-verify
```

---

## 📋 Build Commands

| Command | Description |
|---------|-------------|
| `make` / `make -j$(nproc)` | Build kernel (parallel, all cores) |
| `make run` | QEMU raspi3b — serial console (NOT VM boot!) |
| `make run-gui` | QEMU with GTK graphics window |
| `make debug` | QEMU with GDB server on port 1234 |
| `make firmware` | Download RPi firmware (one-time) |
| `make iso` | Bootable raw SD image (128 MB, auto: GUI→full / headless→base) |
| `make iso-noroot` | Rootless SD image WITHOUT root privileges, NO password |
| `make iso-full` | Full SD image with files (needs desktop GUI for auth popup) |
| `make iso-verify` | Verify SD image: partition table + FAT32 signature |
| `make iso-test` | Test SD image in QEMU (kernel direct + SD image as block device) |
| `make iso-flash` | Flash ISO to SD card (desktop GUI auth) |
| `make clean` | Remove build artifacts |
| `make help` | Show all commands |
| `make check` | Pre-commit quick check (compile + cppcheck) |
| `make harden` | Hardened build (stack-clash-protection) |
| `make harden-test` | Hardened build + QEMU boot test |
| `make cppcheck` | Static C analysis (bug & UB detection) |
| `make flawfinder` | Security audit (CWE/SANS Top 25) |
| `make fanalyzer` | GCC deep analysis (use-after-free, overflow, NULL) |
| `make security` | Security audit summary |
| `make logic` | Logic check summary |
| `make audit` | Full audit: cppcheck + flawfinder + fanalyzer |
| `make analyze` | cppcheck + flawfinder |
| `make clang-tidy` | Code quality linting |
| `make clang-analyzer` | Clang deep logic errors |

---

## 🐚 Shell Commands

| Command | Alias | Description |
|---------|-------|-------------|
| `help` | — | Show all available commands |
| `clear` | — | Clear terminal |
| `info` | `sysinfo` | System information |
| `mem` | `memory` | Memory statistics (heap, stack canary) |
| `tasks` | `ps` | Scheduler task list |
| `fs` | `df` | Filesystem info |
| `version` | `ver` | OS version + build info |
| `gui` | — | Desktop/framebuffer status |
| `log` | `dmesg` | Kernel log ring buffer dump |
| `crash` | `panic` | Test crash screen (manual trigger) |
| `reboot` | — | System restart |
| `fsck` | `check` | Filesystem integrity check |
| `trash` | — | Show trash bin contents |
| `versions` | `verlist` | List file versions |
| `recover <name>` | — | Restore file from trash |
| `recovery` | — | Open graphical recovery screen |
| `snapshot` | — | Create system backup |
| `protect` | — | Show protected files |

---

## 🏗️ Project Structure

```
Vibe_Core_Labor/
├── README.md                  ← This file (EN)
├── RULES.md                   ← Project rules & AI agent guide (EN)
├── Jules.md                   ← Google Jules AI agent rules (EN)
├── ARCHITECTURE.md            ← Detailed architecture docs (EN)
├── TESTING.md                 ← Testing guide & CI setup (EN)
├── BRAINSTORM.md              ← Roadmap, ideas & future vision (EN)
├── Makefile                   ← Build system (parallel, auth, ISO)
├── linker.ld                  ← Linker script (memory map)
├── config.txt                 ← RPi boot configuration
├── .github/                   ← GitHub Actions CI/CD
│   └── workflows/
│       └── build.yml          ← Build, analyze, QEMU test, ISO
├── scripts/                   ← ALL scripts & tools
│   ├── auth-helper.sh         ← Desktop GUI password popup (zenity/pkexec)
│   └── mk-bootmbr.py          ← MBR boot code generator (VM compatibility)
├── src/                       ← Kernel source code (22 files)
│   ├── boot.S                 ← ARMv8 assembly entry point
│   ├── kernel.c               ← Main initialization
│   ├── framebuffer.c          ← 128-bit STP display driver
│   ├── mailbox.c              ← ARM↔GPU communication
│   ├── interrupt.c            ← Exception dispatch + crash screen
│   ├── uart.c                 ← PL011 UART driver
│   ├── timer.c                ← System timer (1 kHz tick)
│   ├── allocator.c            ← Memory allocator (graceful OOM)
│   ├── scheduler.c            ← BORE scheduler
│   ├── mmu.c                  ← ARMv8 page tables (currently disabled)
│   ├── fs.c                   ← FAT32 filesystem (RAM disk)
│   ├── crc32.c                ← CRC32 hardware hash
│   ├── recovery.c             ← 8-subsystem recovery engine
│   ├── crashlog.c             ← Crash report writer (BSS buffer)
│   ├── klog.c                 ← Kernel logging (dmesg ring buffer)
│   ├── gui.c                  ← Desktop environment
│   ├── gui_welcome.c          ← Graphical welcome screen
│   ├── gui_recovery.c         ← Graphical recovery screen
│   ├── boot_anim.c            ← Boot animation (20 frames)
│   ├── shell.c                ← Interactive command shell
│   ├── setup.c                ← First-boot auto-configuration
│   └── string.c               ← String/memory utilities
├── include/                   ← Header files (22 files)
│   ├── types.h                ← Type definitions + macros
│   ├── kernel.h               ← Global kernel definitions
│   ├── peripherals.h          ← BCM2837 MMIO addresses
│   ├── framebuffer.h          ← Graphics API
│   ├── mailbox.h              ← Mailbox tag definitions
│   └── ...                    ← (17 more headers)
├── build/                     ← Build output & firmware
│   ├── vibecore-rpi.img           ← Bootable image (128 MB, MBR+FAT32)
│   ├── bootcode.bin           ← RPi GPU bootloader (52 KB)
│   ├── start.elf              ← RPi GPU firmware (2.9 MB)
│   └── fixup.dat              ← GPU memory configuration (7 KB)
```

---

## 💿 ISO/Image System

### Overview

The build system creates `build/vibecore-rpi.img` — an **MBR+FAT32 disk image**, NOT an ISO 9660.
This is the same format as Ubuntu RPi images and Raspberry Pi OS.

### SD Image Targets

| Target | Description | Root? | GUI? |
|--------|-------------|-------|------|
| `make iso` | Auto: GUI detected → `iso-full`, else → `iso-noroot` | Auto | Auto |
| `make iso-full` | Full image with all files (128 MB) | Yes (losetup) | Yes (zenity) |
| `make iso-noroot` | Rootless image with kernel8.img, config.txt, EFI diagnostic stub, optional firmware | **NO** | No |

### Image Contents (iso-full)

```
MBR (boot code + partition table)
└── Partition 1 (FAT32, bootable, 127 MB)
    ├── kernel8.img      (62 KB)  — Operating system kernel
    ├── config.txt        (682 B)  — Boot configuration
    ├── bootcode.bin      (52 KB)  — GPU first-stage bootloader
    ├── start.elf         (2.9 MB) — GPU firmware
    └── fixup.dat         (7 KB)   — GPU memory config
```


### Boot Support Matrix (as of 2026-06-18)

| Platform | Firmware/Boot Path | Artifact/Command | Status | Limitation |
|---|---|---|---|---|
| QEMU Raspberry Pi 3B | QEMU `-kernel` Direct Boot | `make run` | Supported | QEMU does not fully emulate the RPi firmware chain; the SD image is not directly booted here. |
| Raspberry Pi 3B SD | GPU firmware loads `kernel8.img` from FAT | `make firmware && make iso-full` or `make iso-noroot` with firmware in `build/` | Experimental | Requires `bootcode.bin`, `start.elf`, `fixup.dat`, and `config.txt` on the FAT boot partition. |
| Raspberry Pi 4B SD | Pi 4 firmware/EEPROM + FAT boot partition | No dedicated board target yet | Not verified | BCM2711/Cortex-A72 and Pi-4-specific MMIO/firmware details are not yet cleanly separated. |
| AArch64 UEFI | `EFI/BOOT/BOOTAA64.EFI` | Diagnostic stub in SD image | Diagnostic only | The stub only prints a message; it does not load `kernel8.img` yet. |
| x86 BIOS/Legacy VM | MBR Real Mode | MBR diagnostic code | Diagnostic only | The MBR shows a message and halts; it is not an ARM64 or x86 kernel bootloader. |
| VirtualBox/virt-manager `.iso` boot | ISO 9660/El Torito or UEFI | Not `build/vibecore-rpi.img` | Not supported | The artifact is a raw SD/disk image, not an ISO 9660 CD-ROM image. |

### MBR Boot Code

The image contains **440 bytes of MBR boot code** (generated by `scripts/mk-bootmbr.py`).
The code displays a BIOS message and halts — making the image recognizable by VMs.

```
Bytes 0-439:   Boot code (x86 real-mode, "VibeCore OS ARM64 — Boot via QEMU: make run")
Bytes 440-445: Disk signature
Bytes 446-509: Partition table (sfdisk)
Bytes 510-511: Boot signature (0x55 0xAA) ✅
```

### Flashing to SD Card

```bash
# With auth-helper (desktop GUI popup)
make iso-flash SDCARD=/dev/mmcblk0

# Manual
dd if=build/vibecore-rpi.img of=/dev/mmcblk0 bs=4M status=progress
```

---

## 🖥️ VM / Emulator

### IMPORTANT: No Direct VM Boot!

VibeCore OS is a **bare-metal kernel** for Raspberry Pi. It does NOT boot in a VM like VirtualBox or virt-manager because:

1. **UEFI firmware** (virt-manager, GNOME Boxes) can find `BOOTAA64.EFI`, but it is currently a diagnostic stub and does not load the OS kernel
2. **BIOS** executes MBR code → shows message only, no real boot
3. Raspberry Pi boots via **GPU firmware** → no BIOS, no UEFI

### ✅ Correct Way: QEMU Direct Boot

```bash
make run        # Terminal mode
make run-gui    # With graphics window
```

This works because QEMU uses `-kernel` to load the kernel directly and emulates RPi hardware — without BIOS/UEFI.

### 🔧 Auth Helper (No Terminal Password!)

```bash
scripts/auth-helper.sh <command>
```

The auth helper:
- Shows **ONLY desktop GUI popups** (zenity or pkexec)
- **NO terminal password** — prevents terminal corruption
- **5-minute cache** — one popup per build session
- If no GUI → clear error message, no blocking

---

## 🏛️ Architecture

### Boot Sequence

```
Power-On → GPU loads kernel8.img → boot.S (_start)
  → Zero BSS → Init stack → Enable FP/SIMD
  → kernel_main():
      0.  UART           (Debug serial, 115200 8N1)
      0.5 klog           (Kernel logging ring buffer)
      1.  Timer          (System timer, 1 kHz)
      2.  Framebuffer    (Mailbox-GPU, 1024×768 32-bit)
      3.  Boot Animation (20 frames, gradient progress bar)
      4.  Allocator      (Graceful OOM, 64KB emergency)
      5.  MMU            (currently disabled)
      6.  Scheduler      (BORE-inspired, 32 tasks)
      7.  CRC32          (Self-test)
      8.  Filesystem     (FAT32 RAM disk)
      9.  Recovery       (8 subsystems)
      10. Snapshots      (Auto-backup at boot)
      11. GUI/Desktop    (Framebuffer desktop)
      12. Crash Log      (Check previous crashes)
      13. Welcome Screen (Graphical, press ENTER)
      14. Shell          (Interactive, 17+ commands)
```

### Memory Layout

```
0x00000000 ┌──────────────────────────┐
           │  GPU / Peripherals        │
0x00080000 ├──────────────────────────┤
           │  .text.boot               │  ← Boot code
           │  .text / .rodata / .data  │  ← Kernel
           │  .bss (zero-init)         │
           ├──────────────────────────┤
           │  GUARD PAGE (4 KB)        │  ← Stack protection
           ├──────────────────────────┤
           │  STACK (128 KB)           │  ← Grows downward
           ├──────────────────────────┤
           │  HEAP (1 MB)              │  ← kmalloc/kzalloc
           │  EMERGENCY (64 KB)        │  ← Crash diagnostics
           ├──────────────────────────┤
           │  Framebuffer (GPU-alloc)  │  ← 0x3C100000
           └──────────────────────────┘
```

---

## 📊 Statistics

| Metric | Value |
|--------|-------|
| Kernel size | 62 KB |
| Source files | 22 (.c + .S) + 22 (.h) = 44 |
| Build time (parallel) | ~2 seconds |
| Boot time (QEMU) | ~1.6 seconds |
| Graphics resolution | 1024×768 (32-bit ARGB) |
| Heap size | 1 MB + 64 KB emergency |
| Stack size | 128 KB |
| Max tasks | 32 |
| Idle CPU | WFI sleep (~0%) |

---

## 🐛 Known Bugs & Status

### Recently Fixed (2026-06-18)

| Bug | File | Fix |
|-----|------|-----|
| `framebuffer_fillrect` trailing pixel not drawn | `framebuffer.c` | `*(u32*)line64 = pixel` for odd-width rows |
| Missing memory barrier after mailbox read | `mailbox.c` | Added `dmb sy` before `buffer[1]` check |
| `snprintf_local` va_arg read 64-bit for 32-bit int | `interrupt.c` | `va_arg(args, int)` instead of `va_arg(args, i64)` |
| ELR/SPSR not saved to stack (context-switch blocked) | `boot.S` | Extended frame 32→34 slots, `stp` at offset #16×16 |
| BSS trailing bytes not zeroed (1-7 byte remainder) | `boot.S` | Added `strb` loop for non-8-byte-aligned BSS |
| STP inline asm missing early-clobber constraint | `framebuffer.c` | `"+&r"` constraint on `ptr` output |
| Allocator stats underflow in emergency mode | `allocator.c` | `total` uses `heap_limit` when emergency active |
| `mmu.c` dead if/else (both branches set WB) | `mmu.c` | MAIR_IDX_DEVICE for MMIO, NORMAL_WB for RAM |
| `recovery.c` dead branch (stub always returns 0) | `recovery.c` | Removed dead `if` block, added TODO |
| EFI structs with 10+ unused named members | `uefi/main.c` | Minimized to `_pad*` arrays (ABI offsets preserved) |
| Installer: `execvp` + `access` race condition | `installer.c` | Command whitelist + `open()` instead of `access()` |
| SD image had misleading `.iso` extension | `Makefile` | Renamed to `build/vibecore-rpi.img` (raw SD image, NOT ISO 9660) |
| UEFI boot path missing → VM "not bootable" | `Makefile.efi` | Added `BOOTAA64.EFI` diagnostic stub in `EFI/BOOT/` |

### Recently Fixed (2026-06-15)

| Bug | File | Fix |
|-----|------|-----|
| Translation to English | Multiple Files | Translated all German comments, prints and variables to English. |
| GitHub Workflow Artifact limits | `.github/workflows/build.yml` | Implemented fallback to Github Release for artifacts limit overflow. |
| Directory Cleanup | Multiple | Moved documentation files into a `docs` folder. |
| `mailbox_call` checked wrong return value | `mailbox.c` | `buffer[1] == MBOX_RESPONSE` instead of `result == 0` |
| GPU address read from wrong slot | `framebuffer.c` | `buf[23]` (base) + `buf[24]` (size) instead of `buf[22]` |
| `framebuffer_fillrect` alignment fault (device memory) | `framebuffer.c` | `IS_ALIGNED(buf, 8)` check + 32-bit fallback |
| ISO extension `.img` instead of `.iso` | `Makefile` | Renamed to `build/vibecore-rpi.img` (raw SD image, NOT ISO 9660) |
| MBR without boot code → VM "not bootable" | `mk-bootmbr.py` | 440-byte MBR boot code generated |
| IRQ infinite loop on unknown IRQs | `interrupt.c` | Write-1-to-clear + dmb barrier |
| `snprintf_local` buffer overflow | `interrupt.c` | `max == 0` guard before write |
| Auth-helper terminal fallback | `auth-helper.sh` | Desktop GUI only |


---

## 🔒 Security

| Feature | Implementation |
|---------|----------------|
| Stack protection | `-fstack-protector-strong` + `__stack_chk_fail()` |
| NULL pointer check | `-fno-delete-null-pointer-checks` |
| Frame pointer | `-fno-omit-frame-pointer` (stack trace) |
| Strict alignment | `-mstrict-align` (ARMv8 alignment trap) |
| CRC32 integrity | Auto-verify on every `fs_read_file()` |
| Journal atomicity | Commit markers prevent partial writes |
| OOM resilience | `kmalloc()` returns NULL, no panic |
| Stack canary | Check between stack and heap |
| Crash isolation | System halts on crash (no data corruption) |
| WFI polling | Wait-For-Interrupt instead of busy-spin |
| Stack clash protection | `-fstack-clash-protection` (on harden build) |
| Static security audit | `make flawfinder` (CWE/SANS Top 25) |
| Deep GCC analysis | `make fanalyzer` (use-after-free, overflow, NULL) |
| Bug detection | `make cppcheck` (undefined behavior, memory leaks) |

---

## ⚡ Performance Optimizations

| Optimization | Speedup | Technique |
|-------------|---------|-----------|
| 128-bit STP clear | ~8× | ARMv8 store-pair, 4× unrolled |
| 64-bit fillrect | ~2× | 2 pixels per write operation |
| Gradient row caching | ~w× | Compute color once per row |
| WFI polling | ~99% CPU | Wait-For-Interrupt and Scheduler yield instead of spin |
| Parallel build | ~nproc× | `make -j$(nproc)` |

---

## 🔧 Dependencies

```bash
# Mandatory (build)
gcc-aarch64-linux-gnu          # ARM64 cross compiler
binutils-aarch64-linux-gnu     # Assembler, linker, objcopy

# Recommended (test)
qemu-system-arm                # QEMU emulation

# Recommended (analysis)
cppcheck                       # Static C analysis (v2.17.1)
flawfinder                     # Security scan (v2.0.20)
                              # Install: pip3 install --break-system-packages --user flawfinder

# Optional (SD image)
dosfstools                     # mkfs.fat
mtools                         # mcopy, mmd (no root)
python3                        # MBR generator

# Optional (desktop auth)
zenity                         # GUI password dialog
```

---

## 📚 Documentation

| Document | Content |
|----------|--------|
| [README.md](README.md) | This file — overview & quick start |
| [ARCHITECTURE.md](docs/ARCHITECTURE.md) | Complete architecture: boot, memory, subsystems |
| [TESTING.md](docs/TESTING.md) | Test pyramid: compilation, analysis, QEMU, hardware |
| [RULES.md](docs/RULES.md) | Project rules for Codebuff/Buffy AI agent |
| [Jules.md](Jules.md) | Rules for Google's Jules AI agent |
| [BRAINSTORM.md](docs/BRAINSTORM.md) | Roadmap, ideas & future vision |

---

## 🔄 CI/CD (GitHub Actions)

[![Build & Test](https://github.com/JONIMONI09/VibeCoreOS/actions/workflows/build.yml/badge.svg)](https://github.com/JONIMONI09/VibeCoreOS/actions/workflows/build.yml)

| Job | Description |
|-----|-------------|
| **build** | ARM64 cross-compile → kernel8.img as artifact |
| **security** | fanalyzer + cppcheck + flawfinder summary |
| **test** | QEMU boot test + ISO build + FAT32 verify |

**Workflow**: `.github/workflows/build.yml` — runs on every push & pull request.

---

## 📄 License

### v1.0.0 — "Photon" (2026-06-15)

- **Core**: Bare-metal ARM64 kernel, 62 KB
- **Graphics**: GPU-allocated framebuffer (1024×768×32), 128-bit STP, double buffering
- **GUI**: Desktop environment with taskbar, windows, icons, clock
- **Screens**: Graphical welcome screen + recovery screen (WFI polling)
- **Animation**: 20-frame boot animation with gradient progress bar
- **Scheduler**: BORE-inspired, 32 tasks, 1 kHz tick
- **Recovery**: 8 subsystems (journal, trash, versions, CRC32, boot recovery, protection, snapshots, auto-recovery)
- **Logging**: klog ring buffer (5 levels), crash log (BSS buffer)
- **Shell**: 17+ commands, interactive, UART-based
- **Build**: Parallel (`make -j$(nproc)`), ISO system, MBR boot code
- **Auth**: Desktop GUI-only popup, 5-minute cache
- **Security**: Stack protector, NULL checks, CRC32 verify, journal atomicity, stack clash protection
- **Analysis**: cppcheck v2.17.1, flawfinder v2.0.20, GCC fanalyzer, full audit
- **QEMU**: Direct kernel boot, ISO test, GDB debug, `-monitor none` on all targets
- **UEFI**: Diagnostic stub (`BOOTAA64.EFI`), `EFI/BOOT/` on SD image, PE/COFF aarch64

---

## 📄 License

Proprietary — VibeCore Labs.
