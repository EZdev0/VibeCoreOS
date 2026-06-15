# DOX Framework - Root

## Purpose
This is the root AGENTS.md for VibeCore OS. It defines the global architecture, operating rules, and serves as the top-level contract for the VibeCore OS project. VibeCore OS is a bare-metal operating system aiming for maximum performance, universal multi-architecture support, and isolated execution in QEMU/KVM environments.

## Ownership
- Project Lead OS-Architect & Bare-Metal Developer
- VibeCore Labs

## Local Contracts
- All work must adhere strictly to the rules defined in `Jules.md`, `RULES.md`, and `README.md`.
- No host system modifications are allowed. All tests must be contained within QEMU/KVM emulator (`make run`, `make run-gui`).
- No terminal password prompts (`sudo`/`su` in terminal).
- Ensure 0 compile warnings, 0 fanalyzer issues, and 0 cppcheck critical issues.

## Work Guidance
- Every change must prioritize maximum performance and zero latency.
- Custom bootloaders and deep hardware configurations must not rely on predefined UEFI/BIOS binaries if possible, building from scratch where required.
- Expand architecture to multiple targets (`aarch64`, `x86`, `arm`) using explicit compiler flags and directory separation.
- Maintain isolation; any QEMU boot fixes for AArch64 KVM (like the `virt` board) must securely boot without harming the Raspberry Pi host.

## Verification
- Run `make check` as a pre-commit check.
- Run `make clean && make -j$(nproc)` to ensure 0 errors.
- Run `make run` to verify QEMU boot reachability.
- Run `make audit` periodically for deep analysis.

## Child DOX Index
- `src/AGENTS.md`: Source code rules, kernel structure, architecture isolation, and subsystems.
- `include/AGENTS.md`: Header structure, hardware peripheral definitions, and type safety.
- `scripts/AGENTS.md`: Build scripts, authorization helpers, and boot code generators.
