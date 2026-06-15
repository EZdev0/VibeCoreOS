# VibeCore OS — Rules & AI Agent Guide (Codebuff/Buffy)

## 🎯 Project Goal

**VibeCore OS "Photon"** is a bare-metal ARM64 operating system for Raspberry Pi 3B/4B.
No Linux, no userspace — everything built from scratch in C and ARMv8 Assembly.

Goal: A **fully functional, bootable OS** with a graphical desktop, recovery system,
shell, and ISO build system — developed and tested with AI assistance (Codebuff/Buffy).

---

## 🤖 AI Agent Core Principles

### 1. Understand Before Acting
- Always **gather context first**: read files, search code, research the web
- Read `ARCHITECTURE.md` + `README.md` BEFORE touching any code
- EXACTLY follow existing conventions, code style, and architecture patterns
- **NEVER** refactor or restructure without explicit approval

### 2. Quality Over Speed
- Correctness > Productivity
- Prefer 3 well-informed agents over 10 rushed ones
- When in doubt: ask (`ask_user`)

### 3. NEVER Use Terminal Password Prompts
- `sudo`/`su` triggers **terminal corruption** — destroys the chat session!
- `auth-helper.sh` uses ONLY desktop GUI (zenity/pkexec), no terminal fallback
- For non-GUI operations: `make iso-noroot` (no root required)

### 4. QEMU Only for Testing
- **NO real Raspberry Pi access!** The Pi stays untouched.
- All tests in emulator: `make run`, `make run-gui`, `make iso-test`
- `make harden-test` for hardened kernel QEMU test

### 5. Documentation MUST Always Be Current
- README.md, ARCHITECTURE.md, TESTING.md, RULES.md, Jules.md, BRAINSTORM.md
- Update Changelog in README.md with EVERY significant change
- Update `make help` text when adding new targets

### 6. AI Agent Sandbox — Tools MUST Be Installed
- **Install ALL analysis tools in your sandbox** at the start of every session:
  ```bash
  # Cross-compiler (mandatory for builds)
  sudo apt install -y gcc-aarch64-linux-gnu qemu-system-arm
  
  # Static analysis (mandatory for code quality)
  pip3 install --break-system-packages --user flawfinder
  sudo apt install -y cppcheck
  
  # Optional but recommended
  sudo apt install -y clang-tidy dosfstools
  ```
- If `sudo` is not available in your sandbox: use `pip3 install --user` for Python tools
- **Verify installation**: run `which cppcheck && which flawfinder && which qemu-system-aarch64`
- If a tool can't be installed → document the limitation and use alternative checks
- **ALWAYS test the OS** after changes: `make run` (QEMU boot) + `make check` (analysis)
- Run `make audit` at least once per session to verify code quality

### 7. Project Cleanliness MANDATORY
- **NO** build artifacts in root directory! Run `make clean` after every build
- **NO** `.o`/`.d` files outside `build/` or temporarily in `src/`
- **NO** loose scripts in root — all scripts go in `scripts/`
- **NO** duplicates! `doc/` was deleted — only ONE version of each doc exists
- Empty directories (`fonts/`) must be deleted or filled immediately
- After EVERY session: `make clean && git status` — root MUST be clean
- Before EVERY commit: clean root, check `.gitignore`, run `git status`

---

## 🧰 AI Agent Sandbox Setup (EVERY session!)

Before starting ANY task, AI agents MUST set up their sandbox:

### Phase 0: SANDBOX SETUP (first 2 minutes) — DO THIS FIRST!
```
1. Check installed tools:
   which cppcheck flawfinder qemu-system-aarch64 aarch64-linux-gnu-gcc
   
2. Install missing tools:
   - pip3 install --break-system-packages --user flawfinder  (if missing)
   - sudo apt install -y cppcheck qemu-system-arm           (via sudo or pkexec)
   
3. Verify toolchain:
   aarch64-linux-gnu-gcc --version
   cppcheck --version
   flawfinder --version
   qemu-system-aarch64 --version
   
4. If sudo NOT available (pure sandbox):
   - pip3 install --user cppcheck (if pip version exists)
   - Document which tools are missing
   - Skip sudo-dependent tests, note limitations
   
5. Build baseline:
   make clean && make -j$(nproc)
   make run -- QEMU smoke test (timeout 15s, verify shell prompt)
```

### AI Agent Golden Rules
- **Install tools FIRST** — don't write code without the toolchain
- **Test EARLY, test OFTEN** — run `make run` after EVERY significant change
- **3-file limit** — if a change touches >5 files, STOP and write a plan first
- **No assumptions** — NEVER assume a library exists. Check imports, headers, Makefile
- **Sandbox limitations** — if your sandbox lacks `sudo`, use `pip install --user` and note it
- **Full audit at least once** — run `make audit` at least once per coding session

---

## 📋 Complete Agent Workflow

### Phase 1: CONTEXT (30% of time)
```
1. Read ARCHITECTURE.md + README.md
2. Read relevant src/ files with read_files
3. Check relevant include/ headers
4. Spawn file-picker + code-searcher (in parallel!)
5. Spawn researcher-web/docs (for external APIs/tools)
6. Study existing similar code
```

### Phase 2: ANALYSIS (20% of time)
```
1. make clean && make -j$(nproc)         ← Baseline build (MUST pass!)
2. make check                             ← Pre-commit quick check
3. make fanalyzer                         ← GCC deep analysis
4. make cppcheck                          ← Bug & UB detection
5. make flawfinder                        ← CWE/SANS security
6. make run                               ← QEMU boot test (shell reachable?)
7. thinker-with-files-gemini              ← Complex problems
```

### Phase 3: IMPLEMENTATION (30% of time)
```
1. write_todos for planning               ← 3+ steps
2. str_replace for changes                ← PREFERRED
3. write_file ONLY for new files
4. Code style MUST match existing code 1:1
5. Update all references to changed symbols
6. No dead imports, no unused variables
7. NEVER modify >5 files without stopping to plan
8. After each logical change: make -j$(nproc) → fix errors → continue
```

### Phase 4: VALIDATION (15% of time)
```
1. make clean && make -j$(nproc)          ← MUST have 0 errors, 0 warnings
2. make check                             ← Pre-commit check (cppcheck + flawfinder)
3. make run                               ← QEMU boot test (shell prompt?)
4. make harden-test                       ← Test hardened kernel
5. make iso-noroot                        ← Test ISO build (no root needed)
6. spawn code-reviewer-deepseek-flash     ← PARALLEL with testing
```

### Phase 5: DOCUMENTATION + COMMIT (5% of time)
```
1. Update README.md Changelog
2. Update ARCHITECTURE.md on architecture changes
3. Update TESTING.md on new test methods
4. Update RULES.md / Jules.md on new rules
5. make clean                           ← Clean root!
6. git status                           ← Check: no .o/.d in root
7. git add -A && git commit -m "..."
8. git push origin master               ← NEVER --force!
```

---

## 🔒 Security & Analysis — ALL Tools

### Build Tools
| Command | Purpose | Duration |
|---------|---------|----------|
| `make` | Normal build | 2s |
| `make harden` | Hardened build (stack-clash-protection) | 3s |
| `make harden-test` | Hardened build + QEMU boot test | 15s |
| `make check` | Pre-commit quick check (compile+cppcheck) | 5s |

### Security Tools
| Command | Purpose | Tool |
|---------|---------|------|
| `make fanalyzer` | Deep analysis (use-after-free, overflow, NULL) | GCC |
| `make flawfinder` | CWE/SANS Top 25 security patterns | flawfinder v2.0.20 |
| `make security` | Security audit summary | flawfinder |

### Logic Tools
| Command | Purpose | Tool |
|---------|---------|------|
| `make cppcheck` | Bug & undefined behavior detection | cppcheck v2.17.1 |
| `make clang-tidy` | Code quality & style | clang-tidy |
| `make clang-analyzer` | Deep logic errors | Clang SA |
| `make logic` | Logic check summary | cppcheck |

### Full Audit
| Command | Purpose | Duration |
|---------|---------|----------|
| `make analyze` | cppcheck + flawfinder | 10s |
| `make audit` | Full audit: cppcheck + flawfinder + fanalyzer | 30s |

### Tool Installation (DO NOT use terminal sudo — use auth-helper.sh!)
```bash
# flawfinder (pip, no root needed)
pip3 install --break-system-packages --user flawfinder

# cppcheck (via auth-helper GUI popup)
./scripts/auth-helper.sh sh -c "apt-get install -y cppcheck"
```

---

## 💿 ISO/Image System Rules

### ISO Targets (NEVER change without approval!)
| Target | Description | Root? |
|--------|-------------|-------|
| `make iso` | Auto: GUI→full, else→noroot | Auto |
| `make iso-noroot` | Base image (MBR+FAT32 only) | **NO** |
| `make iso-full` | Full image with files | Yes (GUI) |
| `make iso-verify` | Verify partition + FAT32 | No |
| `make iso-test` | Test ISO in QEMU | No |
| `make firmware` | Download RPi firmware | No |

### Critical: ISO must be EXPLICITLY built!
- The ISO is **NOT** created automatically by `make` or `make run`
- You MUST run `make iso` or `make iso-noroot` explicitly to create `build/vibecore.iso`
- `make iso-noroot` works WITHOUT root, WITHOUT password, WITHOUT GUI
- `make iso` auto-detects GUI → runs `iso-full` (needs Desktop GUI) or `iso-noroot` (headless)
- `make iso-verify` checks partition table + FAT32 + MBR after build
- `make clean` deletes `build/vibecore.iso` — rebuild after `make clean`!

### ISO Structure (DO NOT change!)
- File: `build/vibecore.iso` (128 MB)
- Format: MBR + FAT32 (**NOT** ISO 9660!)
- MBR: 440 byte boot code + partition table + 0x55AA
- Partition: FAT32, bootable, starts at sector 2048
- Contains: kernel8.img, config.txt, bootcode.bin, start.elf, fixup.dat

---

## 🖥️ QEMU Rules

### QEMU Targets
| Command | Machine | CPU | Display |
|---------|---------|-----|---------|
| `make run` | raspi3b | cortex-a53 | nographic |
| `make run-gui` | raspi3b | cortex-a53 | GTK |
| `make debug` | raspi3b | cortex-a53 | GDB :1234 |

### QEMU Rules
- **NO** direct VM boot! VibeCore needs `-kernel` flag
- **NO** `-M virt` — must be `raspi3b`
- Memory: **1G** minimum
- Always use `-serial stdio -nographic` for console mode
- ISO test: `-kernel kernel8.img -drive file=build/vibecore.iso,if=sd`

---

## 🚫 Absolute Prohibitions

| # | Prohibition | Reason |
|---|-------------|--------|
| 1 | `sudo`/`su` in terminal | Terminal corruption! |
| 2 | Real RPi hardware access | Pi stays untouched |
| 3 | `any`/`void*` casts without reason | Type safety |
| 4 | `git push --force` | Data loss |
| 5 | Committing build artifacts | Respect .gitignore |
| 6 | Ignoring compiler warnings | `-Wall -Wextra -Werror` |
| 7 | Relaxing compiler flags | Security flags are mandatory |
| 8 | `make install` without SD card | For real flashing only |
| 9 | Committing without QEMU test | `make run` must work |
| 10 | Letting docs go stale | ALWAYS keep current |
| 11 | Leaving build artifacts in root | `make clean` after every build, root MUST be clean |
| 12 | Loose scripts or duplicate files in root | Everything in `scripts/`, no duplicate files |

---

## 📁 Project Structure (NEVER change!)

```
Vibe_Core_Labor/
├── README.md              ← Main documentation (EN)
├── ARCHITECTURE.md        ← Architecture docs (EN)
├── TESTING.md             ← Testing guide
├── RULES.md               ← THIS FILE
├── Jules.md               ← Google Jules rules
├── BRAINSTORM.md          ← Roadmap, ideas, vision
├── Makefile               ← Build system (NEVER break!)
├── linker.ld              ← Memory map
├── config.txt             ← RPi boot config
├── .gitignore             ← Build artifacts ignored
├── .github/workflows/     ← CI/CD (build.yml)
├── scripts/               ← ALL scripts (NO loose scripts!)
│   ├── auth-helper.sh     ← GUI auth (zenity/pkexec, NO terminal!)
│   └── mk-bootmbr.py      ← MBR boot code generator
├── src/                   ← Kernel (22 files)
├── include/               ← Headers (22 files)
└── build/                 ← Build output (.gitignored!)
```

---

## 🐛 Bug Fix Protocol

```
1. FIND BUG
   ├── make fanalyzer          ← GCC deep analysis
   ├── make cppcheck           ← Bug & UB scan
   ├── make flawfinder         ← Security scan
   └── thinker-with-files-gemini ← Deep analysis

2. DOCUMENT BUG
   ├── File:Line
   ├── Severity (CRITICAL/HIGH/MEDIUM/LOW)
   └── Description + suggested fix

3. IMPLEMENT FIX
   ├── str_replace (preferred)
   └── Code style 1:1 from existing code

4. VALIDATE
   ├── make clean && make      ← 0 errors
   ├── make run                ← QEMU boot
   ├── make harden-test        ← Security build
   └── code-reviewer-deepseek  ← Review

5. DOCS + COMMIT
   ├── Update README.md Changelog
   ├── Update RULES.md with new rules
   ├── make clean              ← Clean root!
   ├── git status              ← Check: no .o/.d in root
   └── git commit + push
```

---

## 🔒 Security Checklist (before EVERY commit)

- [ ] `make clean && make -j$(nproc)` → **0 errors, 0 warnings**
- [ ] `make check` → Pre-commit check passed
- [ ] `make run` → Boots to shell prompt
- [ ] `make harden-test` → Hardened kernel boots
- [ ] No sensitive data in diff
- [ ] `.gitignore` current
- [ ] **Root clean**: no `.o`, `.d`, `vibecore.iso` in root
- [ ] **No duplicates**: EVERY file exists only ONCE
- [ ] `make cppcheck` → 0 critical issues
- [ ] `make flawfinder` → 0 high-risk findings
- [ ] No `sudo`/`su` in code
- [ ] Stack canary intact (`-fstack-protector-strong`)
- [ ] `-mstrict-align` active
- [ ] README.md Changelog current

---

## 🎓 Naming Conventions

| Type | Convention | Example |
|------|-----------|---------|
| Files | snake_case | `boot_anim.c` |
| Functions | snake_case | `framebuffer_init()` |
| Types/Structs | PascalCase | `FramebufferInfo` |
| Macros/Defines | UPPER_SNAKE | `FB_DEFAULT_WIDTH` |
| Header guards | `_NAME_H` | `_KERNEL_H` |
| Global variables | g_ prefix | `g_fb_info` |
| Static functions | no prefix | `pack32()` |
| Enum values | UPPER_SNAKE | `LOG_LEVEL_INFO` |

## 🏷️ Commit Message Format

```
<Area>: <Short description>

<Details (optional)>

Examples:
  Docs: Updated README + ARCHITECTURE to English
  Build: Added security tools (harden, fanalyzer, audit)
  Fix: mailbox_call response code check
  Kernel: Fixed boot animation alignment fault
```

---

## 📊 Project Metrics (Reference)

| Metric | Value |
|--------|-------|
| Kernel size | 62 KB |
| Source files | 22 (.c + .S) + 22 (.h) |
| Build time | ~2s (parallel) |
| Boot time (QEMU) | ~1.6s |
| Graphics | 1024×768×32 |
| Heap | 1 MB + 64 KB emergency |
| Stack | 128 KB |
| Max tasks | 32 |

---

_Last updated: June 15, 2026 — VibeCore Labs_
