# DOX Framework - Scripts

## Purpose
Governs the build, testing, and deployment scripts for VibeCore OS.

## Ownership
- Build & Automation Team

## Local Contracts
- Scripts must be deterministic.
- `auth-helper.sh` must remain GUI-only for password prompts.
- Do not add destructive host commands (e.g., untargeted `dd`).

## Work Guidance
- All scripts should support verbose output if needed.
- Maintain `mk-bootmbr.py` or similar for specific bootloader generation.

## Verification
- Run `make iso-verify` or manual script execution testing.
