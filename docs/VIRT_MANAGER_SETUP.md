# VibeCore OS: QEMU/KVM Direct Kernel Boot (Virt-Manager)

To test VibeCore OS safely and with hardware acceleration in Virt-Manager/QEMU, it must not be booted via UEFI/BIOS. The emulator must instead load the kernel image (`kernel8.img`) directly. This protects your host system (Raspberry Pi OS) from accidental damage.

## Virt-Manager Konfiguration

Open the Virt-Manager GUI, select your created VM (aarch64) and go to **Settings (Details)**:

1. **Boot Options**
   - Go to "Boot Options" and ensure that `Direct kernel boot` is enabled.
2. **Kernel path**
   - Select the path to your freshly compiled `kernel8.img` here.
3. **Machine Type**
   - Ensure that the machine is configured for `virt` (virt board).
4. **Memory & CPU**
   - Select `cortex-a53` (or host) as CPU model and allocate min. 1024MB RAM.

### XML Configuration (for libvirt direct input)

If you edit the VM via XML (`virsh edit vm-name`), ensure the `<os>` block looks like this:

```xml
<os>
  <type arch='aarch64' machine='virt'>hvm</type>
  <kernel>/pfad/zu/deinem/VibeCoreOS/kernel8.img</kernel>
  <cmdline>console=ttyAMA0</cmdline>
</os>
```

Through this method you bypass TianoCore/UEFI completely and the emulator runs the bare-metal operating system directly. The code has been modified so that the hardware addresses of the `virt` board are fully supported during boot (e.g. UART at `0x09000000`).
