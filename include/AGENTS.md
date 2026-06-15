# DOX Framework - Includes

## Purpose
Governs the C headers (`.h`) that declare kernel APIs, type definitions, and peripheral MMIO mappings.

## Ownership
- Core OS Development Team

## Local Contracts
- Headers must include include-guards (`#ifndef ...`).
- Only define hardware abstractions here. Implementation details must reside in `src/`.
- Maintain strict type definitions (e.g., `u32`, `u64`).

## Work Guidance
- Use `BOARD` definitions to toggle peripheral base addresses (e.g., `MMIO_BASE` for `raspi3`, `raspi4`, `virt`).
- Keep hardware structs cleanly packed if mapping directly to memory.

## Verification
- Validate header inclusion chains by running `make check`.
