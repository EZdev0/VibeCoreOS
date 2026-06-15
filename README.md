# VibeCore OS 1.0.0 — "Photon"

**Bare-Metal ARM64 Betriebssystem für Raspberry Pi 3B/4B** — komplett in C und ARMv8 Assembly von Grund auf gebaut. Kein Linux-Kernel, kein Userspace, keine externen Abhängigkeiten. Nur purer Bare-Metal-Code, direkt auf dem Cortex-A53/A72.

```
╔══════════════════════════════════════════════════╗
║      VibeCore OS 1.0 — "Photon" (aarch64)       ║
║      Lightning fast. Bare metal. Unstoppable.    ║
╚══════════════════════════════════════════════════╝
         Kernel: 62 KB  |  aarch64  |  Cortex-A53/A72
```

---

## 🚀 Schnellstart

```bash
# Voraussetzungen
sudo apt install gcc-aarch64-linux-gnu qemu-system-arm

# Bauen (62 KB Kernel)
make -j$(nproc)

# In QEMU starten (Terminal)
make run

# In QEMU mit Grafikfenster
make run-gui

# ISO/Image erstellen (128 MB, bootfähig)
make iso

# ISO verifizieren
make iso-verify
```

---

## 📋 Build-Kommandos

| Befehl | Beschreibung |
|--------|-------------|
| `make` / `make -j$(nproc)` | Kernel bauen (parallel, alle Kerne) |
| `make run` | QEMU raspi3b — serielle Konsole (KEIN VM-Boot!) |
| `make run-gui` | QEMU mit GTK-Grafikfenster |
| `make debug` | QEMU mit GDB-Server auf Port 1234 |
| `make firmware` | RPi-Firmware downloaden (einmalig nötig) |
| `make iso` | Bootfähiges ISO (128 MB, auto: GUI→full / headless→base) |
| `make iso-noroot` | Basis-ISO OHNE Root-Rechte, OHNE Passwort |
| `make iso-full` | Volles ISO mit Dateien (braucht Desktop-GUI für Auth-Popup) |
| `make iso-verify` | ISO verifizieren: Partitionstabelle + FAT32-Signatur |
| `make iso-test` | ISO in QEMU testen (Kernel direkt + ISO als SD) |
| `make iso-flash` | ISO auf SD-Karte flashen (Desktop-GUI-Auth) |
| `make clean` | Build-Artefakte löschen |
| `make help` | Alle Kommandos anzeigen |
| `make cppcheck` | Statische C-Analyse |
| `make flawfinder` | Security-Audit (CWE/SANS Top 25) |
| `make clang-tidy` | Code-Qualität prüfen |
| `make analyze` | cppcheck + flawfinder |

---

## 🐚 Shell-Kommandos

| Befehl | Alias | Beschreibung |
|--------|-------|-------------|
| `help` | — | Alle Kommandos anzeigen |
| `clear` | — | Terminal löschen |
| `info` | `sysinfo` | System-Informationen |
| `mem` | `memory` | Speicher-Statistiken (Heap, Stack-Canary) |
| `tasks` | `ps` | Scheduler-Taskliste |
| `fs` | `df` | Dateisystem-Info |
| `version` | `ver` | OS-Version + Build-Info |
| `gui` | — | Desktop/Framebuffer-Status |
| `log` | `dmesg` | Kernel-Log-Ringpuffer ausgeben |
| `crash` | `panic` | Crash-Screen testen (manuell) |
| `reboot` | — | System-Neustart |
| `fsck` | `check` | Dateisystem-Check |
| `trash` | — | Papierkorb-Inhalt anzeigen |
| `versions` | `verlist` | Dateiversionen auflisten |
| `recover <name>` | — | Datei aus Papierkorb wiederherstellen |
| `recovery` | — | Grafischen Recovery-Screen öffnen |
| `snapshot` | — | System-Backup erstellen |
| `protect` | — | Geschützte Dateien anzeigen |

---

## 🏗️ Projektstruktur

```
Vibe_Core_Labor/
├── README.md                  ← Diese Datei
├── ARCHITECTURE.md            ← Detaillierte Architektur-Doku
├── TESTING.md                 ← Test-Guide & CI-Setup
├── Makefile                   ← Build-System (parallel, auth, ISO)
├── linker.ld                  ← Linker-Script (Memory-Map)
├── config.txt                 ← RPi Boot-Konfiguration
├── scripts/
│   ├── auth-helper.sh         ← Desktop-GUI Passwort-Popup (zenity/pkexec)
│   └── mk-bootmbr.py          ← MBR-Boot-Code Generator (VM-Kompatibilität)
├── src/                       ← Kernel-Quellcode (22 Dateien)
│   ├── boot.S                 ← ARMv8 Assembly-Einstiegspunkt
│   ├── kernel.c               ← Haupt-Initialisierung
│   ├── framebuffer.c          ← 128-bit STP Display-Treiber
│   ├── mailbox.c              ← ARM↔GPU Kommunikation
│   ├── interrupt.c            ← Exception-Dispatch + Crash-Screen
│   ├── uart.c                 ← PL011 UART-Treiber
│   ├── timer.c                ← System-Timer (1 kHz Tick)
│   ├── allocator.c            ← Speicher-Allokator (graceful OOM)
│   ├── scheduler.c            ← BORE-Scheduler
│   ├── mmu.c                  ← ARMv8 Page-Tables (aktuell deaktiviert)
│   ├── fs.c                   ← FAT32-Dateisystem (RAM-Disk)
│   ├── crc32.c                ← CRC32 Hardware-Hash
│   ├── recovery.c             ← 8-Subsystem Recovery-Engine
│   ├── crashlog.c             ← Crash-Report-Writer (BSS-Buffer)
│   ├── klog.c                 ← Kernel-Logging (dmesg Ringpuffer)
│   ├── gui.c                  ← Desktop-Umgebung
│   ├── gui_welcome.c          ← Grafischer Willkommens-Screen
│   ├── gui_recovery.c         ← Grafischer Recovery-Screen
│   ├── boot_anim.c            ← Boot-Animation (20 Frames)
│   ├── shell.c                ← Interaktive Kommando-Shell
│   ├── setup.c                ← First-Boot Auto-Konfiguration│   └── string.c               ← String/Memory-Utilities
├── include/                   ← Header-Dateien (22 Dateien)
│   ├── types.h                ← Typdefinitionen + Makros
│   ├── kernel.h               ← Globale Kernel-Definitionen
│   ├── peripherals.h          ← BCM2837 MMIO-Adressen
│   ├── framebuffer.h          ← Grafik-API
│   ├── mailbox.h              ← Mailbox-Tag-Definitionen
│   └── ...                    ← (weitere 17 Header)
├── build/                     ← Build-Output & Firmware
│   ├── vibecore.iso           ← Bootfähiges Image (128 MB, MBR+FAT32)
│   ├── bootcode.bin           ← RPi GPU Bootloader (52 KB)
│   ├── start.elf              ← RPi GPU Firmware (2.9 MB)
│   └── fixup.dat              ← GPU Speicher-Konfiguration (7 KB)
└── doc/
    └── ARCHITECTURE.md        ← Detaillierte Architektur-Doku
```

---

## 💿 ISO/Image-System

### Übersicht

Das Build-System erstellt `build/vibecore.iso` — ein **MBR+FAT32 Disk-Image**, kein ISO 9660.
Das ist das gleiche Format wie Ubuntu-RPi-Images und Raspberry Pi OS.

### ISO-Targets

| Target | Beschreibung | Root? | GUI? |
|--------|-------------|-------|------|
| `make iso` | Auto: GUI erkannt → `iso-full`, sonst → `iso-noroot` | Auto | Auto |
| `make iso-full` | Volles Image mit allen Dateien (128 MB) | Ja (losetup) | Ja (zenity) |
| `make iso-noroot` | Basis-Image, nur partitioniert + formatiert | **NEIN** | Nein |

### Image-Inhalt (iso-full)

```
MBR (Boot-Code + Partitionstabelle)
└── Partition 1 (FAT32, bootable, 127 MB)
    ├── kernel8.img      (62 KB)  — Betriebssystem-Kernel
    ├── config.txt        (682 B)  — Boot-Konfiguration
    ├── bootcode.bin      (52 KB)  — GPU First-Stage Bootloader
    ├── start.elf         (2.9 MB) — GPU Firmware
    └── fixup.dat         (7 KB)   — GPU Speicher-Konfiguration
```

### MBR-Boot-Code

Das Image enthält jetzt **440 Byte MBR-Boot-Code** (generiert von `scripts/mk-bootmbr.py`).
Der Code zeigt eine BIOS-Meldung und hält an — das macht das Image für VMs erkennbar.

```
Bytes 0-439:   Boot-Code (x86 real-mode, "VibeCore OS ARM64 — Boot via QEMU: make run")
Bytes 440-445: Disk-Signatur
Bytes 446-509: Partitionstabelle (sfdisk)
Bytes 510-511: Boot-Signatur (0x55 0xAA) ✅
```

### Flashen auf SD-Karte

```bash
# Mit auth-helper (Desktop-GUI Popup)
make iso-flash SDCARD=/dev/mmcblk0

# Manuell
dd if=build/vibecore.iso of=/dev/mmcblk0 bs=4M status=progress
```

---

## 🖥️ VM / Emulator

### WICHTIG: Kein VM-Direktboot!

VibeCore OS ist ein **Bare-Metal-Kernel** für Raspberry Pi. Es bootet NICHT in einer VM wie VirtualBox oder virt-manager, weil:

1. **UEFI-Firmware** (virt-manager, GNOME Boxes) sucht nach `BOOTAA64.EFI` → nicht vorhanden
2. **BIOS** führt MBR-Code aus → zeigt nur Meldung, kein echter Boot
3. Der Raspberry Pi bootet über **GPU-Firmware** → kein BIOS, kein UEFI

### ✅ Richtiger Weg: QEMU Direkt-Boot

```bash
make run        # Terminal-Modus
make run-gui    # Mit Grafikfenster
```

Das funktioniert, weil QEMU mit `-kernel` den Kernel direkt lädt und die RPi-Hardware emuliert — ohne BIOS/UEFI.

### 🔧 Auth-Helper (Kein Terminal-Passwort!)

```bash
scripts/auth-helper.sh <befehl>
```

Der Auth-Helper:
- Zeigt **NUR Desktop-GUI Popups** (zenity oder pkexec)
- **KEIN Terminal-Passwort** — verhindert Terminal-Korruption
- **5-Minuten Cache** — ein Popup pro Build-Session
- Wenn keine GUI → klare Fehlermeldung, kein Blockieren

---

## 🏛️ Architektur

### Boot-Sequenz

```
Power-On → GPU lädt kernel8.img → boot.S (_start)
  → BSS nullen → Stack init → FP/SIMD enable
  → kernel_main():
      0.  UART           (Debug-Serial, 115200 8N1)
      0.5 klog           (Kernel-Logging Ringpuffer)
      1.  Timer          (System-Timer, 1 kHz)
      2.  Framebuffer    (Mailbox-GPU, 1024×768 32-bit)
      3.  Boot-Animation (20 Frames, Gradient-Progressbar)
      4.  Allocator      (Graceful OOM, 64KB Emergency)
      5.  MMU            (aktuell deaktiviert)
      6.  Scheduler      (BORE-inspired, 32 Tasks)
      7.  CRC32          (Self-Test)
      8.  Filesystem     (FAT32 RAM-Disk)
      9.  Recovery       (8 Subsysteme)
      10. Snapshots      (Auto-Backup bei Boot)
      11. GUI/Desktop    (Framebuffer-Desktop)
      12. Crash-Log      (Vorherige Crashes prüfen)
      13. Welcome-Screen (Grafisch, ENTER drücken)
      14. Shell          (Interaktiv, 17+ Kommandos)
```

### Memory-Layout

```
0x00000000 ┌──────────────────────────┐
           │  GPU / Peripherals        │
0x00080000 ├──────────────────────────┤
           │  .text.boot               │  ← Boot-Code
           │  .text / .rodata / .data  │  ← Kernel
           │  .bss (zero-init)         │
           ├──────────────────────────┤
           │  GUARD PAGE (4 KB)        │  ← Stack-Schutz
           ├──────────────────────────┤
           │  STACK (128 KB)           │  ← Wächst nach unten
           ├──────────────────────────┤
           │  HEAP (1 MB)              │  ← kmalloc/kzalloc
           │  EMERGENCY (64 KB)        │  ← Crash-Diagnostik
           ├──────────────────────────┤
           │  Framebuffer (GPU-alloc)  │  ← 0x3C100000
           └──────────────────────────┘
```

---

## 📊 Statistiken

| Metrik | Wert |
|--------|------|
| Kernel-Größe | 62 KB |
| Quellcode-Dateien | 22 (.c + .S) + 22 (.h) = 44 |
| Build-Zeit (parallel) | ~2 Sekunden |
| Boot-Zeit (QEMU) | ~1.6 Sekunden |
| Grafik-Auflösung | 1024×768 (32-bit ARGB) |
| Heap-Größe | 1 MB + 64 KB Emergency |
| Stack-Größe | 128 KB |
| Max Tasks | 32 |
| Idle-CPU | WFI Sleep (~0%) |

---

## 🐛 Bekannte Bugs & Status

### Kürzlich behoben (2026-06-15)

| Bug | Datei | Fix |
|-----|-------|-----|
| `mailbox_call` prüfte falschen Return-Wert | `mailbox.c` | `buffer[1] == MBOX_RESPONSE` statt `result == 0` |
| GPU-Adresse aus falschem Slot gelesen | `framebuffer.c` | `buf[23]` (base) + `buf[24]` (size) statt `buf[22]` |
| `framebuffer_fillrect` Alignment-Fault (Device-Memory) | `framebuffer.c` | `IS_ALIGNED(buf, 8)` Check + 32-bit Fallback |
| ISO-Dateiendung `.img` statt `.iso` | `Makefile` | Umbenannt auf `build/vibecore.iso` |
| MBR ohne Boot-Code → VM "not bootable" | `mk-bootmbr.py` | 440-Byte MBR-Boot-Code generiert |
| IRQ-Endlosschleife bei unbekannten IRQs | `interrupt.c` | Write-1-to-Clear + dmb Barrier |
| `snprintf_local` Buffer-Overflow | `interrupt.c` | `max == 0` Guard vor Write |
| Auth-Helper Terminal-Fallback | `auth-helper.sh` | Nur noch Desktop-GUI |

---

## 🔒 Security

| Feature | Implementierung |
|---------|----------------|
| Stack-Schutz | `-fstack-protector-strong` + `__stack_chk_fail()` |
| NULL-Pointer-Check | `-fno-delete-null-pointer-checks` |
| Frame-Pointer | `-fno-omit-frame-pointer` (Stack-Trace) |
| Strikte Ausrichtung | `-mstrict-align` (ARMv8 Alignment-Trap) |
| CRC32-Integrität | Auto-Verify bei jedem `fs_read_file()` |
| Journal-Atomicität | Commit-Marker verhindern partielle Schreibvorgänge |
| OOM-Resilienz | `kmalloc()` returned NULL, kein Panic |
| Stack-Canary | Prüfung zwischen Stack und Heap |
| Crash-Isolation | System stoppt bei Crash (keine Datenkorruption) |
| WFI-Polling | Wait-For-Interrupt statt Busy-Spin |

---

## ⚡ Performance-Optimierungen

| Optimierung | Beschleunigung | Technik |
|------------|---------------|---------|
| 128-bit STP Clear | ~8× | ARMv8 Store-Pair, 4× unrolled |
| 64-bit Fillrect | ~2× | 2 Pixel pro Schreibzugriff |
| Gradient Row Caching | ~w× | Farbe einmal pro Zeile berechnen |
| WFI Polling | ~99% CPU | Wait-For-Interrupt statt Spin |
| Parallel Build | ~nproc× | `make -j$(nproc)` |

---

## 🔧 Abhängigkeiten

```bash
# Pflicht (Build)
gcc-aarch64-linux-gnu          # ARM64 Cross-Compiler
binutils-aarch64-linux-gnu     # Assembler, Linker, Objcopy

# Empfohlen (Test)
qemu-system-arm                # QEMU Emulation

# Optional (Analyse)
cppcheck                       # Statische C-Analyse
flawfinder                     # Security-Scan

# Optional (ISO/Image)
dosfstools                     # mkfs.fat
python3                        # MBR-Generator

# Optional (Desktop Auth)
zenity                         # GUI-Passwort-Dialog
```

---

## 📚 Dokumentation

| Dokument | Inhalt |
|----------|--------|
| [README.md](README.md) | Diese Datei — Übersicht & Schnellstart |
| [ARCHITECTURE.md](ARCHITECTURE.md) | Vollständige Architektur: Boot, Speicher, Subsysteme |
| [TESTING.md](TESTING.md) | Test-Pyramide: Kompilierung, Analyse, QEMU, Hardware |
| [doc/ARCHITECTURE.md](doc/ARCHITECTURE.md) | Detaillierte Referenz |

---

## 📝 Changelog

### v1.0.0 — "Photon" (2026-06-15)

- **Core**: Bare-Metal ARM64 Kernel, 62 KB
- **Graphics**: GPU-allocierter Framebuffer (1024×768×32), 128-bit STP, Double-Buffering
- **GUI**: Desktop-Umgebung mit Taskbar, Fenster, Icons, Clock
- **Screens**: Grafischer Welcome-Screen + Recovery-Screen (WFI-Polling)
- **Animation**: 20-Frame Boot-Animation mit Gradient-Progressbar
- **Scheduler**: BORE-inspired, 32 Tasks, 1 kHz Tick
- **Recovery**: 8 Subsysteme (Journal, Trash, Versions, CRC32, Boot-Recovery, Protection, Snapshots, Auto-Recovery)
- **Logging**: klog Ringpuffer (5 Level), Crash-Log (BSS-Buffer)
- **Shell**: 17+ Kommandos, interaktiv, UART-basiert
- **Build**: Parallel (`make -j$(nproc)`), ISO-System, MBR-Boot-Code
- **Auth**: Desktop-GUI-Only Popup, 5-Minuten-Cache
- **Security**: Stack-Protector, NULL-Checks, CRC32-Verify, Journal-Atomicity
- **QEMU**: Direkter Kernel-Boot, ISO-Test, GDB-Debug

---

## 📄 Lizenz

Proprietär — VibeCore Labs.
