# 🧠 VibeCore OS — Brainstorming & Roadmap

> **"Photon" v1.0.0** | Bare-Metal ARM64 OS | 62 KB Kernel | Raspberry Pi 3B/4B

---

## 📊 Current State — What We Have

```
✅ = Done   🟡 = Partial   ❌ = Missing
```

### Kernel Core
| Feature | Status | Details |
|---------|--------|---------|
| ARMv8 Boot | ✅ | boot.S + BSS init + FP/SIMD enable |
| UART (PL011) | ✅ | 115200 8N1, send + receive |
| Exception Handling | ✅ | VBAR_EL1, Crash-Screen, ESR/ELR/FAR |
| Timer | ✅ | System Timer, 1 kHz Tick, sleep_ms |
| Framebuffer | ✅ | Mailbox-GPU, 1024×768×32, 128-bit STP |
| Allocator | ✅ | kmalloc/kzalloc, graceful OOM, emergency |
| MMU | 🟡 | Code exists, disabled (MMIO cache bug) |
| Scheduler | ✅ | BORE-inspired, 32 Tasks, 1 kHz |

### Storage & Recovery
| Feature | Status | Details |
|---------|--------|---------|
| FAT32 FS | 🟡 | RAM-Disk only, no EMMC/SD |
| CRC32 | ✅ | Hardware-accelerated, self-test |
| Journal | ✅ | 8 KB ring, 512 entries, atomic |
| Trash Bin | ✅ | .meta sidecar, original path |
| Versioning | ✅ | CoW before every write |
| Snapshots | ✅ | Auto-boot + manual |
| System Protection | ✅ | Immutable file list |
| Boot Recovery | ✅ | Journal scan, rollback |

### GUI & UX
| Feature | Status | Details |
|---------|--------|---------|
| Desktop | ✅ | Taskbar, icons, windows, clock |
| Welcome Screen | ✅ | Gradient, logo, ENTER |
| Recovery Screen | ✅ | Red/danger theme, keyboard |
| Boot Animation | ✅ | 20 frames, gradient progress |
| Shell | ✅ | 18+ commands, UART |

### Build & CI/CD
| Feature | Status | Details |
|---------|--------|---------|
| Makefile | ✅ | Parallel, Auth, ISO |
| ISO-System | ✅ | MBR+FAT32, 128 MB, Boot-Code |
| Cross-Compiler | ✅ | aarch64-linux-gnu-gcc |
| GitHub Actions | ✅ | Build + cppcheck + QEMU + ISO |
| Security Tools | ✅ | fanalyzer, flawfinder, cppcheck, harden |
| Auth-Helper | ✅ | GUI-only, 5-minute cache |

### Docs
| Feature | Status | Details |
|---------|--------|---------|
| README.md (EN) | ✅ | 395 lines, 1842 words |
| ARCHITECTURE.md (EN) | ✅ | Complete architecture |
| TESTING.md | ✅ | Test pyramid |
| RULES.md (EN) | ✅ | Codebuff/Buffy rules |
| Jules.md (EN) | ✅ | Google Jules rules |

---

## 🎯 ROADMAP

### 🔴 v1.1 — "Electron" (Stability & Bugfixes) — ~2 weeks (optimistic)

#### Priority 1: Fix Bugs
```
[ ] va_arg Type-Mismatch in snprintf_local (interrupt.c)
    → Type correction: va_arg(args, uint64_t) instead of va_arg(args, uint32_t)
    → Impact: Crash risk with varargs on ARM64

[ ] Missing prototypes:
    → boot_animation_run() declaration missing
    → crash_log_init(), crash_log_write() declaration missing

[ ] Precision loss: u32→u8 in boot_anim.c Gradient
    → Explicit conversion with warning

[ ] Sign-change: int + size_t in crashlog.c
    → Explicit cast
```

#### Priority 2: Code Quality
```
[ ] Alle compiler warnings fixen (Ziel: 0 warnings mit -Wall -Wextra -Werror)
[ ] fanalyzer-warnings durchgehen und filtern
[ ] flawfinder findings reviewen (FPs markieren)
[ ] cppcheck inconclusive warnings durchgehen
```

#### Priority 3: Testing
```
[ ] Unit-Test Framework (minimal, in-kernel)
    → ASSERT Macro
    → Test-Runner in Shell (make test-target)
[ ] Integration tests for recovery subsystems
    → Journal Write → Read → Verify
    → Trash Move → Restore → Verify Path
    → CRC32 Corruption → Auto-FSCK
[ ] QEMU Regression Suite
    → Boot + Shell + 5 Kommandos + Reboot
    → Crash-Test (absichtlich) + Bluescreen-Verify
[ ] CI/CD: Regression-Job in GitHub Actions
    → make check + make harden-test + make run (smoke)
```

---

### 🟡 v1.2 — "Neutron" (I/O & Persistence) — ~4-8 weeks (optimistic)

#### Tests for v1.2
```
[ ] CI/CD erweitern: ISO + QEMU Boot mit SD-Image
[ ] EMMC Read/Write Loopback Test (write → read → compare)
[ ] FAT32 persistent: Datei erstellen → Reboot → Datei existiert
```

#### EMMC/SD Card Driver
```
[ ] BCM2837 EMMC2 Controller Treiber
    → Memory-mapped I/O (0x7E300000)
    → Command Queue (CQ)
    → DMA Transfers (SDMA/ADMA2)
    → Error Recovery (CRC, Timeout)
[ ] FAT32 persistent mount von SD
    → echte Partition lesen/schreiben
    → Boot-Partition mounten
    → Recovery-Daten persistent
```

#### USB Stack (DWC2)
```
[ ] DWC2 Host Controller Driver
    → Root Hub Enumeration
    → Device Detection
[ ] HID Keyboard Driver
    → Boot Protocol
    → Scancode → ASCII
    → Modifier Keys (Shift, Ctrl, Alt)
[ ] Optional: USB Mouse
    → HID Report Parser
    → Cursor-Bewegung
```

#### SD/USB Integration
```
[ ] Boot von ISO auf echter SD-Karte testen
[ ] Keyboard-Input in Shell (statt nur UART)
[ ] Desktop mit Maus-Navigation
```

---

### 🟢 v1.3 — "Proton" (Network) — ~6-10 weeks (optimistic)

#### Tests for v1.3
```
[ ] CI/CD: ping localhost via QEMU user-mode network
[ ] DHCP: lease acquirieren + erneuern
[ ] ICMP: 100 pings ohne Paketverlust
```

#### Ethernet (LAN9514)
```
[ ] USB Ethernet Driver (LAN9514 on RPi 3B)
    → smsc95xx Treiber
    → MAC-Adresse lesen
    → RX/TX Ring Buffer
[ ] Minimal TCP/IP Stack
    → Ethernet Frame Parser
    → ARP (Address Resolution)
    → IPv4 (ohne Fragmentation)
    → ICMP (Ping!)
    → UDP (einfach)
    → TCP (minimal: SYN/SYN-ACK/ACK, kein Window-Scaling)
[ ] DHCP Client
    → Discover → Offer → Request → ACK
```

#### Shell-Netzwerk-Kommandos
```
[ ] ping <ip>        → ICMP Echo
[ ] ipconfig         → DHCP lease info
[ ] netstat          → Verbindungen anzeigen
```

#### WiFi? (optional)
```
[ ] Cypress CYW43438 (RPi 3B WiFi)
    → SDIO Interface
    → Firmware Load
    → WPA2 Supplicant (minimal)
```

---

### 🔵 v1.4 — "Gluon" (Userspace & Processes) — ~8-12 weeks (optimistic)

#### Tests for v1.4
```
[ ] CI/CD: Build userspace binary + execute in QEMU
[ ] ELF Loader: 100 Programme laden/entladen ohne Leak
[ ] SVC: syscall Fuzzing (random args → kein Kernel-Crash)
```

#### ELF Loader
```
[ ] ELF64 Parser (Little-Endian)
    → Program Headers lesen
    → Segmente in Speicher laden
    → Relocations (R_AARCH64_RELATIVE)
[ ] Userspace Memory Allocator
    → Separate Heap per Process
```

#### System Calls (SVC)
```
[ ] SVC Interface (Supervisor Call)
    → syscall_table[NR]
    → SVC #0 → dispatch → return
[ ] Syscalls:
    → SYS_WRITE (UART)
    → SYS_READ  (UART)
    → SYS_EXIT
    → SYS_SBRK (Memory)
    → SYS_OPEN/READ/WRITE/CLOSE (FS)
    → SYS_SLEEP
```

#### Process Isolation (MMU)
```
[ ] MMU Fix (L2 Tables for MMIO)
    → Identity Map Kernel
    → 2MB granules for device memory
[ ] Per-Process Page Tables
    → ASID (Address Space ID)
    → TTBR0_EL1 (User) / TTBR1_EL1 (Kernel)
    → Context Switch: TTBR0 swap
[ ] EL0 → EL1 Transition
    → SVC, IRQ, Data Abort Handling
```

#### Userspace Programme
```
[ ] /bin/hello     → "Hello from userspace!"
[ ] /bin/sh        → Mini-Shell
[ ] /bin/tests     → Userspace Unit Tests
```

---

### 🟣 v2.0 — "Boson" (App Platform) — ~12-16 weeks (optimistic)

#### Tests for v2.0
```
[ ] CI/CD: Full userspace test suite
[ ] Lua: Scripting-Tests (math, string, io)
[ ] GUI: Widget Regression Screenshots (VNC capture)
```

#### libc Port
```
[ ] newlib oder picolibc Port
    → Bare-Metal Syscall-Backend
    → malloc/free/fopen/fprintf
[ ] Oder: Eigene Mini-libc
    → string.h, stdio.h, stdlib.h
```

#### Interpreter
```
[ ] Lua 5.4 Port
    → ANSI C, not OS dependent
    → Only write/read syscalls needed
    → Scripting-Shell!
[ ] Oder: MicroPython
    → ARM64 Bare-Metal Port existiert
    → Hardware-Zugriff (GPIO, I2C)
```

#### GUI Toolkit
```
[ ] Eigene Widget-Bibliothek
    → Button, Label, TextInput, Checkbox
    → Layout-Manager (VBox, HBox)
    → Event-System (on_click, on_key)
[ ] Dateimanager
    → Verzeichnisbaum
    → Copy/Paste/Delete
```

---

## 💡 Idea Parking Lot

### Security
```
[ ] ASLR (Address Space Layout Randomization)
    → Kernel: einmalig bei Boot
    → Userspace: per Process

[ ] W^X (Write XOR Execute)
    → MMU: Pages NUR writable ODER executable
    → Verhindert Shellcode-Injection

[ ] SMEP/SMAP Emulation
    → Kernel must not execute userspace
    → PAN (Privileged Access Never) auf ARMv8.1

[ ] Signed Binaries
    → Ed25519 Signatur
    → Kernel verweigert unsignierte ELFs

[ ] Secure Boot Chain
    → GPU → kernel8.img → Userspace
    → Each stage verifies the next
```

### Performance
```
[ ] NEON SIMD Optimierungen
    → memcpy, memset per NEON
    → CRC32 per NEON (noch schneller)
    → Grafik-Filter (Blur, Scale)

[ ] Multicore (SMP)
    → CPU1-3 aktivieren (PSCI)
    → Per-Core Scheduler Queues
    → Spinlocks / Mutexes

[ ] L1/L2 Cache Optimierung
    → Cache-Line Alignment (64 Byte)
    → Prefetch Hints (PLD)
    → Cache-Coloring gegen Thrashing

[ ] JIT Compiler (long term)
    → Tiny JIT for shell scripts
    → BPF engine for network filters
```

### Storage
```
[ ] ext2 Dateisystem
    → Better than FAT32 for recovery
    → Symlinks, Permissions, Journal

[ ] NVMe Driver (RPi 5)
    → PCIe Root Complex
    → NVMe Command Set
    → 4K Random I/O

[ ] RAM-Disk Erweiterung
    → tmpfs-artig
    → /tmp, /var/run
```

### GUI & Multimedia
```
[ ] TrueType Font Renderer
    → FreeType Port
    → Anti-Aliasing
    → Unicode (UTF-8)

[ ] Bildformate
    → BMP Decoder (einfach)
    → PNG Decoder (zlib + deflate)
    → JPEG Decoder (optional)

[ ] Audio (PWM)
    → BCM2837 PWM Audio
    → WAV Player (PCM)
    → System Sounds

[ ] Hardware Video Decoder
    → H.264 via VideoCore GPU
    → Kiosk-Mode
```

### Connectivity
```
[ ] Bluetooth
    → BCM2837 UART Bluetooth (RPi 3B)
    → HID Profile (Tastatur, Maus)
    → BLE GATT Server

[ ] MQTT Client
    → IoT Gateway
    → Sensor-Daten sammeln

[ ] HTTP Server
    → TCP/80
    → Statische Dateien
    → CGI-like endpoints

[ ] SSH Server
    → dropbear Port
    → Remote Shell!
```

### Developer Experience
```
[ ] In-Kernel Debugger
    → Breakpoints (BRK)
    → Single-Step
    → Memory Dump
    → Register View

[ ] Crash Dump Analyzer
    → Symbol-Resolver (kernel.map)
    → Stack Unwinding
    → Offline Crash-Report

[ ] Hot-Reload
    → Kernel-Module dynamisch laden
    → Ohne Reboot testen

[ ] QEMU-basierte CI/CD Erweiterungen
    → Fuzzing (AFL)
    → Coverage (gcov/lcov)
    → Benchmark Regression
```

### Raspberry Pi 5 Support
```
[ ] BCM2712 (Cortex-A76, ARMv8.2)
    → 64-bit Memory Map
    → GIC-400 Interrupt Controller
    → RP1 I/O Controller
    → PCIe Root Complex
```

---

## 🏗️ Architecture Debate

### Pro/Contra: MMU for everything
| Pro | Contra |
|-----|--------|
| Protection against wild pointers | Overhead per context switch |
| Userspace isolation | Complexity (TLB management) |
| ASLR possible | 2MB granules coarse |

**Decision**: Enable MMU when userspace arrives. Kernel in own address space.

### Pro/Contra: Microkernel vs Monolith
| Monolith (current) | Microkernel |
|--------------------|-------------|
| Fast (direct calls) | Safer (IPC isolation) |
| Easier to build | Complex IPC overhead |
| Fewer context switches | Driver crash doesn't kill kernel |

**Decision**: Keep monolith. Recovery system compensates stability. Microkernel only at multi-user/multi-tenant.

### Pro/Contra: Rust instead of C?
| C (current) | Rust |
|-------------|------|
| Complete control | Memory safety by default |
| ARM64 toolchain stable | ARM64 bare-metal still young |
| All examples in C | Fewer embedded resources |

**Decision**: C for kernel core. Consider Rust for userspace apps (when userspace exists).

---

## 🔬 RESEARCH AREAS

### Currently to research
```
[ ] ARM GIC (Generic Interrupt Controller)
    → GIC-400 on RPi 4/5
    → Legacy IRQ on RPi 3B (simpler)
    → Interrupt priorities

[ ] PCIe Enumeration (RPi 5)
    → Root Complex BAR-Scan
    → Device Tree Parsing
    → NVMe, USB3, Ethernet via PCIe

[ ] ARM TrustZone (EL3)
    → Secure Monitor
    → TEE (Trusted Execution Environment)
    → Key Storage, DRM, Attestation

[ ] ARMv8 Crypto Extensions
    → AES, SHA-1, SHA-256 in hardware
    → Speed: 10-100x faster
    → For signature verification

[ ] ACPI / Device Tree
    → Hardware detection instead of hardcoding
    → Multi-platform (RPi 3/4/5, QEMU virt)
```

---

## 📈 Metrics & Goals

| Metric | Current (v1.0) | Target (v1.1) | Target (v1.2) | Target (v2.0) |
|--------|------------|-------------|-------------|-------------|
| Kernel size | 62 KB | < 70 KB | < 100 KB | < 200 KB |
| Boot time (QEMU) | 1.6s | < 1.5s | < 1.8s | < 2.5s |
| Shell commands | 18 | 20 | 25 | 30+ |
| Security flags | 6 | 8 | 10 | 12 |
| CI/CD jobs | 3 | 4 | 5 | 6 |
| Documentation (lines) | ~2000 | ~2500 | ~3000 | ~4000 |
| compiler warnings (`-Wall -Wextra`) | 0 ✅ | 0 | 0 | 0 |
| fanalyzer issues | 0 ✅ | 0 | 0 | 0 |
| flawfinder hits | 0 ✅ | 0 | 0 | 0 |
| cppcheck issues | 1 🟡 | 0 | 0 | 0 |

---

## 🗳️ Priority Matrix

```
                  LOW IMPACT             HIGH IMPACT
                  │                       │
EASY ─────────────┼───────────────────────┼──────────
                  │ Bug fixes             │ Shell commands
                  │ Code cleanup          │ make check
                  │ Doc updates           │ CI/CD tests
                  │                       │
                  ────────────────────────┼──────────
                  │                       │
HARD ─────────────┼───────────────────────┼──────────
                  │ WiFi driver           │ EMMC driver
                  │ Audio PWM             │ USB stack
                  │ TrueType fonts        │ MMU fix
                  │                       │ Userspace
```

**Recommendation**: First EASY+HIGH IMPACT, then HARD+HIGH IMPACT.

---

## 🎓 Learning Resources

### Books
- **ARM System Developer's Guide** (Sloss, Symes, Wright)
- **ARM64 Assembly Language** (Smith)
- **Operating Systems: Three Easy Pieces** (Arpaci-Dusseau)
- **Linux Device Drivers** (Corbet, Rubini, Kroah-Hartman)

### Specifications
- **ARMv8-A Architecture Reference Manual**
- **BCM2837 ARM Peripherals Manual**
- **DWC2 USB 2.0 Host Controller Databook**
- **EMMC2 Controller (SDHCI Specification)**

### Code References
- **xv6** (MIT Teaching OS, RISC-V)
- **raspberry-pi-os** (s-matyukevich, Tutorial)
- **Circle** (Rene Stange, C++ Bare-Metal RPi)
- **Ultibo** (Pascal Bare-Metal RPi)

---

## 📝 Notes / Ideas (unfiltered)

```
- "VibeScript" — custom scripting language for the shell?
  → Syntax: help { cmd } | pipe > file
  → Interpreter: 200 Zeilen C

- Shell-History (Pfeiltasten)
  → UART Escape-Sequenzen parsen (ESC [ A/B/C/D)
  → Ring buffer for last 20 commands

- Tab-Completion
  → ONLY for shell commands (no FS)
  → Prefix matching, first match

- ANSI-Farben in Shell
  → \033[31m ROT \033[0m
  → Colorize prompt, errors red, info blue

- Splash-Screen statt Boot-Animation
  → 1024×768 BMP laden (von SD)
  → Hardware Compositing (GPU Layer)

- Watchdog Timer
  → HW Watchdog (PM_WDOG)
  → Kernel-Panic → 10s → Auto-Reboot

- Power Management
  → CPU-Frequenz-Skalierung (ARM Clock Manager)
  → DVFS (Dynamic Voltage & Frequency Scaling)
  → Temperatur-Sensor lesen

- RTC (Real-Time Clock)
  → DS3231 via I2C
  → Oder: NTP Client (wenn Netzwerk existiert)

- Packet Radio / LoRa
  → SX1278 via SPI
  → Mesh-Netzwerk ohne WiFi

- CAN Bus
  → MCP2515 via SPI
  → Automotive/Industrie

- Retro-Gaming
  → Framebuffer = Canvas
  → Eigene 2D Engine
  → Emulatoren? (NES, GameBoy → zu komplex)
```

---

## 🚀 Next Actions (this session)

1. **Fix all 4 bugs** (va_arg, prototypes, precision, sign-change)
2. **Analyze fanalyzer/flawfinder/cppcheck warnings**
3. **Build unit test framework**
4. **Start EMMC research**

---

_Last updated: June 15, 2026 — VibeCore Labs_
