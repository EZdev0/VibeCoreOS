#!/bin/bash
make iso-full
qemu-system-aarch64 \
    -M virt \
    -cpu cortex-a53 \
    -bios /usr/share/qemu-efi-aarch64/QEMU_EFI.fd \
    -m 1G \
    -drive file=build/vibecore.iso,format=raw,if=virtio \
    -nographic \
    -display none \
    -serial stdio
