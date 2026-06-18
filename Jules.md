# VibeCore OS — Jules AI Agent Rules (Google)

## 🤖 For Google Jules (AI Coding Agent)

This file contains the specific rules and instructions for **Google's Jules AI Coding Agent**
to work correctly, safely, and efficiently on the VibeCore OS project.

---

## 📋 Project Fact Sheet

| Field | Value |
|-------|-------|
| **Name** | VibeCore OS 1.0.0 "Photon" |
| **Type** | Bare-metal ARM64 operating system |
| **Target platform** | Raspberry Pi 3B (BCM2837) / 4B (BCM2711) |
| **Language** | C (GCC) + ARMv8 Assembly |
| **Kernel size** | ~62 KB |
| **Build system** | GNU Make (parallel, `make -j$(nproc)`) |
| **Compiler** | `aarch64-linux-gnu-gcc` |
| **Emulator** | QEMU `raspi3b`, Cortex-A53 |
| **Static analysis** | cppcheck v2.17.1 + flawfinder v2.0.20 + GCC fanalyzer |
| **CI/CD** | GitHub Actions `.github/workflows/build.yml` |
| **Repo** | 🔒 Private: `github.com/JONIMONI09/VibeCoreOS` |

---

## 🎯 Jules' Tasks & Goals

### Primary Goals
1. **Sandbox setup first**: Install cppcheck, flawfinder, qemu-system-aarch64 before coding
2. **Find & fix bugs**: Use ALL analysis tools (fanalyzer, cppcheck, flawfinder)
3. **Implement features**: Only after consultation — no wild refactoring
4. **Security audits**: `make audit` — cppcheck + flawfinder + fanalyzer
5. **Always test OS**: `make run` (QEMU) + `make check` (analysis) after EVERY change
6. **Maintain documentation**: README.md, ARCHITECTURE.md, TESTING.md, RULES.md, Jules.md, BRAINSTORM.md
7. **Monitor CI/CD**: Check GitHub Actions build status
8. **Maintain build system**: Keep Makefile targets current

### AI Agent Sandbox Rules
- **Install ALL tools FIRST** at session start (before touching any code!)
- **Test EARLY, test OFTEN** — run `make run` after EVERY significant code change
- **3-file rule** — if >5 files need changes, write a plan first
- **No assumptions** — always verify library availability, never assume headers exist
- **Full audit minimum once** — `make audit` at least once per session

### On EVERY Task
- Read `ARCHITECTURE.md` + `README.md` first
- Run `make clean && make -j$(nproc)` as baseline
- Run `make check` as pre-commit check
- Run `make run` QEMU boot test
- Run `make iso-noroot` (verify ISO builds without root)
- Get a code review
- Update documentation
- **`make clean` + `git status` → root MUST be clean!**

---

## 🚫 Absolute Prohibitions (12 Rules)

| # | Prohibition | Reason |
|---|-------------|--------|
| 1 | **`sudo`/`su` in terminal** | Terminal corruption! Auth via `auth-helper.sh` only (GUI) |
| 2 | **Real Raspberry Pi access** | Pi stays untouched — 100% QEMU |
| 3 | **`any`/`void*` casts without reason** | Type safety is critical in C |
| 4 | **`git push --force`** | Risk of data loss |
| 5 | **Committing build artifacts** | `.gitignore` MUST be respected |
| 6 | **Ignoring compiler warnings** | `-Wall -Wextra -Werror` is mandatory |
| 7 | **Relaxing compiler flags** | Security flags are non-negotiable |
| 8 | **`make install` without SD card** | For real hardware flashing only |
| 9 | **Committing without QEMU test** | `make run` MUST reach shell prompt |
| 10 | **Letting docs go stale** | ALWAYS keep current! |
| 11 | **Leaving build artifacts in root** | `make clean` after every build. Root MUST be clean! |
| 12 | **Loose files or duplicates** | Everything has its place — scripts in `scripts/`, no duplicate docs |

---

## 📋 Complete Task Workflow (5 Phases)

### Phase 0: SANDBOX SETUP (first 2 minutes) — DO THIS FIRST!
```
□ Check tools: which cppcheck flawfinder qemu-system-aarch64 aarch64-linux-gnu-gcc
□ Install tools:
   pip3 install --break-system-packages --user flawfinder          # Python tool, no sudo
   sudo apt install -y cppcheck qemu-system-arm                    # via auth-helper.sh GUI
□ Verify toolchain:
   aarch64-linux-gnu-gcc --version && cppcheck --version && flawfinder --version
□ Build baseline:
   make clean && make -j$(nproc)
   make run -- QEMU smoke test (timeout 15s, verify shell prompt)
□ If sudo unavailable: pip3 install --user for Python tools, skip sudo-only tests, note limitations
```

### Phase 1: CONTEXT (30% of time)
```
□ Read ARCHITECTURE.md + README.md
□ Study relevant src/*.c files
□ Check relevant include/*.h headers
□ Analyze existing similar code
□ Understand code style, naming patterns, structure
□ Check linker.ld memory map (for memory questions)
```

### Phase 2: ANALYSIS (20% of time)
```
□ make clean && make -j$(nproc)         ← Baseline build (0 errors?)
□ make check                             ← Quick check
□ make fanalyzer                         ← GCC deep analysis
□ make cppcheck                          ← Bug & UB detection
□ make flawfinder                        ← CWE/SANS security scan
□ make audit                             ← Full audit (for big changes)
□ thinker-with-files-gemini              ← Complex problems
```

### Phase 3: IMPLEMENTATION (30% of time)
```
□ write_todos                             ← Planning for 3+ steps
□ str_replace for changes                 ← PREFERRED (precise)
□ write_file ONLY for new files          ← Full file new
□ Code style 1:1 from existing code      ← NO deviations!
□ Update all references                  ← spawn code-searcher
□ No dead imports, no unused variables
□ No magic numbers                       ← Use defines/const
```

### Phase 4: VALIDATION (15% of time)
```
□ make clean && make -j$(nproc)          ← MUST have 0 errors, 0 warnings
□ make check                             ← Pre-commit passed?
□ make run                               ← QEMU boot (shell reachable?)
□ make harden-test                       ← Hardened kernel boots?
□ code-reviewer-deepseek-flash           ← Code review PARALLEL
□ Fix all review findings
```

### Phase 5: DOCUMENTATION + COMMIT (5% of time)
```
□ Update README.md Changelog
□ Update ARCHITECTURE.md (on architecture changes)
□ Update TESTING.md (on new test methods)
□ Update RULES.md (on new rules/tools)
□ Update Jules.md (on new Jules rules)
□ Update BRAINSTORM.md (on new ideas/roadmap)
□ Update make help text (on new targets)
□ make clean              ← Clean root!
□ git status              ← Check: no .o/.d in root!
□ git add -A
□ git commit -m "Area: Description"
□ git push origin master  ← NEVER --force!
```

---

## 🔒 Security & Analysis — All Tools

### Build
```bash
make -j$(nproc)       # Normal build (~2s)
make harden           # Hardened build (stack-clash-protection)
make harden-test      # Hardened build + QEMU boot test
make check            # Pre-commit: compile + cppcheck
```

### Security
```bash
make fanalyzer        # GCC deep analysis (use-after-free, overflow, NULL)
make flawfinder       # CWE/SANS Top 25 security patterns (v2.0.20)
make security         # Security audit summary
```

### Logic
```bash
make cppcheck         # Bug & undefined behavior detection (v2.17.1)
make clang-tidy       # Code quality & CERT compliance
make clang-analyzer   # Clang static analyzer (deep logic)
make logic            # Logic check summary
```

### Full Audit
```bash
make audit            # cppcheck + flawfinder + fanalyzer (~30s)
make analyze          # cppcheck + flawfinder (~10s)
```

---

## 💿 SD Image System

```bash
make iso              # Auto: GUI=full, headless=noroot
make iso-noroot       # Rootless SD image (MBR+FAT32+boot files, NO root)
make iso-full         # Full image (needs desktop GUI)
make iso-verify       # Check partition + FAT32
make iso-test         # Test ISO in QEMU
make firmware         # Download RPi firmware
```

**ISO structure**: `build/vibecore-rpi.img` (128 MB, MBR+FAT32, NOT ISO 9660!)

**Critical**: ISO is NOT built automatically! Run `make iso` or `make iso-noroot` explicitly.
`make iso-noroot` needs NO root, NO password, NO GUI.

---

## 🖥️ QEMU (ONLY way to test!)

```bash
make run              # Terminal mode (raspi3b, nographic)
make run-gui          # Graphics mode (GTK display)
make debug            # GDB debug server (:1234)
```

**Never** VM direct boot! UEFI limited to diagnostic stub, no BIOS — only QEMU `-kernel` flag.

---

## 📁 Important Files

| File | Purpose | On Change |
|------|---------|-----------|
| `src/kernel.c` | Main init (14 phases) | Update ARCHITECTURE.md |
| `src/boot.S` | ARMv8 entry + vectors | Extreme caution! |
| `src/framebuffer.c` | GPU framebuffer (128-bit STP) | Test with QEMU run-gui |
| `src/mailbox.c` | ARM↔GPU mailbox | Test with `make run` |
| `src/interrupt.c` | Exception dispatch | Check crash screen |
| `src/scheduler.c` | BORE scheduler | Test with `make run` |
| `src/recovery.c` | 8-subsystem recovery | FSCK test |
| `src/gui.c` | Desktop | Test with `make run-gui` |
| `src/shell.c` | Shell (18 commands) | Test with `make run` |
| `include/peripherals.h` | BCM2837 MMIO | Careful! |
| `include/types.h` | Types + macros | Check all sources |
| `linker.ld` | Memory layout | Check stack/heap |
| `Makefile` | Build system | Update `make help` |
| `scripts/auth-helper.sh` | GUI auth | NO terminal fallback! |
| `scripts/mk-bootmbr.py` | MBR generator | Test with `make iso-verify` |
| `BRAINSTORM.md` | Roadmap & ideas | On new features/goals |
| `.github/workflows/build.yml` | CI/CD | Check GH Actions |

---

## 🐛 Known Bugs & Issues

| # | Bug | File | Severity |
|---|-----|------|----------|
| 1 | MMU disabled (MMIO caching) | `mmu.c`, `kernel.c` | HIGH |
| 2 | No EMMC/SD driver | `fs.c` | HIGH |
| 3 | No USB driver | — | MEDIUM |
| 4 | UEFI boot limited to diagnostic stub (no kernel loading yet) | `src/bootloader/uefi/` | MEDIUM |
| 5 | `va_arg` type mismatch (`snprintf_local`) | `interrupt.c` | MEDIUM |
| 6 | Missing prototypes | `boot_anim.c`, `crashlog.c` | LOW |
| 7 | Precision loss gradient | `boot_anim.c` | LOW |
| 8 | Sign change in crashlog | `crashlog.c` | LOW |

### Recently Fixed (2026-06-15)
| Bug | Fix |
|-----|-----|
| `mailbox_call` wrong return check | `buffer[1] == MBOX_RESPONSE` |
| GPU address wrong slot | `buf[23]` (base) + `buf[24]` (size) |
| `framebuffer_fillrect` alignment fault | `IS_ALIGNED(buf,8)` + 32-bit fallback |
| SD image had misleading `.iso` extension | Renamed to `build/vibecore-rpi.img` (raw SD image, NOT ISO 9660) |
| MBR without boot code → VM "not bootable" | `scripts/mk-bootmbr.py` (440-byte MBR) |
| IRQ infinite loop | Write-1-to-clear + dmb |
| `snprintf_local` buffer overflow | `max==0` guard |
| Auth-helper terminal fallback | Desktop GUI only (zenity/pkexec) |

---

## 🔒 Security Checklist (before EVERY commit)

```
□ make clean && make -j$(nproc)    → 0 errors, 0 warnings
□ make check                       → Pre-commit passed
□ make run                         → Boots to shell prompt
□ make harden-test                 → Hardened kernel boots
□ No sensitive data in diff
□ .gitignore current
□ Root clean: no .o, .d, duplicates in root!
□ make cppcheck (0 critical issues)
□ make flawfinder (0 high-risk findings)
□ No sudo/su in code
□ Stack canary intact (-fstack-protector-strong)
□ -mstrict-align active
□ -fstack-clash-protection (on harden)
□ README.md Changelog current
```

---

## 📊 Reference Values

| Metric | Target | Tolerance |
|--------|--------|-----------|
| Kernel size | 62 KB | ±5 KB |
| Build time | ~2s | <5s |
| Boot time (QEMU) | ~1.6s | <3s |
| Heap | 1 MB | — |
| Stack | 128 KB | — |
| Compile warnings | 0 | **0** |
| Fanalyzer warnings | <5 | Informational |
| Cppcheck issues | 0 | <5 informational |

---

_Jules Rules v3.0 — June 15, 2026 — VibeCore Labs_

## 🛡️ Extended AI Directives (Emulation, Fallbacks & Strict Analysis)

### 1. Hardware Fallbacks & Emulation Compatibility
- **No Hardware Lock-in:** The OS MUST NOT depend on proprietary NVIDIA, AMD, or specific modern hardware accelerations.
- **Software Rendering:** Ensure graphics and essential subsystems have robust software fallbacks so they can run smoothly in pure emulation modes (like QEMU without KVM, or old hardware emulators).
- **Userspace Protection:** The boundary between kernel and userspace MUST NOT be violated. Do not delete or compromise any userspace code or structures.

### 2. Zero-Tolerance Analysis (No Muting)
- **Zero Suppressions:** The use of `// flawfinder: ignore`, `/* flawfinder: ignore */`, and `cppcheck-suppress` is **STRICTLY FORBIDDEN**.
- **Real Fixes Only:** All static analysis issues found by `make cppcheck`, `make flawfinder`, and `make fanalyzer` must be fundamentally resolved by changing the logic or ensuring safe constraints, rather than silencing the warnings.
- **CI/CD Integrity:** Do NOT edit the `.github/workflows` YAML files to mute errors or alter the strictness of the tests. The code must pass locally with 0 warnings before pushing.
