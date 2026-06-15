# VibeCore OS — Testing Guide

## Overview

VibeCore OS testing follows a multi-layer strategy adapted from the embedded systems test pyramid:

```
┌─────────────────────────────────────┐
│  Hardware-in-the-Loop (Raspberry Pi)│  ← Manual, JTAG
├─────────────────────────────────────┤
│  QEMU Emulation (aarch64)           │  ← Automated smoke tests
├─────────────────────────────────────┤
│  Static Analysis (cppcheck, clang)   │  ← CI-ready, fast
├─────────────────────────────────────┤
│  Compilation (-Wall -Wextra -Werror) │  ← Every build
└─────────────────────────────────────┘
```

## Layer 1: Compilation

Every build enforces strict warnings as errors. Supports parallel compilation with resource limits.

```bash
# Standard build
make clean && make

# Parallel build (all CPU cores)
make clean && make -j$(nproc)

# With resource limits (CPU + IO priority)
make -j4 NICE=10 IONICE=-c3
```

**Compiler flags:**
- `-Wall -Wextra -Werror` — All warnings treated as errors
- `-O3` — Maximum optimization (catches more UB at compile time)
- `-fstack-protector-strong` — Stack canary on all functions with arrays
- `-fno-omit-frame-pointer` — Enables stack traces in crash dumps
- `-fno-delete-null-pointer-checks` — Prevents dangerous optimization
- `-mstrict-align` — Traps unaligned access on ARMv8
- `-ffreestanding` — No standard library assumptions

**Expected:** 0 errors, 0 warnings. Kernel size: ~60 KB.

## Layer 2: Static Analysis

### cppcheck — Bug & Undefined Behavior Detection

```bash
make cppcheck
# or directly:
cppcheck --enable=all --inconclusive --std=c11 \
    --suppress=missingIncludeSystem \
    --suppress=unusedFunction \
    -Iinclude src/
```

**What it finds:**
- Null pointer dereferences
- Buffer overflows
- Uninitialized variables
- Memory leaks
- Resource leaks
- Dangerous function usage

### flawfinder — Security Pattern Scanner (CWE/SANS Top 25)

```bash
make flawfinder
# or directly:
flawfinder --minlevel=1 --columns --context src/ include/
```

**What it finds:**
- `strcpy` / `strcat` (buffer overflow risk)
- `sprintf` / `vsprintf` (format string risk)
- Missing bounds checks
- Dangerous API usage matching CWE patterns

### clang-tidy — Code Quality & Modernization

```bash
make clang-tidy
# or directly:
clang-tidy -p . --checks='*,-clang-analyzer-alpha.*' \
    src/*.c -- -Iinclude -nostdlib -ffreestanding
```

**What it finds:**
- CERT C/C++ coding standard violations
- C++ Core Guidelines violations (C equivalents)
- Potential performance issues
- Readability improvements

### Combined Analysis

```bash
make analyze   # runs cppcheck + flawfinder
```

### Tools Reference Table

| Tool | Focus | Speed | Output |
|------|-------|-------|--------|
| **cppcheck** | Bugs, undefined behavior | Medium | Text |
| **flawfinder** | Security (CWE patterns) | Fast | Text |
| **clang-tidy** | Style, modernization | Slow | Text |
| **Clang SA** | Deep logic errors | Slow | HTML |
| **PVS-Studio** | Complex semantic bugs | Medium | HTML/IDE |
| **Coverity** | Safety/compliance | Very slow | Web UI |
| **Semgrep** | Custom pattern rules | Fast | JSON/Text |

### About "Fallow"

**Fallow** (`github.com/fallow-rs/fallow`) is a codebase intelligence engine for **TypeScript/JavaScript** projects — it is NOT a C/C++ tool. It runs via npm:
```bash
npm install --save-dev fallow
npx fallow health    # Codebase health score
npx fallow unused    # Dead code detection
npx fallow deps      # Circular dependency analysis
npx fallow security  # Security pattern detection
```

For C/embedded projects, the equivalent capabilities are covered by:

| Fallow Feature | C/Embedded Equivalent |
|----------------|----------------------|
| Dead code detection | `cppcheck --enable=all` (unusedFunction), linker `--gc-sections` |
| Architecture analysis | `clang-tidy` + custom scripts |
| Complexity tracking | `lizard` (cyclomatic complexity), `pmccabe` |
| Duplication detection | `simian`, `duplo` |
| Security patterns | `flawfinder`, `semgrep` with C rules |
| PR risk analysis | `clang-tidy-diff`, `git diff | cppcheck` |

## Layer 3: QEMU Smoke Test

```bash
make run
# or manually:
qemu-system-aarch64 -M raspi3b -cpu cortex-a53 -m 1G \
    -kernel kernel8.img -serial stdio -nographic

# With graphics (for GUI/recovery screens):
make run-gui
```

**Test checklist (manual):**

| Command | Expected Output |
|---------|----------------|
| (boot) | Boot animation → Welcome screen (ENTER to continue) → klog init → Shell prompt |
| `help` | All 17+ commands listed |
| `version` | `VibeCore OS 1.0.0 "Photon" (aarch64)` |
| `log` | Kernel boot messages with timestamps and log levels |
| `fsck` | Filesystem integrity check — SYSTEM CLEAN |
| `protect` | List of immutable system files |
| `snapshot` | Snapshot created with incrementing counter |
| `trash` | Recycle bin listing (empty, RAM-disk) |
| `versions` | Version listing (empty, RAM-disk) |
| `mem` | Heap stats (1024 KB, 0% used), Stack-Canary: OK |
| `info` | System info (OS, arch, CPU, RAM, uptime) |
| `recovery` | Graphical recovery screen (requires `-display gtk`) |
| `crash` | Crash screen with ESR/ELR/FAR dump (halts system) |

### Automated QEMU Test (KTAP-style)

Create a test script that pipes commands and greps output:

```bash
#!/bin/bash
# test_smoke.sh — VibeCore OS smoke test
echo -e "\n\n\n\nhelp\nversion\nfsck\nmem\n" | \
    timeout 15 qemu-system-aarch64 \
        -M raspi3b -cpu cortex-a53 -m 1G \
        -kernel kernel8.img -serial stdio -nographic \
        -monitor none 2>&1 | tee qemu_output.log

# Verify key outputs
grep -q "VibeCore OS 1.0.0" qemu_output.log && echo "[PASS] Version" || echo "[FAIL] Version"
grep -q "SYSTEM CLEAN" qemu_output.log && echo "[PASS] FSCK" || echo "[FAIL] FSCK"
grep -q "OK" qemu_output.log && echo "[PASS] Memory" || echo "[FAIL] Memory"
```

## Layer 4: Hardware-in-the-Loop (Raspberry Pi)

```bash
# 1. Create bootable disk image (128MB FAT32)
make iso       # Auto: GUI→iso-full, headless→iso-noroot

# 2. Flash to SD card (triggers Desktop-GUI password popup)
make iso-flash SDCARD=/dev/mmcblk0

# 3. Or flash manually
dd if=build/vibecore.iso of=/dev/mmcblk0 bs=4M status=progress

# 4. Insert SD card into Raspberry Pi 3B and power on
```

**Hardware-specific tests:**
- UART output via GPIO pins 14/15 (TX/RX)
- HDMI framebuffer output (1024×768 or native resolution)
- Framebuffer mailbox communication
- No USB keyboard initially (UART input only)
- WFI-based polling saves battery vs busy-spin

## Known Limitations

1. **No USB keyboard driver** — All input is via UART serial. Recovery and welcome screens use `uart_getc()` with WFI-based polling for UART input. A DWC2 USB HID driver is needed for standalone Pi usage.

2. **FAT32 is RAM-disk stub** — No SD card driver (EMMC) yet. All filesystem operations are RAM-only. Crash logs and config files are not persistent across reboots.

3. **MMU is disabled** — 1GB block mappings cache MMIO addresses, breaking UART. Needs 2MB L2 table granularity for Device/MMIO separation.

4. **No watchdog timer** — Reboot command halts in WFI loop instead of triggering a hardware reset.

## Parallel Build & Resource Limits

```bash
# Build with all CPU cores (default: 10% CPU priority via nice)
make -j$(nproc)

# Customize resource limits
make -j4 NICE=5 IONICE=-c2

# With auth helper (for install/iso targets)
make install    # First time: GUI password popup via zenity/pkexec
# ... cached for 5 minutes, subsequent commands don't re-prompt
```

## Auth Helper Testing

The `scripts/auth-helper.sh` script:
- **NUR Desktop-GUI**: zenity → pkexec (KEIN Terminal-Fallback!)
- Shows UAC-style password dialog
- Caches auth for 5 minutes (AUTH_VALID_SECONDS=300)
- Fails gracefully if no GUI available

Test manually:
```bash
# Test zenity GUI popup (requires desktop session)
./scripts/auth-helper.sh echo "Auth works!"
```

## CI/CD Pipeline (GitHub Actions)

**Implemented** at `.github/workflows/build.yml` — runs on every push & PR:

| Job | Steps |
|-----|-------|
| **build** | `apt install gcc-aarch64-linux-gnu` → `make -j$(nproc)` → upload `kernel8.img` |
| **analyze** | `apt install cppcheck` → `cppcheck --enable=all src/` |
| **test** | Download artifact → QEMU smoke test → ISO build → FAT32 verify |

Status: [![Build & Test](https://github.com/JONIMONI09/VibeCoreOS/actions/workflows/build.yml/badge.svg)](https://github.com/JONIMONI09/VibeCoreOS/actions/workflows/build.yml)

## Quick Reference

```bash
# Run everything
make clean && make -j$(nproc)     # Parallel compile
make analyze                        # cppcheck + flawfinder
make clang-tidy                     # clang-tidy linting
make run                            # QEMU smoke test

# Individual tools
cppcheck --enable=all -Iinclude src/
flawfinder --minlevel=1 src/ include/
clang-tidy src/*.c -- -Iinclude -nostdlib -ffreestanding

# Build ISO for hardware
make iso                            # build/vibecore.iso (128 MB)
```
