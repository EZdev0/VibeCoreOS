#!/bin/bash
qemu-system-aarch64 \
    -M virt \
    -cpu cortex-a53 \
    -m 1G \
    -bios /usr/share/qemu-efi-aarch64/QEMU_EFI.fd \
    -drive file=build/vibecore_aarch64_rescue.iso,format=raw \
    -nographic \
    -serial stdio -monitor none
