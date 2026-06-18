#!/bin/bash
mkdir -p build/iso_temp/EFI/BOOT
cp BOOTAA64.EFI build/iso_temp/EFI/BOOT/BOOTAA64.EFI

# 1) Try using grub-mkrescue, it is the standard way to create bootable UEFI ISOs
sudo apt install -y grub-efi-arm64-bin mtools xorriso dosfstools grub-common grub2-common mtools
mkdir -p build/iso_temp/boot/grub
cat << 'CFG' > build/iso_temp/boot/grub/grub.cfg
set timeout=5
set default=0
menuentry "VibeCore OS" {
    linux /kernel8.img
}
CFG
cp kernel8.img build/iso_temp/

# We'll just run grub-mkrescue if it's available, otherwise fallback to our xorriso custom logic
if command -v grub-mkrescue >/dev/null 2>&1; then
    grub-mkrescue -o build/vibecore_aarch64_rescue.iso build/iso_temp -- -volid VIBECORE
else
    echo "grub-mkrescue not found"
fi
