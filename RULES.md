# VibeCore OS — Regeln & KI-Agent Vorgehen (Codebuff/Buffy)

## 🎯 Projekt-Ziel

**VibeCore OS "Photon"** ist ein Bare-Metal ARM64 Betriebssystem für Raspberry Pi 3B/4B.
Kein Linux, kein Userspace — alles von Grund auf in C und ARMv8 Assembly gebaut.

Ziel: Ein **funktionierendes, bootfähiges OS** mit grafischem Desktop, Recovery-System,
Shell und ISO-Build-System — entwickelt und getestet mit KI-Unterstützung (Codebuff/Buffy).

---

## 🤖 KI-Agent Grundprinzipien

### 1. Verstehen vor Handeln
- Immer zuerst **Kontext sammeln**: Dateien lesen, Code durchsuchen, Web recherchieren
- `ARCHITECTURE.md` + `README.md` lesen, bevor du Code anfasst
- Bestehende Konventionen, Code-Stil, Architektur-Muster EXAKT nachahmen
- **NIEMALS** wild refactorn oder Struktur ändern ohne explizite Absprache

### 2. Qualität über Geschwindigkeit
- Korrektheit > Produktivität
- Lieber 3 gut informierte Agents als 10 überhastete
- Im Zweifel: nachfragen (`ask_user`)

### 3. NIEMALS Terminal-Passwort-Eingabe
- `sudo`/`su` triggert **Terminal-Korruption** — zerstört den Chat!
- `auth-helper.sh` nutzt NUR Desktop-GUI (zenity/pkexec), kein Terminal-Fallback
- Build-Operationen ohne GUI: `make iso-noroot` (kein Root nötig)

### 4. Nur QEMU zum Testen
- **KEIN echter Raspberry Pi Zugriff!** Der Pi bleibt unberührt.
- Alle Tests im Emulator: `make run`, `make run-gui`, `make iso-test`
- `make harden-test` für gehärteten Kernel in QEMU

### 5. Dokumentation IMMER aktuell halten
- README.md, ARCHITECTURE.md, TESTING.md, RULES.md, Jules.md
- Bei JEDER signifikanten Änderung: Changelog in README.md updaten
- Neue Befehle/Targets: `make help`-Text updaten

---

## 📋 Vollständiger Agent-Workflow

### Phase 1: KONTEXT (30% der Zeit)
```
1. ARCHITECTURE.md + README.md lesen
2. Relevante src/-Dateien mit read_files lesen
3. Relevante include/-Header prüfen
4. file-picker + code-searcher spawnen (parallel!)
5. researcher-web/docs spawnen (bei externen APIs/Tools)
6. Bestehenden ähnlichen Code studieren
```

### Phase 2: ANALYSE (20% der Zeit)
```
1. make clean && make -j$(nproc)        ← Baseline Build
2. make check                            ← Pre-Commit Schnell-Check
3. make fanalyzer                        ← GCC Deep Analysis
4. make cppcheck                         ← Bug & UB Detection
5. make flawfinder                       ← CWE/SANS Security
6. thinker-with-files-gemini             ← bei komplexen Problemen
```

### Phase 3: IMPLEMENTIERUNG (30% der Zeit)
```
1. write_todos für Planung               ← bei 3+ Schritten
2. str_replace für Änderungen            ← BEVORZUGT
3. write_file NUR für neue Dateien
4. Code-Stil des existierenden Codes MUSS 1:1 übernommen werden
5. Alle Referenzen auf geänderte Symbole updaten
6. Keine toten Imports, keine ungenutzten Variablen
```

### Phase 4: VALIDIERUNG (15% der Zeit)
```
1. make clean && make -j$(nproc)         ← MUSS 0 Fehler, 0 Warnings
2. make check                            ← Pre-Commit Check
3. make run                              ← QEMU Boot-Test (Shell erreichbar?)
4. make harden-test                      ← Hardened Kernel testen
5. code-reviewer-deepseek spawnen        ← PARALLEL zum Testen
```

### Phase 5: DOKUMENTATION + COMMIT (5% der Zeit)
```
1. README.md Changelog updaten
2. ARCHITECTURE.md bei Architektur-Änderungen
3. TESTING.md bei neuen Test-Methoden
4. RULES.md / Jules.md bei neuen Regeln
5. git add -A && git commit -m "..."
6. git push origin master                ← NIEMALS --force!
```

---

## 🔒 Security & Analyse — ALLE Tools

### Build-Tools
| Befehl | Zweck | Dauer |
|--------|-------|-------|
| `make` | Normaler Build | 2s |
| `make harden` | Gehärteter Build (stack-clash-protection) | 3s |
| `make harden-test` | Hardened Build + QEMU Boot-Test | 15s |
| `make check` | Pre-Commit Quick-Check (compile+cppcheck) | 5s |

### Security-Tools
| Befehl | Zweck | Tool |
|--------|-------|------|
| `make fanalyzer` | Deep Analysis (use-after-free, overflow, NULL) | GCC |
| `make flawfinder` | CWE/SANS Top 25 Security Patterns | flawfinder |
| `make security` | Security Audit Summary | flawfinder |

### Logic-Tools
| Befehl | Zweck | Tool |
|--------|-------|------|
| `make cppcheck` | Bug & Undefined Behavior Detection | cppcheck |
| `make clang-tidy` | Code Quality & Style | clang-tidy |
| `make clang-analyzer` | Deep Logic Errors | Clang SA |
| `make logic` | Logic Check Summary | cppcheck |

### Full Audit
| Befehl | Zweck | Dauer |
|--------|-------|-------|
| `make analyze` | cppcheck + flawfinder | 10s |
| `make audit` | Voll-Audit: cppcheck + flawfinder + fanalyzer | 30s |

---

## 💿 ISO/Image-System Regeln

### ISO-Targets (NIE ändern ohne Absprache!)
| Target | Beschreibung | Root? |
|--------|-------------|-------|
| `make iso` | Auto: GUI→full, sonst→noroot | Auto |
| `make iso-noroot` | Basis-Image (nur MBR+FAT32) | **NEIN** |
| `make iso-full` | Volles Image mit Dateien | Ja (GUI) |
| `make iso-verify` | Partition + FAT32 prüfen | Nein |
| `make iso-test` | ISO in QEMU testen | Nein |
| `make firmware` | RPi-Firmware downloaden | Nein |

### ISO-Struktur (NICHT ändern!)
- Datei: `build/vibecore.iso` (128 MB)
- Format: MBR + FAT32 (**KEIN** ISO 9660!)
- MBR: 440 Byte Boot-Code + Partitionstabelle + 0x55AA
- Partition: FAT32, bootable, startet bei Sektor 2048
- Enthält: kernel8.img, config.txt, bootcode.bin, start.elf, fixup.dat

---

## 🖥️ QEMU Regeln

### QEMU-Targets
| Befehl | Maschine | CPU | Display |
|--------|----------|-----|---------|
| `make run` | raspi3b | cortex-a53 | nographic |
| `make run-gui` | raspi3b | cortex-a53 | GTK |
| `make debug` | raspi3b | cortex-a53 | GDB :1234 |

### QEMU-Regeln
- **KEIN** VM-Direktboot! VibeCore braucht `-kernel` Flag
- **KEIN** `-M virt` — muss `raspi3b` sein
- Memory: **1G** Minimum
- Immer `-serial stdio -nographic` für Console-Mode
- ISO-Test: `-kernel kernel8.img -drive file=build/vibecore.iso,if=sd`

---

## 🚫 Absolute Verbote

| # | Verbot | Grund |
|---|--------|-------|
| 1 | `sudo`/`su` im Terminal | Terminal-Korruption! |
| 2 | Echter RPi Hardware-Zugriff | Pi bleibt unberührt |
| 3 | `any`/`void*` Casts ohne Grund | Type-Safety |
| 4 | `git push --force` | Datenverlust |
| 5 | Build-Artefakte committen | .gitignore respektieren |
| 6 | Compiler-Warnings ignorieren | `-Wall -Wextra -Werror` |
| 7 | Compiler-Flags lockern | Security-Flags sind Pflicht |
| 8 | `make install` ohne SD-Karte | Nur für echtes Flashen |
| 9 | Ohne QEMU-Test commiten | `make run` muss funktionieren |
| 10 | Doku veralten lassen | IMMER aktuell halten |

---

## 📁 Projekt-Struktur (NIE ändern!)

```
Vibe_Core_Labor/
├── README.md              ← Haupt-Doku (DE)
├── ARCHITECTURE.md        ← Architektur-Doku (EN)
├── TESTING.md             ← Test-Guide
├── RULES.md               ← DIESE DATEI
├── Jules.md               ← Google Jules Regeln
├── Makefile               ← Build-System (NIE zerstören!)
├── linker.ld              ← Memory-Map
├── config.txt             ← RPi Boot-Config
├── .gitignore             ← Build-Artefakte ignoriert
├── .github/workflows/     ← CI/CD (build.yml)
├── scripts/
│   ├── auth-helper.sh     ← GUI-Auth (zenity/pkexec, KEIN Terminal!)
│   └── mk-bootmbr.py      ← MBR-Boot-Code Generator
├── src/                   ← Kernel (22 Dateien)
├── include/               ← Header (22 Dateien)
├── build/                 ← ISO-Output (.gitignored)
├── doc/                   ← Zusatz-Doku
└── fonts/                 ← Fonts
```

---

## 🐛 Bug-Fix Protokoll

```
1. BUG FINDEN
   ├── make fanalyzer          ← GCC Deep Analysis
   ├── make cppcheck           ← Bug & UB Scan
   ├── make flawfinder         ← Security Scan
   └── thinker-with-files-gemini ← Deep Analysis

2. BUG DOKUMENTIEREN
   ├── Datei:Zeile
   ├── Schweregrad (CRITICAL/HIGH/MEDIUM/LOW)
   └── Beschreibung + Fix-Vorschlag

3. FIX IMPLEMENTIEREN
   ├── str_replace (bevorzugt)
   └── Code-Stil 1:1 nachahmen

4. VALIDIEREN
   ├── make clean && make      ← 0 Fehler
   ├── make run                ← QEMU Boot
   ├── make harden-test        ← Security Build
   └── code-reviewer-deepseek  ← Review

5. DOKU + COMMIT
   ├── README.md Changelog updaten
   ├── RULES.md bei neuen Regeln
   └── git commit + push
```

---

## 🔒 Security-Checkliste (vor JEDEM Commit)

- [ ] `make clean && make -j$(nproc)` → **0 Fehler, 0 Warnings**
- [ ] `make check` → Pre-Commit Check bestanden
- [ ] `make run` → Bootet bis Shell-Prompt
- [ ] `make harden-test` → Gehärteter Kernel bootet
- [ ] Keine sensitiven Daten im Diff
- [ ] `.gitignore` aktuell
- [ ] Keine `sudo`/`su` im Code
- [ ] Stack-Canary intakt (`-fstack-protector-strong`)
- [ ] `-mstrict-align` aktiv
- [ ] README.md Changelog aktuell

---

## 🎓 Namens-Konventionen

| Typ | Konvention | Beispiel |
|-----|-----------|---------|
| Dateien | snake_case | `boot_anim.c` |
| Funktionen | snake_case | `framebuffer_init()` |
| Typen/Structs | PascalCase | `FramebufferInfo` |
| Makros/Defines | UPPER_SNAKE | `FB_DEFAULT_WIDTH` |
| Header-Guards | `_NAME_H` | `_KERNEL_H` |
| Globale Variablen | g_ prefix | `g_fb_info` |
| Static Functions | kein prefix | `pack32()` |
| Enum-Werte | UPPER_SNAKE | `LOG_LEVEL_INFO` |

## 🏷️ Commit-Message-Format

```
<Bereich>: <Kurzbeschreibung>

<Details (optional)>

Beispiele:
  Docs: README + ARCHITECTURE aktualisiert
  Build: Security-Tools (harden, fanalyzer, audit)
  Fix: mailbox_call response code check
  Kernel: Boot-Animation Alignment-Fault behoben
```

---

## 📊 Projekt-Kennzahlen (Referenz)

| Metrik | Wert |
|--------|------|
| Kernel-Größe | 62 KB |
| Quellcode-Dateien | 22 (.c + .S) + 22 (.h) |
| Build-Zeit | ~2s (parallel) |
| Boot-Zeit (QEMU) | ~1.6s |
| Grafik | 1024×768×32 |
| Heap | 1 MB + 64 KB Emergency |
| Stack | 128 KB |
| Max Tasks | 32 |

---

_Letzte Aktualisierung: 15. Juni 2026 — VibeCore Labs_
