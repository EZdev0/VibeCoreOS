# VibeCore OS — Rules & KI-Agent Vorgehen

## 🎯 Projekt-Ziel

**VibeCore OS "Photon"** ist ein Bare-Metal ARM64 Betriebssystem für Raspberry Pi 3B/4B.
Kein Linux, kein Userspace — alles von Grund auf in C und ARMv8 Assembly gebaut.

Ziel: Ein **funktionierendes, bootfähiges OS** mit grafischem Desktop, Recovery-System,
Shell und ISO-Build-System — entwickelt und getestet mit KI-Unterstützung (Codebuff/Buffy).

---

## 🤖 KI-Agent Regeln (Codebuff/Buffy)

### Grundprinzipien

1. **Verstehen vor Handeln**: Immer zuerst Kontext sammeln, Dateien lesen, recherchieren —
   dann erst Änderungen vornehmen.

2. **Qualität über Geschwindigkeit**: Korrektheit ist wichtiger als Produktivität zu zeigen.
   Lieber wenige, gut informierte Agents als viele überhastete.

3. **NIEMALS Terminal-Passwort-Eingabe**: `sudo`/`su` triggert Terminal-Korruption.
   Der `auth-helper.sh` nutzt NUR Desktop-GUI-Popups (zenity/pkexec).
   Bei Build-Operationen ohne GUI: `make iso-noroot` verwenden.

4. **Nur QEMU zum Testen**: Kein echter Raspberry Pi Zugriff! Der Pi bleibt unberührt.
   Alle Tests laufen im Emulator: `make run`, `make run-gui`, `make iso-test`.

5. **Bestehende Konventionen respektieren**: Code-Style, Namensgebung, Architektur-Muster
   des existierenden Codes exakt nachahmen. Keine wilden Refactorings ohne Absprache.

### Vorgehen bei Änderungen

1. **Kontext sammeln**: `file-picker`, `code-searcher`, `researcher-web/docs` spawnen
2. **Dateien lesen**: `read_files` für alle relevanten Dateien
3. **Thinker**: `thinker-with-files-gemini` für komplexe Probleme
4. **Änderungen**: `str_replace` (bevorzugt) oder `write_file`
5. **Build testen**: `make clean && make -j$(nproc)` — muss fehlerfrei sein
6. **QEMU testen**: `make run` — OS muss booten, Shell muss erreichbar sein
7. **Code Review**: `code-reviewer-deepseek` nach signifikanten Änderungen
8. **Aufräumen**: Keine Build-Artefakte, keine temporären Dateien hinterlassen

### Wichtige Constraints

- **Kein "any" Typ**: Keine unsicheren Type-Casts in C
- **Keine globalen Pakete**: Immer den Projekt-Paketmanager nutzen
- **Code-Wiederverwendung**: Vorhandene Hilfsfunktionen, Komponenten nutzen
- **Idiomatischer Code**: Lokalen Kontext (Imports, Funktionen) verstehen und passend integrieren
- **Alle Referenzen updaten**: Bei Änderung exportierter Symbole alle Aufrufer finden und anpassen

---

## 🏗️ Build-System Regeln

### Build-Kommandos (NIE ändern ohne Absprache!)

```bash
make -j$(nproc)     # Paralleler Build
make run            # QEMU Test (Terminal)
make run-gui        # QEMU Test (Grafik)
make iso            # ISO erstellen (auto: GUI/noroot)
make iso-noroot     # ISO OHNE Root-Rechte
make iso-full       # Volles ISO (Desktop-GUI Auth)
make iso-verify     # ISO verifizieren
make iso-test       # ISO in QEMU testen
make clean          # Build-Artefakte löschen
```

### Compiler-Flags (NIE lockern!)

```
-Wall -Wextra -Werror -O3 -nostdlib -nostartfiles
-ffreestanding -mgeneral-regs-only -mstrict-align
-fstack-protector-strong -fno-exceptions
-fno-omit-frame-pointer -fno-delete-null-pointer-checks
```

### CI/CD (GitHub Actions)

- **Workflow**: `.github/workflows/build.yml`
- **Runner**: `ubuntu-latest`
- **Compiler**: `gcc-aarch64-linux-gnu`
- **Schritte**: Install → Build → QEMU-Smoke-Test → Artifact-Upload
- **Bei Fehler**: Build bricht ab, Issue wird erstellt

---

## 📁 Projekt-Struktur (NIE ändern ohne Absprache!)

```
Vibe_Core_Labor/
├── src/           ← Kernel-Quellcode (.c + .S)
├── include/       ← Header-Dateien (.h)
├── scripts/       ← Build-Helfer (auth-helper.sh, mk-bootmbr.py)
├── build/         ← Build-Output + Firmware (.gitignored)
├── doc/           ← Zusätzliche Dokumentation
├── fonts/         ← Font-Dateien
├── .github/       ← GitHub Actions Workflows
├── Makefile       ← Build-System
├── linker.ld      ← Linker-Script
├── config.txt     ← RPi Boot-Konfiguration
├── README.md      ← Haupt-Dokumentation
├── ARCHITECTURE.md← Architektur-Doku
├── TESTING.md     ← Test-Dokumentation
├── RULES.md       ← Diese Datei
└── .gitignore     ← Git-Ignore-Regeln
```

---

## 🔒 Sicherheits-Regeln

1. **Keine sensitiven Daten im Code**: Keine Passwörter, Token, Keys
2. **Stack-Schutz aktiv**: `-fstack-protector-strong` immer an
3. **NULL-Checks**: `CHECK_NULL()` Makro verwenden
4. **Kein `sudo` im Terminal**: Auth nur über `auth-helper.sh` (Desktop-GUI)
5. **Kein echter Hardware-Zugriff**: Nur QEMU-Emulation
6. **Build-Artefakte ignoriert**: `.gitignore` aktuell halten

---

## 📝 Commit-Regeln

- **Commit-Messages**: Klar, beschreibend, auf Deutsch oder Englisch
- **Keine Build-Artefakte**: `.o`, `.d`, `kernel8.img`, `kernel8.elf` sind ignoriert
- **Vor jedem Commit**: `make clean && make -j$(nproc)` — Build muss sauber sein
- **Keine `-f`/`--force`**: Kein gewaltsames Pushen
- **Kein direktes `main`/`master` Pushen ohne Test**

---

## 🐛 Bug-Fix Protokoll

Bei gefundenen Bugs:
1. Bug dokumentieren (Datei, Zeile, Beschreibung)
2. Fix implementieren
3. Build verifizieren (`make clean && make`)
4. QEMU testen (`make run`)
5. Code-Review (`code-reviewer-deepseek`)
6. README.md Changelog updaten
7. Commit + Push

---

## 🎓 Namens-Konventionen

| Typ | Konvention | Beispiel |
|-----|-----------|---------|
| Dateien | snake_case | `boot_anim.c` |
| Funktionen | snake_case | `framebuffer_init()` |
| Typen | PascalCase | `FramebufferInfo` |
| Makros | UPPER_SNAKE | `FB_DEFAULT_WIDTH` |
| Header-Guards | `_NAME_H` | `_KERNEL_H` |
| Globale Variablen | g_ prefix | `g_fb_info` |

---

_Letzte Aktualisierung: 15. Juni 2026 — VibeCore Labs_
