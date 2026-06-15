# DOX Framework - Source

## Purpose
Governs the core source code (`.c`, `.S`) for the VibeCore OS kernel. It details the rules for architecture separation, hardware interaction, and kernel subsystem implementations.

## Ownership
- Core OS Development Team

## Local Contracts
- Code must be split cleanly by architecture (e.g., `arch/aarch64`, `arch/x86`).
- Generic kernel code must not contain architecture-specific registers or MMIO hardcodes.
- Ensure 100% complete code with no placeholders or "TODOs".

## Work Guidance
- Use strict typing. Casts must be explicitly justified.
- Optimize critical paths for extreme performance.
- When creating bootloaders or installers, ensure deep memory validation and error handling to prevent crashes.

## Verification
- `make check` inside the root directory to validate all `src/` files.
- `make fanalyzer` and `make flawfinder` for deep code analysis.
- Architecture-specific builds must pass independently.
