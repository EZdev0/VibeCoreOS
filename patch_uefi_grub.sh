#!/bin/bash
sudo apt install -y grub-efi-arm64-bin
mkdir -p build/iso_temp/boot/grub
mkdir -p build/iso_temp/EFI/BOOT
cat << 'CFG' > build/iso_temp/boot/grub/grub.cfg
set timeout=5
set default=0
menuentry "VibeCore OS" {
    chainloader /EFI/BOOT/BOOTAA64.EFI
}
CFG
grub-mkstandalone -O arm64-efi -o build/iso_temp/EFI/BOOT/grubaa64.efi "boot/grub/grub.cfg=build/iso_temp/boot/grub/grub.cfg"
# ... we don't have grub-efi-arm64-bin
