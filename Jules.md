# VibeCore OS — Jules AI Agent Regeln (Google)

## 🤖 Für Google Jules (KI Coding Agent)

Diese Datei enthält die spezifischen Regeln und Anweisungen für **Google's Jules AI Coding Agent**,
damit er korrekt, sicher und effizient am VibeCore OS Projekt arbeiten kann.

---

## 📋 Projekt-Steckbrief

| Feld | Wert |
|------|------|
| **Name** | VibeCore OS 1.0.0 "Photon" |
| **Typ** | Bare-Metal ARM64 Betriebssystem |
| **Zielplattform** | Raspberry Pi 3B (BCM2837) / 4B (BCM2711) |
| **Sprache** | C (GCC) + ARMv8 Assembly |
| **Kernel-Größe** | ~62 KB |
| **Build-System** | GNU Make (parallel, `make -j$(nproc)`) |
| **Compiler** | `aarch64-linux-gnu-gcc` |
| **Emulator** | QEMU `raspi3b`, Cortex-A53 |
| **CI/CD** | GitHub Actions `.github/workflows/build.yml` |
| **Repo** | 🔒 Privat: `github.com/JONIMONI09/VibeCoreOS` |

---

## 🎯 Jules' Aufgaben & Ziele

### Primäre Ziele
1. **Bugs finden & fixen**: Nutze ALLE Analyse-Tools (fanalyzer, cppcheck, flawfinder)
2. **Features implementieren**: Nur nach Absprache, keine wilden Refactorings
3. **Security-Audit**: `make audit` — cppcheck + flawfinder + fanalyzer
4. **Dokumentation pflegen**: README.md, ARCHITECTURE.md, TESTING.md, RULES.md, Jules.md
5. **CI/CD überwachen**: GitHub Actions Build-Status prüfen
6. **Build-System warten**: Makefile-Targets aktuell halten

### Bei JEDEM Task
- `ARCHITECTURE.md` + `README.md` zuerst lesen
- `make clean && make -j$(nproc)` Baseline
- `make check` Pre-Commit Check
- `make run` QEMU Boot-Test
- Code-Review einholen
- Dokumentation updaten
- **`make clean` + `git status` → Root MUSS sauber sein!**

---

## 🚫 Absolute Verbote (10 Regeln)

| # | Verbot | Begründung |
|---|--------|-----------|
| 1 | **`sudo`/`su` im Terminal** | Terminal-Korruption! Auth nur via `auth-helper.sh` (GUI) |
| 2 | **Echter Raspberry Pi Zugriff** | Pi bleibt unberührt — 100% QEMU |
| 3 | **`any`/`void*` Casts ohne Grund** | Type-Safety ist kritisch in C |
| 4 | **`git push --force`** | Datenverlust-Risiko |
| 5 | **Build-Artefakte committen** | `.gitignore` MUSS respektiert werden |
| 6 | **Compiler-Warnings ignorieren** | `-Wall -Wextra -Werror` ist Pflicht |
| 7 | **Compiler-Flags lockern** | Security-Flags sind nicht verhandelbar |
| 8 | **`make install` ohne SD-Karte** | Nur für echtes Flashen auf Hardware |
| 9 | **Ohne QEMU-Test commiten** | `make run` MUSS Shell-Prompt erreichen |
| 10 | **Dokumentation veralten lassen** | IMMER aktuell halten! |
| 11 | **Build-Artefakte im Root liegen lassen** | `make clean` nach jedem Build. Root MUSS sauber sein! |
| 12 | **Lose Dateien oder Duplikate** | Alles hat seinen Platz — Skripte in `scripts/`, keine doppelten Docs |

---

## 📋 Vollständiger Task-Workflow (5 Phasen)

### Phase 1: KONTEXT (30% der Zeit)
```
□ ARCHITECTURE.md + README.md lesen
□ Relevante src/*.c Dateien studieren
□ Relevante include/*.h Header prüfen
□ Bestehenden ähnlichen Code analysieren
□ Code-Stil, Namensmuster, Struktur verstehen
□ linker.ld Memory-Map prüfen (bei Speicher-Fragen)
```

### Phase 2: ANALYSE (20% der Zeit)
```
□ make clean && make -j$(nproc)        ← Baseline Build (0 Fehler?)
□ make check                            ← Schnell-Check
□ make fanalyzer                        ← GCC Deep Analysis
□ make cppcheck                         ← Bug & UB Detection
□ make flawfinder                       ← CWE/SANS Security Scan
□ make audit                            ← Full Audit (bei großen Änderungen)
□ thinker-with-files-gemini             ← Bei komplexen Problemen
```

### Phase 3: IMPLEMENTIERUNG (30% der Zeit)
```
□ write_todos                            ← Planung bei 3+ Schritten
□ str_replace für Änderungen             ← BEVORZUGT (präziser)
□ write_file NUR für neue Dateien        ← Ganze Datei neu
□ Code-Stil 1:1 vom Bestand übernehmen   ← KEINE Abweichungen!
□ Alle Referenzen updaten                ← code-searcher spawnen
□ Keine toten Imports, keine ungenutzten Variablen
□ Keine Magic Numbers                    ← Defines/Const verwenden
```

### Phase 4: VALIDIERUNG (15% der Zeit)
```
□ make clean && make -j$(nproc)         ← MUSS 0 Fehler, 0 Warnings
□ make check                            ← Pre-Commit bestanden?
□ make run                              ← QEMU Boot (Shell erreichbar?)
□ make harden-test                      ← Gehärteter Kernel bootet?
□ code-reviewer-deepseek                ← Code Review PARALLEL
□ Alle Review-Findings fixen
```

### Phase 5: DOKUMENTATION + COMMIT (5% der Zeit)
```
□ README.md Changelog updaten
□ ARCHITECTURE.md (bei Architektur-Änderungen)
□ TESTING.md (bei neuen Test-Methoden)
□ RULES.md (bei neuen Regeln/Tools)
□ Jules.md (bei neuen Jules-Regeln)
□ BRAINSTORM.md (bei neuen Ideen/Roadmap-Änderungen)
□ make help-Text (bei neuen Targets)
□ make clean              ← Root aufräumen!
□ git status              ← Prüfen: keine .o/.d im Root!
□ git add -A
□ git commit -m "Bereich: Beschreibung"
□ git push origin master
```

---

## 🔒 Security & Analyse — Alle Tools

### Build
```bash
make -j$(nproc)       # Normaler Build (~2s)
make harden           # Hardened Build (stack-clash-protection)
make harden-test      # Hardened Build + QEMU Boot-Test
make check            # Pre-Commit: compile + cppcheck
```

### Security
```bash
make fanalyzer        # GCC Deep Analysis (use-after-free, overflow, NULL)
make flawfinder       # CWE/SANS Top 25 Security Patterns
make security         # Security Audit Summary
```

### Logic
```bash
make cppcheck         # Bug & Undefined Behavior Detection
make clang-tidy       # Code Quality & CERT Compliance
make clang-analyzer   # Clang Static Analyzer (Deep Logic)
make logic            # Logic Check Summary
```

### Full Audit
```bash
make audit            # cppcheck + flawfinder + fanalyzer (~30s)
make analyze          # cppcheck + flawfinder (~10s)
```

---

## 💿 ISO-System

```bash
make iso              # Auto: GUI=full, headless=noroot
make iso-noroot       # Basis-Image (MBR+FAT32, KEIN Root)
make iso-full         # Volles Image (braucht Desktop-GUI)
make iso-verify       # Partition + FAT32 prüfen
make iso-test         # ISO in QEMU testen
make firmware         # RPi-Firmware downloaden
```

**ISO-Struktur**: `build/vibecore.iso` (128 MB, MBR+FAT32, KEIN ISO 9660!)

---

## 🖥️ QEMU (NUR so testen!)

```bash
make run              # Terminal-Mode (raspi3b, nographic)
make run-gui          # Grafik-Mode (GTK Display)
make debug            # GDB Debug-Server (:1234)
```

**Niemals** VM-Direktboot! Kein UEFI, kein BIOS — nur QEMU `-kernel` Flag.

---

## 📁 Wichtige Dateien

| Datei | Zweck | Bei Änderung |
|-------|-------|-------------|
| `src/kernel.c` | Haupt-Init (14 Phasen) | ARCHITECTURE.md updaten |
| `src/boot.S` | ARMv8 Entry + Vectors | Mit äußerster Vorsicht! |
| `src/framebuffer.c` | GPU-FB (128-bit STP) | QEMU run-gui testen |
| `src/mailbox.c` | ARM↔GPU Mailbox | `make run` testen |
| `src/interrupt.c` | Exception-Dispatch | Crash-Screen prüfen |
| `src/scheduler.c` | BORE-Scheduler | `make run` testen |
| `src/recovery.c` | 8-Subsystem Recovery | FSCK-Test |
| `src/gui.c` | Desktop | `make run-gui` testen |
| `src/shell.c` | Shell (18 Commands) | `make run` testen |
| `include/peripherals.h` | BCM2837 MMIO | Mit Vorsicht! |
| `include/types.h` | Typen + Makros | Alle Sourcen prüfen |
| `linker.ld` | Memory-Layout | Stack/Heap prüfen |
| `Makefile` | Build-System | `make help` updaten |
| `scripts/auth-helper.sh` | GUI-Auth | KEIN Terminal-Fallback! |
| `scripts/mk-bootmbr.py` | MBR Generator | `make iso-verify` testen |
| `BRAINSTORM.md` | Roadmap & Ideen | Bei neuen Features/Zielen |
| `.github/workflows/build.yml` | CI/CD | GH Actions prüfen |

---

## 🐛 Bekannte Bugs & Baustellen

| # | Bug | Datei | Schwere |
|---|-----|-------|---------|
| 1 | MMU deaktiviert (MMIO-Caching) | `mmu.c`, `kernel.c` | HIGH |
| 2 | Kein EMMC/SD-Treiber | `fs.c` | HIGH |
| 3 | Kein USB-Treiber | — | MEDIUM |
| 4 | Kein UEFI-Boot (VMs) | — | MEDIUM |
| 5 | `va_arg` Type-Mismatch (`snprintf_local`) | `interrupt.c` | MEDIUM |
| 6 | Fehlende Prototypen | `boot_anim.c`, `crashlog.c` | LOW |
| 7 | Precision-Loss Gradient | `boot_anim.c` | LOW |
| 8 | Sign-Change in Crashlog | `crashlog.c` | LOW |

### Kürzlich behoben (2026-06-15)
| Bug | Fix |
|-----|-----|
| `mailbox_call` falscher Return-Check | `buffer[1] == MBOX_RESPONSE` |
| GPU-Adresse falscher Slot | `buf[23]` (base) + `buf[24]` (size) |
| `framebuffer_fillrect` Alignment-Fault | `IS_ALIGNED(buf,8)` + 32-bit Fallback |
| ISO-Dateiendung `.img` statt `.iso` | Umbenannt auf `build/vibecore.iso` |
| MBR ohne Boot-Code → VM "not bootable" | `scripts/mk-bootmbr.py` (440-Byte MBR) |
| IRQ-Endlosschleife | Write-1-to-Clear + dmb |
| `snprintf_local` Buffer-Overflow | `max==0` Guard |
| Auth-Helper Terminal-Fallback | Nur noch Desktop-GUI (zenity/pkexec) |

---

## 🔒 Security-Checkliste (vor JEDEM Commit)

```
□ make clean && make -j$(nproc)    → 0 Fehler, 0 Warnings
□ make check                       → Pre-Commit bestanden
□ make run                         → Bootet bis Shell-Prompt
□ make harden-test                 → Gehärteter Kernel bootet
□ Keine sensitiven Daten im Diff
□ .gitignore aktuell
□ Root sauber: keine .o, .d, Duplikate im Root!
□ Kein sudo/su im Code
□ Stack-Canary intakt (-fstack-protector-strong)
□ -mstrict-align aktiv
□ -fstack-clash-protection (bei harden)
□ README.md Changelog aktuell
```

---

## 📊 Referenzwerte

| Metrik | Soll | Toleranz |
|--------|------|----------|
| Kernel-Größe | 62 KB | ±5 KB |
| Build-Zeit | ~2s | <5s |
| Boot-Zeit (QEMU) | ~1.6s | <3s |
| Heap | 1 MB | — |
| Stack | 128 KB | — |
| Compile-Warnings | 0 | **0** |
| Fanalyzer-Warnings | <5 | Informational |

---

_Jules-Regeln v2.0 — 15. Juni 2026 — VibeCore Labs_
