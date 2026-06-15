/*
 * ============================================================
 *  VibeCore OS — Memory Allocator Header (OOM-safe)
 * ============================================================
 */

#ifndef _ALLOCATOR_H
#define _ALLOCATOR_H

#include "types.h"

/* ── Initialisierung ─────────────────────────────────────── */
void  allocator_init(void);

/* ── Graceful OOM: return NULL (no panic!) ──────────────── */
void *kmalloc(size_t size);
void *kzalloc(size_t size);
void *kcalloc(size_t count, size_t size);

/* ── Kritische Allokation: panic() bei OOM ───────────────── */
void *kmalloc_or_panic(size_t size);

/* ── Freigabe ────────────────────────────────────────────── */
void  kfree(void *ptr);

/* ── Diagnose ────────────────────────────────────────────── */
void allocator_stats(void);
bool allocator_check_stack(void);

#define SIZE_MAX  ((size_t)-1)

#endif /* _ALLOCATOR_H */
