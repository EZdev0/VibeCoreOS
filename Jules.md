# VibeCore OS — Jules AI Agent Regeln

## 🤖 Für Google Jules (KI Coding Agent)

Diese Datei enthält die spezifischen Regeln und Anweisungen für Google's **Jules** AI Coding Agent,
damit er korrekt am VibeCore OS Projekt arbeiten kann.

---

## 📋 Projekt-Info

| Feld | Wert |
|------|------|
| **Name** | VibeCore OS 1.0.0 "Photon" |
| **Typ** | Bare-Metal ARM64 Betriebssystem |
| **Zielplattform** | Raspberry Pi 3B/4B (BCM2837/BCM2711) |
| **Sprache** | C (GCC) + ARMv8 Assembly |
| **Kernel-Größe** | ~62 KB |
| **Build-System** | GNU Make |
| **Compiler** | `aarch64-linux-gnu-gcc` |
| **Emulator** | QEMU (`raspi3b`, Cortex-A53) |
| **Repo** | GitHub privat: `JONIMONI09/VibeCoreOS` |

---

## 🎯 Jules' Aufgaben & Ziele

1. **Code verstehen**: Lies `ARCHITECTURE.md` und `README.md` zuerst
2. **Bugs finden & fixen**: Nutze `cppcheck`, `flawfinder`, GCC `-fanalyzer`
3. **Features implementieren**: Nur nach Absprache, keine wilden Refactorings
4. **Dokumentation aktuell halten**: README.md, ARCHITECTURE.md, RULES.md
5. **Build sicherstellen**: `make clean && make -j$(nproc)` muss 0 Fehler haben
6. **QEMU testen**: `make run` — OS muss booten, Shell erreichbar sein

---

## 🚫 Absolute Verbote

| Verbot | Grund |
|--------|-------|
| ❌ `sudo` im Terminal | Terminal-Korruption! Nur `auth-helper.sh` (GUI-Popup) |
| ❌ Echter Raspberry Pi Zugriff | Pi bleibt unberührt — nur QEMU |
| ❌ `any`/`void*` Casts ohne Grund | Type-Safety ist kritisch |
| ❌ `-f`/`--force` bei git push | Keine gewaltsamen Pushes |
| ❌ Build-Artefakte committen | `.gitignore` respektieren |
| ❌ Compiler-Warnings ignorieren | `-Wall -Wextra -Werror` ist Pflicht |
| ❌ `make install` auf dem Dev-System | Nur für echte SD-Karte |
| ❌ Ohne Test commiten | `make run` muss funktionieren |

---

## ✅ Vorgehen pro Task

```
1. KONTEXT SAMMELN
   ├── README.md + ARCHITECTURE.md lesen
   ├── Relevante src/-Dateien lesen
   └── include/-Header prüfen

2. ANALYSE
   ├── `make clean && make -j$(nproc)` (Baseline)
   ├── `cppcheck --enable=all src/`
   └── GCC `-fanalyzer` bei Verdacht

3. IMPLEMENTIERUNG
   ├── `str_replace` für kleine Änderungen
   ├── `write_file` nur für neue Dateien
   └── Bestehenden Code-Stil exakt nachahmen

4. VALIDIERUNG
   ├── `make clean && make -j$(nproc)` (muss 0 Fehler)
   ├── `make run` (QEMU Boot-Test)
   └── Code-Review einholen

5. DOKUMENTATION
   ├── README.md Changelog updaten
   ├── RULES.md bei neuen Regeln updaten
   └── Commit mit klarer Message
```

---

## 🏗️ Build-Kommandos (Referenz)

```bash
make -j$(nproc)     # Kompilieren (parallel)
make run            # QEMU starten (Terminal)
make run-gui        # QEMU mit Grafik
make iso            # ISO erstellen
make iso-noroot     # ISO ohne Root
make iso-verify     # ISO prüfen
make iso-test       # ISO in QEMU testen
make clean          # Aufräumen
make cppcheck       # Statische Analyse
make flawfinder     # Security-Audit
make analyze        # cppcheck + flawfinder
make help           # Alle Kommandos
```

---

## 📁 Wichtige Dateien

| Datei | Zweck |
|-------|-------|
| `src/kernel.c` | Haupt-Initialisierung (14 Phasen) |
| `src/boot.S` | ARMv8 Assembly-Einstiegspunkt |
| `src/framebuffer.c` | GPU-Framebuffer (128-bit STP) |
| `src/mailbox.c` | ARM↔GPU Kommunikation |
| `src/interrupt.c` | Exception-Handling |
| `src/scheduler.c` | BORE-Scheduler |
| `src/recovery.c` | 8-Subsystem Recovery |
| `src/gui.c` | Desktop-Umgebung |
| `src/shell.c` | Kommando-Shell |
| `include/peripherals.h` | BCM2837 MMIO-Adressen |
| `include/types.h` | Typ-Definitionen + Makros |
| `linker.ld` | Memory-Layout |
| `Makefile` | Build-System |
| `scripts/auth-helper.sh` | GUI-Auth (NUR Desktop!) |
| `scripts/mk-bootmbr.py` | MBR-Boot-Code Generator |

---

## 🐛 Bekannte Bugs & Baustellen

1. **MMU deaktiviert**: 1GB-Blocks cachen MMIO → UART-Bug. Braucht 2MB L2-Tables.
2. **Kein EMMC/SD-Treiber**: Filesystem ist RAM-Disk, nicht persistent.
3. **Kein USB-Treiber**: Keine Tastatur/Maus — nur UART-Eingabe.
4. **Kein UEFI-Boot**: Kein `BOOTAA64.EFI` → VMs booten nicht direkt.
5. **va_arg Type-Mismatch**: `snprintf_local` liest `u64` bei `%d`/`%u`/`%x`.
6. **Fehlende Prototypen**: `boot_animation_run()`, `crash_log_init()`, `crash_log_write()`.
7. **Precision-Loss**: `boot_anim.c` Gradient `u32→u8` Cast.
8. **Sign-Change**: `crashlog.c` addiert `int` zu `size_t`.

---

## 🔒 Security-Checks (vor jedem Commit)

- [ ] `make clean && make -j$(nproc)` → 0 Fehler, 0 Warnings
- [ ] `make run` → Bootet bis Shell-Prompt
- [ ] Keine sensitiven Daten im Diff
- [ ] `.gitignore` aktuell
- [ ] Keine `sudo`/`su` Aufrufe im Code
- [ ] Stack-Canary intakt (`-fstack-protector-strong`)

---

_Jules-Regeln v1.0 — 15. Juni 2026 — VibeCore Labs_
