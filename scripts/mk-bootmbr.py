#!/usr/bin/env python3
"""
Generate a minimal 440-byte MBR (Master Boot Record) boot code.
Prints a message via BIOS interrupt 0x10 and halts.
This makes the disk image recognizable as "bootable" by BIOS-based VMs.

Output: 440 bytes of x86 real-mode machine code to stdout.
"""

import struct
import sys

def assemble_mbr():
    """Assemble a minimal MBR that prints a message and halts."""
    msg = b"VibeCore OS ARM64 - Boot via QEMU: make run\r\nKein BIOS-Boot - Flashe auf SD-Karte\r\n"
    
    # x86 real-mode assembly (hand-encoded):
    # org 0x7C00
    code = bytearray()
    
    # Set up segments (CS=0x0000, DS=0x0000, ES=0x0000)
    # xor ax, ax
    code += b'\x31\xc0'
    # mov ds, ax
    code += b'\x8e\xd8'
    # mov es, ax
    code += b'\x8e\xc0'
    # mov ss, ax
    code += b'\x8e\xd0'
    # mov sp, 0x7C00
    code += b'\xbc\x00\x7c'
    
    # mov si, msg_offset
    # We need to calculate the offset of the message
    # Current position after setup: 11 bytes
    # After mov si + jmp: +5 bytes = 16 bytes
    # So msg is at offset 16 from start
    msg_offset = 16  # will be calculated after we know exact position
    code += b'\xbe' + struct.pack('<H', msg_offset)
    
    # Print loop:
    # .next_char:
    next_char_offset = len(code)
    # lodsb
    code += b'\xac'
    # test al, al
    code += b'\x84\xc0'
    # jz .halt
    # (jump offset will be calculated)
    jz_patch_pos = len(code)
    code += b'\x74\x00'  # placeholder
    
    # mov ah, 0x0E  (BIOS teletype output)
    code += b'\xb4\x0e'
    # mov bh, 0x00  (page 0)
    code += b'\xb7\x00'
    # int 0x10
    code += b'\xcd\x10'
    # jmp .next_char
    jmp_back = next_char_offset - (len(code) + 2)
    code += b'\xeb' + struct.pack('<b', jmp_back)
    
    # .halt:
    halt_offset = len(code)
    # Fix up the jz .halt jump
    jz_offset = halt_offset - (jz_patch_pos + 2)
    code[jz_patch_pos + 1] = jz_offset & 0xFF
    
    # cli
    code += b'\xfa'
    # hlt
    code += b'\xf4'
    # jmp .halt (infinite loop if NMI)
    code += b'\xeb\xfe'
    
    # Message
    code += msg + b'\x00'
    
    # Pad to exactly 440 bytes with zeros
    if len(code) > 440:
        print(f"ERROR: MBR code too large: {len(code)} bytes (max 440)", file=sys.stderr)
        sys.exit(1)
    
    code += b'\x00' * (440 - len(code))
    
    assert len(code) == 440, f"MBR must be exactly 440 bytes, got {len(code)}"
    
    return bytes(code)


if __name__ == '__main__':
    mbr = assemble_mbr()
    sys.stdout.buffer.write(mbr)
