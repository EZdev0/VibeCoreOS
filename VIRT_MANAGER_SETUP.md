# VibeCore OS: QEMU/KVM Direct Kernel Boot (Virt-Manager)

Um VibeCore OS sicher und mit Hardwarebeschleunigung in Virt-Manager/QEMU zu testen, darf nicht über UEFI/BIOS gebootet werden. Der Emulator muss stattdessen das Kernel-Image (`kernel8.img`) direkt laden. Das schützt dein Host-System (Raspberry Pi OS) vor versehentlichen Schäden.

## Virt-Manager Konfiguration

Öffne die Virt-Manager GUI, wähle deine erstellte VM (aarch64) und gehe in die **Einstellungen (Details)**:

1. **Boot Options**
   - Gehe zu "Boot Options" und stelle sicher, dass `Direct kernel boot` aktiviert ist.
2. **Kernel path**
   - Wähle hier den Pfad zu deinem frisch kompilierten `kernel8.img`.
3. **Machine Type**
   - Stelle sicher, dass die Maschine auf `virt` (virt Board) konfiguriert ist.
4. **Memory & CPU**
   - Wähle `cortex-a53` (oder host) als CPU Modell und weise min. 1024MB RAM zu.

### XML-Konfiguration (für libvirt direkteingaben)

Falls du die VM über XML bearbeitest (`virsh edit vm-name`), stelle sicher, dass der `<os>` Block wie folgt aussieht:

```xml
<os>
  <type arch='aarch64' machine='virt'>hvm</type>
  <kernel>/pfad/zu/deinem/VibeCoreOS/kernel8.img</kernel>
  <cmdline>console=ttyAMA0</cmdline>
</os>
```

Durch diese Methode umgehst du TianoCore/UEFI komplett und der Emulator führt das Bare-Metal-Betriebssystem direkt aus. Der Code wurde so modifiziert, dass die Hardwareadressen des `virt`-Boards während des Bootens voll unterstützt werden (z.B. UART an `0x09000000`).
