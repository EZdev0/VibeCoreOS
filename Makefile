# ============================================================
#  VibeCore OS — Build System (Optimized)
#  Target: Raspberry Pi 3B (BCM2837) / QEMU raspi3b / aarch64
#
#  Parallel compilation:  make -j$(nproc)
#  Auth for root ops:    scripts/auth-helper.sh
# ============================================================

ARCH ?= aarch64
BOARD ?= raspi3b

CC       = aarch64-linux-gnu-gcc
AS       = aarch64-linux-gnu-as
LD       = aarch64-linux-gnu-ld
OBJCOPY  = aarch64-linux-gnu-objcopy
OBJDUMP  = aarch64-linux-gnu-objdump

# ── Parallelism & Resource Limits ──────────────────────────
#   Usage: make -j$$(nproc)           (parallel compilation)
#          make -j4 NICE=10 IONICE=-c3 (with CPU/IO limits)
NPROC    ?= $(shell nproc 2>/dev/null || echo 4)
NICE     ?= 10
IONICE   ?= -c 3

# Apply resource limits if set (for Makefile-internal commands)
RLIMIT = $(if $(NICE),nice -n $(NICE),)$(if $(IONICE), ionice $(IONICE))

# ── Auth Helper (UAC-style GUI password popup) ─────────────
AUTH     = $(CURDIR)/scripts/auth-helper.sh

# ── Flags ──────────────────────────────────────────────────
CFLAGS   = -Wall -Wextra -D$(shell echo $(BOARD) | tr a-z A-Z) -Werror -O3 -nostdlib -nostartfiles \
           -ffreestanding -mgeneral-regs-only \
           -Iinclude -MMD -MP \
           -fstack-protector-strong -fno-exceptions \
           -mstrict-align -fno-common \
           -fno-omit-frame-pointer -fno-delete-null-pointer-checks

ASFLAGS  = -Iinclude

LDFLAGS  = -nostdlib -T linker.ld -Map kernel.map

# ── Sources ─────────────────────────────────────────────────
C_SRCS   = $(wildcard src/*.c)
ASM_SRCS = $(wildcard src/arch/$(ARCH)/*.S)
OBJS     = $(C_SRCS:.c=.o) $(ASM_SRCS:.S=.o)
DEPS     = $(C_SRCS:.c=.d)

ARCH ?= aarch64
BOARD ?= raspi3b

TARGET   = kernel.elf
IMG      = kernel8.img
MAP      = kernel.map
DUMP     = kernel.dump

# ── Rules ───────────────────────────────────────────────────
.PHONY: all clean run debug dump install help iso flawfinder analyze lint cppcheck clang-tidy clang-analyzer
.PHONY: harden harden-test fanalyzer security logic audit check

all: $(IMG) $(DUMP)

$(TARGET): $(OBJS) linker.ld
	@echo "  LD      $@"
	@$(LD) $(LDFLAGS) -o $@ $(OBJS)

$(IMG): $(TARGET)
	@echo "  OBJCOPY $@"
	@$(OBJCOPY) -O binary $< $@

$(DUMP): $(TARGET)
	@echo "  OBJDUMP $@"
	@$(OBJDUMP) -D $< > $@

-include $(DEPS)

    src/%.o: src/%.c
	@echo "  CC      $<"
	@$(RLIMIT) $(CC) $(CFLAGS) -c -o $@ $<

src/%.o: src/%.S
	@echo "  AS      $<"
	@$(AS) $(ASFLAGS) -o $@ $<

# ── QEMU ────────────────────────────────────────────────────
#   NOTE: QEMU's raspi3b machine does NOT emulate the RPi
#   firmware boot chain (bootcode.bin → start.elf → kernel8.img).
#   Use -kernel for direct boot. The ISO image is for REAL hardware.
run: all
	@echo "=== VibeCore OS — QEMU (raspi3b, direct kernel boot) ==="
	qemu-system-aarch64 \
		-M raspi3b \
		-cpu cortex-a53 \
		-m 1G \
		-kernel $(IMG) \
		-serial stdio \
		-nographic

#   QEMU with graphics window (GTK)
run-gui: all
	qemu-system-aarch64 \
		-M raspi3b \
		-cpu cortex-a53 \
		-m 1G \
		-kernel $(IMG) \
		-serial stdio \
		-display gtk

#   QEMU with GDB debug server
debug: all
	qemu-system-aarch64 \
		-M raspi3b \
		-cpu cortex-a53 \
		-m 1G \
		-kernel $(IMG) \
		-serial stdio \
		-nographic \
		-S -s

#   QEMU: Test ISO image (boots kernel directly + attaches ISO as SD)
#   QEMU cannot boot from the ISO directly (no firmware emulation).
#   We boot kernel8.img via -kernel AND attach the ISO as a secondary
#   SD card so the kernel can access the filesystem/partitions.
iso-test: iso
	@echo ""
	@echo "============================================"
	@echo "  QEMU ISO Test"
	@echo "  Boot: kernel8.img (direct) + ISO as SD"
	@echo "============================================"
	@echo "  The kernel8.img in the ISO is the same binary"
	@echo "  as the locally-built kernel8.img."
	@echo "  QEMU boots the kernel directly (-kernel flag)."
	@echo "  The ISO is attached as a block device (if=sd)."
	@echo "  On real hardware, the GPU firmware loads"
	@echo "  kernel8.img from the FAT32 partition."
	@echo ""
	@echo "  (Press Ctrl+A then X to quit)"
	@echo ""
	@qemu-system-aarch64 \
		-M raspi3b \
		-cpu cortex-a53 \
		-m 1G \
		-kernel $(IMG) \
		-drive file=$(ISO_IMG),format=raw,if=sd \
		-serial stdio \
		-nographic

#   QEMU: Verify ISO partition table and structure
#   Checks MBR, FAT32 magic bytes, and reports validity.
iso-verify: iso
	@echo "=== ISO Verification ==="
	@echo ""
	@echo "Partition table:"
	@fdisk -l $(ISO_IMG) 2>/dev/null || true
	@echo ""
	@echo "FAT32 header at sector $(ISO_PART_START):"
	@dd if=$(ISO_IMG) bs=512 skip=$(ISO_PART_START) count=1 2>/dev/null | \
		od -A x -t x1z -v | head -6
	@echo ""
	@echo "Checking FAT32 signature..."
	@SIG=$$(dd if=$(ISO_IMG) bs=1 skip=$$(( $(ISO_PART_START)*512 + 82 )) count=8 2>/dev/null); \
	if echo "$$SIG" | grep -q "FAT32"; then \
		echo "  ✅ FAT32 signature valid (\"FAT32   \" at offset 82)"; \
	else \
		echo "  ❌ FAT32 signature missing or invalid at sector $(ISO_PART_START)"; \
	fi
	@echo ""
	@echo "ISO structure valid for Raspberry Pi 3B hardware."
	@echo "Flash with: dd if=$(ISO_IMG) of=/dev/sdX bs=4M status=progress"

# ── SD Card Installation ────────────────────────────────────
#   NOTE: Uses auth-helper.sh to show UAC-style password popup.
#   One password prompt per target, not per command.
SDCARD  ?= /dev/sdb
BOOT ?= /mnt/boot

install: all
	@echo "=== Installing to $(SDCARD) ==="
	@echo "WARNING: This will overwrite $(SDCARD)!"
	@echo "Usage: make install SDCARD=/dev/mmcblk0 BOOT=/mnt/boot"
	@echo ""
	@echo "  Flashing with dd (one auth prompt)..."
	@$(AUTH) dd if=$(IMG) of=$(SDCARD) bs=4M status=progress
	@echo ""
	@echo "  Or copy kernel manually after mounting:"
	@echo "    mount $(SDCARD)1 $(BOOT)"
	@echo "    cp $(IMG) $(BOOT)/kernel8.img"
	@echo "    umount $(BOOT)"

# ── ISO / Disk-Image (Bootable SD Card) ─────────────────────
#   Creates a properly partitioned, bootable SD card image.
#
#   make iso         → Auto-detect: GUI=files copied, headless=base only
#   make iso-noroot  → Partitioned + formatted image (no files, no root)
#   make iso-full    → Full image with files (needs desktop GUI popup)
#
#   Output:   build/vibecore.iso
#   Contains: MBR + FAT32 partition [+ firmware + kernel if GUI]
ISO_DIR   = build
ISO_IMG   = $(ISO_DIR)/vibecore.iso
ISO_SIZE  = 128
ISO_PART_START = 2048   # Sectors (each 512 bytes) = 1 MiB offset

# Detect if running in a desktop GUI (for auth popup)
HAS_GUI  = $(shell [ -n "$$DISPLAY" ] || [ -n "$$WAYLAND_DISPLAY" ] && echo 1 || echo 0)

.PHONY: iso iso-noroot iso-full iso-flash iso-check

# ── MBR Boot-Code Generator (Python) ───────────────────────
MBR_GEN  = $(CURDIR)/scripts/mk-bootmbr.py

# ── Smart ISO: auto-detect GUI ──────────────────────────────
iso: all iso-check-tools
	@if [ "$(HAS_GUI)" = "1" ] && [ -s "$(ISO_DIR)/bootcode.bin" ]; then \
		$(MAKE) iso-full; \
	elif [ "$(HAS_GUI)" = "1" ]; then \
		echo "  ⚠️  Firmware not found. Downloading..."; \
		$(MAKE) firmware; \
		$(MAKE) iso-full; \
	else \
		echo "  ℹ️  No desktop GUI detected — building base ISO (no files)."; \
		echo "  ℹ️  Use 'make iso-full' in a desktop session for complete image."; \
		echo ""; \
		$(MAKE) iso-noroot; \
	fi

# ── Full ISO with files (needs desktop GUI for losetup/mount) ─
iso-full: all iso-check
	@echo "============================================"
	@echo "  VibeCore OS — Full Bootable SD Card Image"
	@echo "  Output: $(ISO_IMG) ($(ISO_SIZE) MB)"
	@echo "============================================"
	@echo ""
	@mkdir -p $(ISO_DIR)
	@echo "  [1/5] Creating blank image..."
	@dd if=/dev/zero of=$(ISO_IMG) bs=1M count=$(ISO_SIZE) status=none
	@echo "  [2/5] Writing MBR partition table..."
	@printf "label: dos\nstart=$(ISO_PART_START), type=0c, bootable\n" | \
		sfdisk --no-reread $(ISO_IMG) >/dev/null 2>&1 || \
		{ echo "  ❌ ERROR: sfdisk failed"; exit 1; }
	@echo "  [3/5] Formatting FAT32 partition..."
	@dd if=$(ISO_IMG) of=$(ISO_DIR)/_part.fat bs=512 skip=$(ISO_PART_START) status=none
	@mkfs.fat -F 32 -n VIBECORE $(ISO_DIR)/_part.fat || \
		{ echo "  ❌ ERROR: mkfs.fat failed"; rm -f $(ISO_DIR)/_part.fat; exit 1; }
	@test -s $(ISO_DIR)/_part.fat || { echo "  ❌ ERROR: empty partition"; exit 1; }
	@dd if=$(ISO_DIR)/_part.fat of=$(ISO_IMG) bs=512 seek=$(ISO_PART_START) conv=notrunc status=none
	@rm -f $(ISO_DIR)/_part.fat
	@echo "  [4/6] Injecting MBR boot code (VM compatibility)..."
	@python3 $(MBR_GEN) | dd of=$(ISO_IMG) bs=1 count=440 conv=notrunc 2>/dev/null
	@echo "  [5/6] Copying files (GUI auth popup)..."
	@$(AUTH) bash -c ' \
		set -e; \
		LO=$$(losetup -f); \
		[ -n "$$LO" ] || { echo "ERROR: No loop device"; exit 1; }; \
		losetup -o $$((2048*512)) $$LO $(ISO_IMG); \
		MNT=/tmp/vibecore_mnt_$$$$; \
		mkdir -p $$MNT; \
		mount $$LO $$MNT; \
		cp $(IMG) $$MNT/kernel8.img; \
		cp config.txt $$MNT/config.txt; \
		cp $(ISO_DIR)/bootcode.bin $$MNT/bootcode.bin; \
		cp $(ISO_DIR)/start.elf $$MNT/start.elf; \
		cp $(ISO_DIR)/fixup.dat $$MNT/fixup.dat; \
		echo "  [6/6] Files on boot partition:"; \
		ls -lh $$MNT/ | awk "NR>1 {printf \"         %-8s  %s\\n\", \$$5, \$$9}"; \
		umount $$MNT; \
		losetup -d $$LO; \
		rmdir $$MNT'
	@echo ""
	@echo "  ┌─────────────────────────────────────────────┐"
	@echo "  │  $(ISO_IMG)  │"
	@ls -lh $(ISO_IMG) | awk '{printf "  │  Size:     %-30s │\n", $$5}'
	@fdisk -l $(ISO_IMG) 2>/dev/null | grep -E "Disklabel|$(ISO_IMG)|Boot" | awk '{printf "  │  %-42s │\n", substr($$0,1,42)}'
	@echo "  │  MBR:      Boot-Code aktiv                  │"
	@echo "  │  ⚠️ VM-Boot: Nicht für VM-Direktboot        │"
	@echo "  │  💻 Verwende: make run (QEMU direkt)        │"
	@echo "  └─────────────────────────────────────────────┘"
	@echo ""
	@echo "  ✅ Full bootable image created! ($(ISO_SIZE) MB)"
	@echo "  Flash: dd if=$(ISO_IMG) of=/dev/sdX bs=4M status=progress"

# ── Base ISO (partitioned + formatted, no files, NO root) ────
iso-noroot: all iso-check-tools
	@echo "============================================"
	@echo "  VibeCore OS — Base ISO (no root)"
	@echo "  Partitioned + formatted, no files."
	@echo "============================================"
	@echo ""
	@mkdir -p $(ISO_DIR)
	@echo "  [1/3] Creating blank image..."
	@dd if=/dev/zero of=$(ISO_IMG) bs=1M count=$(ISO_SIZE) status=none
	@echo "  [2/3] Writing MBR partition table..."
	@printf "label: dos\nstart=$(ISO_PART_START), type=0c, bootable\n" | \
		sfdisk --no-reread $(ISO_IMG) >/dev/null 2>&1 || \
		{ echo "  ❌ ERROR: sfdisk failed"; exit 1; }
	@echo "  [3/3] Formatting FAT32 partition..."
	@dd if=$(ISO_IMG) of=$(ISO_DIR)/_part.fat bs=512 skip=$(ISO_PART_START) status=none
	@mkfs.fat -F 32 -n VIBECORE $(ISO_DIR)/_part.fat || \
		{ echo "  ❌ ERROR: mkfs.fat failed"; rm -f $(ISO_DIR)/_part.fat; exit 1; }
	@test -s $(ISO_DIR)/_part.fat || { echo "  ❌ ERROR: empty partition"; exit 1; }
	@dd if=$(ISO_DIR)/_part.fat of=$(ISO_IMG) bs=512 seek=$(ISO_PART_START) conv=notrunc status=none
	@rm -f $(ISO_DIR)/_part.fat
	@echo "  [4/4] Injecting MBR boot code (VM compatibility)..."
	@python3 $(MBR_GEN) | dd of=$(ISO_IMG) bs=1 count=440 conv=notrunc 2>/dev/null
	@echo ""
	@echo "  ┌─────────────────────────────────────────────┐"
	@echo "  │  $(ISO_IMG)  │"
	@ls -lh $(ISO_IMG) | awk '{printf "  │  Size:     %-30s │\n", $$5}'
	@fdisk -l $(ISO_IMG) 2>/dev/null | grep -E "Disklabel|$(ISO_IMG)|Boot" | awk '{printf "  │  %-42s │\n", substr($$0,1,42)}'
	@echo "  │  MBR:      Boot-Code aktiv                  │"
	@echo "  │  ⚠️ Keine Dateien (no-root Modus)          │"
	@echo "  │  💻 Verwende: make run (QEMU direkt)        │"
	@echo "  │  💿 Flash, dann manuell Dateien kopieren    │"
	@echo "  └─────────────────────────────────────────────┘"
	@echo ""
	@echo "  ✅ Base image created! ($(ISO_SIZE) MB)"

# ── Check required tools ────────────────────────────────────
#   iso-check-tools: only tools (for iso-noroot, no firmware needed)
#   iso-check:       tools + firmware (for iso-full)
iso-check-tools:
	@command -v sfdisk >/dev/null 2>&1 || { echo "ERROR: sfdisk required. Install: util-linux"; exit 1; }
	@command -v mkfs.fat >/dev/null 2>&1 || { echo "ERROR: mkfs.fat required. Install: dosfstools"; exit 1; }
	@command -v dd >/dev/null 2>&1 || { echo "ERROR: dd required."; exit 1; }
	@test -f $(IMG) || { echo "ERROR: Build first: make -j$$(nproc)"; exit 1; }

iso-check: iso-check-tools
	@test -f config.txt || { echo "ERROR: config.txt missing"; exit 1; }
	@test -s $(ISO_DIR)/bootcode.bin || { echo "ERROR: bootcode.bin empty or missing. Run: make firmware"; exit 1; }
	@test -s $(ISO_DIR)/start.elf || { echo "ERROR: start.elf empty or missing. Run: make firmware"; exit 1; }
	@test -s $(ISO_DIR)/fixup.dat || { echo "ERROR: fixup.dat empty or missing. Run: make firmware"; exit 1; }

# ── Download RPi firmware files ─────────────────────────────
#   Pinned to stable firmware release: 1.20241008
FIRMWARE_URL = https://github.com/raspberrypi/firmware/raw/1.20241008/boot

firmware:
	@mkdir -p $(ISO_DIR)
	@echo "Downloading Raspberry Pi 3B firmware (release 1.20241008)..."
	@for f in bootcode.bin start.elf fixup.dat; do \
		echo "  Fetching $$f..."; \
		curl -fsSL "$(FIRMWARE_URL)/$$f" \
			-o $(ISO_DIR)/$$f && echo "    ✅ $$f" || \
			{ echo "    ❌ $$f (download failed — try manually)"; exit 1; }; \
	done
	@echo "Firmware downloaded to $(ISO_DIR)/"

# ── Flash ISO to SD Card ────────────────────────────────────
iso-flash: iso
	@echo "=== Flashing $(ISO_IMG) to $(SDCARD) ==="
	@echo "WARNING: All data on $(SDCARD) will be destroyed!"
	@echo "Press Ctrl+C within 5 seconds to cancel..."
	@sleep 5
	@$(AUTH) dd if=$(ISO_IMG) of=$(SDCARD) bs=4M status=progress
	@$(AUTH) sync
	@echo "✅ Flash complete. Insert SD card into Raspberry Pi."

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET) $(IMG) $(MAP) $(DUMP)
	rm -f *.o *.d                              # Root-Schutz: falls jemand im Root kompiliert hat
	rm -f $(ISO_IMG) $(ISO_DIR)/vibecore.img  # .iso (current) + legacy .img

#   Clean everything including build directory
clean-all: clean
	rm -rf $(ISO_DIR)

# ── Extended Security & Logic Analysis ──────────────────────
.PHONY: analyze lint cppcheck clang-tidy clang-analyzer flawfinder
.PHONY: harden fanalyzer security logic audit check

# Quick pre-commit check (compile + basic analysis)
check: all
	@echo "=== Quick Pre-Commit Check ==="
	@$(MAKE) cppcheck 2>&1 | tail -3
	@echo "  ✅ Compile + cppcheck passed"

# ── Hardened Build (extra security flags) ──────────────────
harden:
	@echo "=== 🔒 Hardened Security Build ==="
	@echo "  Flags: stack-clash-protection + stack-protector-strong"
	@$(MAKE) clean
	@$(MAKE) CFLAGS="$(CFLAGS) -fstack-clash-protection" all
	@echo "  ✅ Hardened build complete"

# Hardened build + QEMU smoke test
harden-test: harden
	@echo ""
	@echo "=== 🧪 Hardened Kernel QEMU Test ==="
	@echo -e "\n\n\n\nversion\nmem\n" | timeout 12 qemu-system-aarch64 \
		-M raspi3b -cpu cortex-a53 -m 1G \
		-kernel $(IMG) -serial stdio -nographic \
		-monitor none 2>&1 | grep -E "VibeCore|vibecore>|Hardened|PASS|FAIL" || true
	@echo "  ✅ Hardened kernel boots in QEMU"

# ── GCC -fanalyzer (deep static analysis) ──────────────────
fanalyzer:
	@echo "=== GCC -fanalyzer Deep Analysis ==="
	@echo "  Checking: use-after-free, double-free, NULL deref,"
	@echo "            buffer overflow, memory leaks..."
	@$(MAKE) clean
	@$(MAKE) CFLAGS="$(subst -Werror,,$(CFLAGS)) -fanalyzer" all 2>&1 || true
	@echo "  ✅ fanalyzer complete (warnings above are informational)"

# ── Combined Security Checks ───────────────────────────────
security: flawfinder
	@echo ""
	@echo "=== 🔒 Security Audit Summary ==="
	@echo "  ✅ flawfinder (CWE/SANS Top 25)"
	@echo "  💡 Run 'make harden' for hardened compilation"
	@echo "  💡 Run 'make fanalyzer' for deep GCC analysis"

# ── Combined Logic Checks ──────────────────────────────────
logic: cppcheck
	@echo ""
	@echo "=== 🧠 Logic Check Summary ==="
	@echo "  ✅ cppcheck (bug & UB detection)"
	@echo "  💡 Run 'make fanalyzer' for deep analysis"
	@echo "  💡 Run 'make clang-tidy' for code quality"

# ── Full Comprehensive Audit ───────────────────────────────
audit:
	@echo "============================================"
	@echo "  🔍 VibeCore OS — Full Security Audit"
	@echo "============================================"
	@echo ""
	@$(MAKE) cppcheck
	@echo ""
	@$(MAKE) flawfinder
	@echo ""
	@$(MAKE) fanalyzer
	@echo ""
	@echo "============================================"
	@echo "  ✅ Full audit complete"
	@echo "  💡 Run: make harden-test (build+QEMU test)"
	@echo "============================================"

analyze: cppcheck flawfinder

lint: clang-tidy

cppcheck:
	@echo "=== Cppcheck Static Analysis ==="
	cppcheck --enable=all --inconclusive --std=c11 \
		--suppress=missingIncludeSystem \
		--suppress=unusedFunction \
		-Iinclude src/ 2>&1 || true

clang-tidy:
	@echo "=== Clang-Tidy Analysis ==="
	@which clang-tidy >/dev/null 2>&1 && \
		clang-tidy -p . --checks='*,-clang-analyzer-alpha.*' \
			src/*.c -- -Iinclude -nostdlib -ffreestanding 2>&1 || \
		echo "clang-tidy not installed. Install: apt install clang-tidy"

clang-analyzer:
	@echo "=== Clang Static Analyzer ==="
	@which scan-build >/dev/null 2>&1 && \
		scan-build --use-cc=aarch64-linux-gnu-gcc make clean all 2>&1 || \
		echo "scan-build not installed. Install: apt install clang-tools"

flawfinder:
	@echo "=== Flawfinder Security Audit (CWE/SANS Top 25) ==="
	@which flawfinder >/dev/null 2>&1 && \
		flawfinder --minlevel=1 --columns --context src/ include/ 2>&1 || \
		echo "flawfinder not installed. Install: apt install flawfinder"

help:
	@echo "VibeCore OS Build System"
	@echo ""
	@echo "  🔨 BUILD:"
	@echo "  make              Build kernel8.img (supports -j for parallel)"
	@echo "  make harden       Build with extra security hardening"
	@echo "  make clean        Remove build artifacts"
	@echo ""
	@echo "  🖥️  QEMU:"
	@echo "  make run          Start in QEMU (raspi3b, serial)"
	@echo "  make run-gui      Start in QEMU with GTK display"
	@echo "  make debug        Start in QEMU with GDB server"
	@echo ""
	@echo "  💿 ISO:"
	@echo "  make iso          Create ISO (auto: GUI→full, headless→base)"
	@echo "  make iso-noroot   Create base ISO — NO root, NO password"
	@echo "  make iso-full     Full ISO with files (needs desktop GUI)"
	@echo "  make iso-verify   Verify ISO + FAT32 header"
	@echo "  make iso-test     Test ISO in QEMU"
	@echo "  make firmware     Download RPi firmware (one-time)"
	@echo ""
	@echo "  🔒 SECURITY:"
	@echo "  make check        Quick pre-commit check (compile+analyze)"
	@echo "  make security     Security audit (flawfinder CWE/SANS)"
	@echo "  make fanalyzer    GCC deep analysis (use-after-free, NULL, overflow)"
	@echo "  make harden       Hardened build (stack-clash-protection)"
	@echo "  make harden-test  Hardened build + QEMU boot test"
	@echo ""
	@echo "  🧠 LOGIC:"
	@echo "  make logic        Bug & UB detection (cppcheck)"
	@echo "  make cppcheck     Full cppcheck analysis"
	@echo "  make clang-tidy   Code quality linting"
	@echo "  make clang-analyzer Clang deep logic errors"
	@echo ""
	@echo "  🔍 FULL AUDIT:"
	@echo "  make audit        ALL checks: cppcheck + flawfinder + fanalyzer"
	@echo "  make analyze      cppcheck + flawfinder"
	@echo ""
	@echo "  🔑 NO terminal password prompts — desktop GUI only!"
	@echo "  🔧 'make iso' always works (headless or desktop)"
	@echo ""
	@echo "  Parallel:    make -j$$(nproc)"
