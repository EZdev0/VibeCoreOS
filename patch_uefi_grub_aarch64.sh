#!/bin/bash
sudo apt install -y grub-common grub2-common mtools dosfstools xorriso

mkdir -p build/iso_temp/boot/grub
cat << 'CFG' > build/iso_temp/boot/grub/grub.cfg
set timeout=5
set default=0
menuentry "VibeCore OS" {
    linux /kernel8.img
}
CFG
cp kernel8.img build/iso_temp/

# We'll use a direct UEFI boot file (not grub-mkstandalone because we don't have grub-efi-arm64-bin, we have our own stub)
mkdir -p build/iso_temp/EFI/BOOT
cp BOOTAA64.EFI build/iso_temp/EFI/BOOT/BOOTAA64.EFI

dd if=/dev/zero of=build/efi.img bs=1M count=33
mkfs.fat -F 32 build/efi.img
mmd -i build/efi.img ::/EFI
mmd -i build/efi.img ::/EFI/BOOT
mcopy -i build/efi.img BOOTAA64.EFI ::/EFI/BOOT/BOOTAA64.EFI

# But maybe we need grub for x86_64 or aarch64 properly. Let's try sticking to our stub
cp build/efi.img build/iso_temp/efi.img

xorriso -as mkisofs -R -f \
  -e efi.img -no-emul-boot \
  -isohybrid-gpt-basdat \
  -o build/vibecore_aarch64.iso build/iso_temp
