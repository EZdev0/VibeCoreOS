import re

with open('Makefile', 'r') as f:
    content = f.read()

content = content.replace("type=0c", "type=ef")
content = content.replace(
    "@dd if=$(ISO_DIR)/_part.fat of=$(ISO_IMG) bs=512 seek=$(ISO_PART_START) conv=notrunc status=none\n\t@rm -f $(ISO_DIR)/_part.fat\n\t@echo \"  [4/6] Injecting MBR boot code (VM compatibility)...\"",
    "@echo \"  [4/6] Injecting MBR boot code (VM compatibility)...\""
)

content = re.sub(
    r"@\$\(AUTH\) bash -c ' \\.*?rmdir \$\$MNT'",
    r"""@mmd -i $(ISO_DIR)/_part.fat ::/EFI
	@mmd -i $(ISO_DIR)/_part.fat ::/EFI/BOOT
	@mcopy -i $(ISO_DIR)/_part.fat $(ISO_DIR)/EFI/BOOT/BOOTAA64.EFI ::/EFI/BOOT/BOOTAA64.EFI
	@mcopy -i $(ISO_DIR)/_part.fat $(IMG) ::/kernel8.img
	@mcopy -i $(ISO_DIR)/_part.fat config.txt ::/config.txt
	@mcopy -i $(ISO_DIR)/_part.fat $(ISO_DIR)/bootcode.bin ::/bootcode.bin
	@mcopy -i $(ISO_DIR)/_part.fat $(ISO_DIR)/start.elf ::/start.elf
	@mcopy -i $(ISO_DIR)/_part.fat $(ISO_DIR)/fixup.dat ::/fixup.dat
	@echo "  [6/6] Files on boot partition:"
	@mdir -i $(ISO_DIR)/_part.fat
	@dd if=$(ISO_DIR)/_part.fat of=$(ISO_IMG) bs=512 seek=$(ISO_PART_START) conv=notrunc status=none
	@rm -f $(ISO_DIR)/_part.fat""",
    content,
    flags=re.DOTALL
)

with open('Makefile', 'w') as f:
    f.write(content)
