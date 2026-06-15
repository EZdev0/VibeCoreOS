# 🧠 VibeCore OS — Brainstorming & Roadmap

> **"Photon" v1.0.0** | Bare-Metal ARM64 OS | 62 KB Kernel | Raspberry Pi 3B/4B

---

## 📊 IST-Zustand — Was wir haben

```
✅ = Fertig   🟡 = Teilweise   ❌ = Fehlt
```

### Kernel-Core
| Feature | Status | Details |
|---------|--------|---------|
| ARMv8 Boot | ✅ | boot.S + BSS init + FP/SIMD enable |
| UART (PL011) | ✅ | 115200 8N1, send + receive |
| Exception Handling | ✅ | VBAR_EL1, Crash-Screen, ESR/ELR/FAR |
| Timer | ✅ | System Timer, 1 kHz Tick, sleep_ms |
| Framebuffer | ✅ | Mailbox-GPU, 1024×768×32, 128-bit STP |
| Allocator | ✅ | kmalloc/kzalloc, graceful OOM, emergency |
| MMU | 🟡 | Code vorhanden, deaktiviert (MMIO-Cache-Bug) |
| Scheduler | ✅ | BORE-inspired, 32 Tasks, 1 kHz |

### Storage & Recovery
| Feature | Status | Details |
|---------|--------|---------|
| FAT32 FS | 🟡 | RAM-Disk only, kein EMMC/SD |
| CRC32 | ✅ | Hardware-beschleunigt, Self-Test |
| Journal | ✅ | 8 KB Ring, 512 Einträge, atomar |
| Trash Bin | ✅ | .meta Sidecar, Original-Pfad |
| Versioning | ✅ | CoW vor jedem Write |
| Snapshots | ✅ | Auto-Boot + Manuell |
| System Protection | ✅ | Immutable File List |
| Boot Recovery | ✅ | Journal-Scan, Rollback |

### GUI & UX
| Feature | Status | Details |
|---------|--------|---------|
| Desktop | ✅ | Taskbar, Icons, Fenster, Clock |
| Welcome Screen | ✅ | Gradient, Logo, ENTER |
| Recovery Screen | ✅ | Rot/Gefahr-Theme, Tastatur |
| Boot Animation | ✅ | 20 Frames, Gradient-Progress |
| Shell | ✅ | 18+ Kommandos, UART |

### Build & CI/CD
| Feature | Status | Details |
|---------|--------|---------|
| Makefile | ✅ | Parallel, Auth, ISO |
| ISO-System | ✅ | MBR+FAT32, 128 MB, Boot-Code |
| Cross-Compiler | ✅ | aarch64-linux-gnu-gcc |
| GitHub Actions | ✅ | Build + Cppcheck + QEMU + ISO |
| Security Tools | ✅ | fanalyzer, flawfinder, cppcheck, harden |
| Auth-Helper | ✅ | GUI-Only, 5-Minuten-Cache |

### Docs
| Feature | Status | Details |
|---------|--------|---------|
| README.md (DE) | ✅ | 395 Zeilen, 1842 Wörter |
| ARCHITECTURE.md (EN) | ✅ | Komplette Architektur |
| TESTING.md | ✅ | Test-Pyramide |
| RULES.md | ✅ | Codebuff/Buffy Regeln |
| Jules.md | ✅ | Google Jules Regeln |

---

## 🎯 ROADMAP

### 🔴 v1.1 — "Electron" (Stabilität & Bugfixes) — ~2 Wochen (optimistisch)

#### Priorität 1: Bugs beheben
```
[ ] va_arg Type-Mismatch in snprintf_local (interrupt.c)
    → Typ-Korrektur: va_arg(args, uint64_t) statt va_arg(args, uint32_t)
    → Impact: Crash-Risiko bei varargs auf ARM64

[ ] Missing prototypes:
    → boot_animation_run() Deklaration fehlt
    → crash_log_init(), crash_log_write() Deklaration fehlt

[ ] Precision loss: u32→u8 in boot_anim.c Gradient
    → Explizite Konvertierung mit Warnung

[ ] Sign-change: int + size_t in crashlog.c
    → Expliziter Cast
```

#### Priorität 2: Code-Qualität
```
[ ] Alle compiler warnings fixen (Ziel: 0 warnings mit -Wall -Wextra -Werror)
[ ] fanalyzer-warnings durchgehen und filtern
[ ] flawfinder findings reviewen (FPs markieren)
[ ] cppcheck inconclusive warnings durchgehen
```

#### Priorität 3: Testing
```
[ ] Unit-Test Framework (minimal, in-kernel)
    → ASSERT Macro
    → Test-Runner in Shell (make test-target)
[ ] Integration Tests für Recovery-Subsysteme
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

### 🟡 v1.2 — "Neutron" (I/O & Persistenz) — ~4-8 Wochen (optimistisch)

#### Tests für v1.2
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

### 🟢 v1.3 — "Proton" (Netzwerk) — ~6-10 Wochen (optimistisch)

#### Tests für v1.3
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

### 🔵 v1.4 — "Gluon" (Userspace & Prozesse) — ~8-12 Wochen (optimistisch)

#### Tests für v1.4
```
[ ] CI/CD: Userspace Binary bauen + in QEMU ausführen
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
[ ] MMU Fix (L2 Tables für MMIO)
    → Identity Map Kernel
    → 2MB Granules für Device Memory
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

### 🟣 v2.0 — "Boson" (App-Plattform) — ~12-16 Wochen (optimistisch)

#### Tests für v2.0
```
[ ] CI/CD: Vollständiger Userspace-Test-Suite
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
    → ANSI C, kein OS-Abhängig
    → Nur write/read syscalls nötig
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

## 💡 IDEA PARKING LOT

### Security
```
[ ] ASLR (Address Space Layout Randomization)
    → Kernel: einmalig bei Boot
    → Userspace: per Process

[ ] W^X (Write XOR Execute)
    → MMU: Pages NUR writable ODER executable
    → Verhindert Shellcode-Injection

[ ] SMEP/SMAP Emulation
    → Kernel darf nicht Userspace ausführen
    → PAN (Privileged Access Never) auf ARMv8.1

[ ] Signed Binaries
    → Ed25519 Signatur
    → Kernel verweigert unsignierte ELFs

[ ] Secure Boot Chain
    → GPU → kernel8.img → Userspace
    → Jeder Stage verifiziert den nächsten
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

[ ] JIT Compiler (längerfristig)
    → Tiny JIT für Shell-Scripts
    → BPF Engine für Network Filter
```

### Storage
```
[ ] ext2 Dateisystem
    → Besser als FAT32 für Recovery
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

### Konnektivität
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
    → CGI-ähnliche Endpoints

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

## 🏗️ ARCHITEKTUR-DEBATTE

### Pro/Contra: MMU für alles
| Pro | Contra |
|-----|--------|
| Schutz vor Wild-Pointern | Overhead pro Context Switch |
| Userspace-Isolation | Komplexität (TLB Management) |
| ASLR möglich | 2MB Granules grob |

**Entscheidung**: MMU aktivieren, sobald Userspace kommt. Kernel in eigenen Adressraum.

### Pro/Contra: Microkernel vs Monolith
| Monolith (aktuell) | Microkernel |
|--------------------|-------------|
| Schnell (direct calls) | Sicherer (IPC-Isolation) |
| Einfacher zu bauen | Komplexer IPC-Overhead |
| Weniger Context-Switches | Treiber-Crash killt nicht Kernel |

**Entscheidung**: Monolith beibehalten. Recovery-System kompensiert Stabilität. Microkernel erst bei Multi-User/Multi-Tenant.

### Pro/Contra: Rust statt C?
| C (aktuell) | Rust |
|-------------|------|
| Komplette Kontrolle | Memory-Safety by default |
| ARM64 Toolchain stabil | ARM64 Bare-Metal noch jung |
| Alle Beispiele in C | Weniger Embedded-Resources |

**Entscheidung**: C für Kernel-Core. Rust für Userspace-Apps erwägen (wenn Userspace existiert).

---

## 🔬 RESEARCH AREAS

### Aktuell zu erforschen
```
[ ] ARM GIC (Generic Interrupt Controller)
    → GIC-400 auf RPi 4/5
    → Legacy IRQ auf RPi 3B (einfacher)
    → Interrupt-Prioritäten

[ ] PCIe Enumeration (RPi 5)
    → Root Complex BAR-Scan
    → Device Tree Parsing
    → NVMe, USB3, Ethernet via PCIe

[ ] ARM TrustZone (EL3)
    → Secure Monitor
    → TEE (Trusted Execution Environment)
    → Key Storage, DRM, Attestation

[ ] ARMv8 Crypto Extensions
    → AES, SHA-1, SHA-256 in Hardware
    → Geschwindigkeit: 10-100x schneller
    → Für Signatur-Verifikation

[ ] ACPI / Device Tree
    → Hardware-Erkennung statt Hardcoding
    → Multi-Platform (RPi 3/4/5, QEMU virt)
```

---

## 📈 METRIKEN & ZIELE

| Metrik | Ist (v1.0) | Ziel (v1.1) | Ziel (v1.2) | Ziel (v2.0) |
|--------|------------|-------------|-------------|-------------|
| Kernel-Größe | 62 KB | < 70 KB | < 100 KB | < 200 KB |
| Boot-Zeit (QEMU) | 1.6s | < 1.5s | < 1.8s | < 2.5s |
| Shell-Kommandos | 18 | 20 | 25 | 30+ |
| Security-Flags | 6 | 8 | 10 | 12 |
| CI/CD Jobs | 3 | 4 | 5 | 6 |
| Dokumentation (Zeilen) | ~2000 | ~2500 | ~3000 | ~4000 |
| compiler warnings (`-Wall -Wextra`) | 0 ✅ | 0 | 0 | 0 |
| fanalyzer issues | 0 ✅ | 0 | 0 | 0 |
| flawfinder hits | 0 ✅ | 0 | 0 | 0 |
| cppcheck issues | 1 🟡 | 0 | 0 | 0 |

---

## 🗳️ PRIORITÄTS-MATRIX

```
                  NIEDRIG IMPACT          HOHER IMPACT
                  │                       │
EINFACH ──────────┼───────────────────────┼──────────
                  │ Bug-Fixes             │ Shell-Commands
                  │ Code-Cleanup          │ make check
                  │ Doku-Updates          │ CI/CD Tests
                  │                       │
                  ────────────────────────┼──────────
                  │                       │
SCHWER ───────────┼───────────────────────┼──────────
                  │ WiFi Driver           │ EMMC Driver
                  │ Audio PWM             │ USB Stack
                  │ TrueType Fonts        │ MMU Fix
                  │                       │ Userspace
```

**Empfehlung**: Erst EINFACH+HOHER IMPACT, dann SCHWER+HOHER IMPACT.

---

## 🎓 LERN-RESSOURCEN

### Bücher
- **ARM System Developer's Guide** (Sloss, Symes, Wright)
- **ARM64 Assembly Language** (Smith)
- **Operating Systems: Three Easy Pieces** (Arpaci-Dusseau)
- **Linux Device Drivers** (Corbet, Rubini, Kroah-Hartman)

### Spezifikationen
- **ARMv8-A Architecture Reference Manual**
- **BCM2837 ARM Peripherals Manual**
- **DWC2 USB 2.0 Host Controller Databook**
- **EMMC2 Controller (SDHCI Specification)**

### Code-Referenzen
- **xv6** (MIT Teaching OS, RISC-V)
- **raspberry-pi-os** (s-matyukevich, Tutorial)
- **Circle** (Rene Stange, C++ Bare-Metal RPi)
- **Ultibo** (Pascal Bare-Metal RPi)

---

## 📝 NOTIZEN / IDEEN (ungefiltert)

```
- "VibeScript" — eigene Scripting-Sprache für die Shell?
  → Syntax: help { cmd } | pipe > file
  → Interpreter: 200 Zeilen C

- Shell-History (Pfeiltasten)
  → UART Escape-Sequenzen parsen (ESC [ A/B/C/D)
  → Ring-Buffer für letzte 20 Kommandos

- Tab-Completion
  → NUR für Shell-Kommandos (kein FS)
  → Prefix-Matching, erste Übereinstimmung

- ANSI-Farben in Shell
  → \033[31m ROT \033[0m
  → Prompt färben, Errors rot, Info blau

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

## 🚀 NEXT ACTIONS (diese Session)

1. **Alle 4 Bugs fixen** (va_arg, prototypes, precision, sign-change)
2. **fanalyzer/flawfinder/cppcheck warnings analysieren**
3. **Unit-Test Framework bauen**
4. **EMMC Research starten**

---

_Letzte Aktualisierung: 15. Juni 2026 — VibeCore Labs_
