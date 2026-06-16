/*
 * ============================================================
 *  VibeCore OS — Memory Allocator (Intelligent OOM-safe)
 *
 *  Features:
 *    - GRACEFUL OOM: kmalloc() returns NULL, NO panic()
 *    - Watermark monitoring: warns at 75%/90%/95% usage
 *    - Emergency reserve: 64KB reserved for crash diagnostics
 *    - Stack canary: stack overflow detection
 *    - Alignment guarantees (16-byte)
 *    - Memory tracking with high-water mark
 *
 *  Optimized for 1GB-constrained Raspberry Pi environments.
 * ============================================================
 */

#include "types.h"
#include "kernel.h"
#include "string.h"
#include "allocator.h"
#include "interrupt.h"

/* ── Allocator Metadata ───────────────────────────────────── */
typedef struct alloc_block {
    u64     size;
    bool    free;
    u32     magic; /* cppcheck-suppress unusedStructMember */
    struct alloc_block *next; /* cppcheck-suppress unusedStructMember */
    struct alloc_block *prev; /* cppcheck-suppress unusedStructMember */
} AllocBlock;

#define ALLOC_MAGIC         0xDEADBEEF
#define ALLOC_ALIGNMENT     16
#define ALLOC_HEADER_SIZE   ALIGN_UP(sizeof(AllocBlock), ALLOC_ALIGNMENT)

/* ── Watermark Thresholds ─────────────────────────────────── */
#define WM_WARN         75    /* Yellow warning */
#define WM_CRITICAL     90    /* Orange warning */
#define WM_EMERGENCY    95    /* Red warning — reserve active */

/* ── Emergency Reserve ────────────────────────────────────── */
#define EMERGENCY_RESERVE_SIZE  65536   /* 64 KB emergency pool */

/* ── Global State ─────────────────────────────────────────── */
static u8  *heap_current    = NULL;
static u8  *heap_start      = NULL;
static u8  *heap_limit      = NULL;
static u8  *heap_emergency  = NULL;     /* Start of emergency reserve */
static u64  total_allocated  = 0;
static u64  total_freed      = 0;
static u64  high_water_mark  = 0;       /* Max bytes ever allocated */
static u32  allocation_count = 0;
static u32  oom_count        = 0;        /* Number of OOM events */
static bool emergency_active = false;

/* ── Stack Canary (initialized by boot.S) ────────────────── */
static volatile u64 stack_canary __attribute__((section(".stack_canary"))) = 0;
#define STACK_CANARY_MAGIC  0xCAFE1337BEEF4242ULL

/* ────────────────────────────────────────────────────────────
 *  allocator_init
 *
 *  Initializes the heap. Reserves 64KB emergency pool
 *  at the END of the heap — used ONLY in emergencies.
 * ────────────────────────────────────────────────────────── */

void allocator_init(void)
{
    heap_start   = (u8*)&__heap_start;
    heap_limit   = (u8*)&__heap_end;
    heap_current = heap_start;

    /* Separate emergency reserve at heap end */
    heap_emergency = (u8*)((uintptr_t)heap_limit - EMERGENCY_RESERVE_SIZE);
    if (heap_emergency < heap_start) {
        heap_emergency = heap_start;  /* Safety: heap too small */
    }

    /* Zero heap (up to reserve) */
    size_t clear_size = (size_t)(heap_emergency - heap_start);
    memset(heap_start, 0, clear_size);

    /* Set stack canary */
    stack_canary = STACK_CANARY_MAGIC;

    uart_printf("[ALLOC] Heap: 0x%x - 0x%x (%d KB) | Reserve: %d KB\n",
                (u64)heap_start, (u64)heap_limit,
                (int)((heap_emergency - heap_start) / 1024),
                (int)(EMERGENCY_RESERVE_SIZE / 1024));
}

/* ────────────────────────────────────────────────────────────
 *  allocator_check_watermark
 *
 *  Internal: checks heap usage and warns.
 *  Called AFTER each allocation.
 * ────────────────────────────────────────────────────────── */

static void allocator_check_watermark(void)
{
    u64 used  = (u64)(heap_current - heap_start);
    u64 total = (u64)(heap_emergency - heap_start);
    if (total == 0) return;

    u32 pct = (u32)((used * 100) / total);

    /* Update high-water mark */
    if (used > high_water_mark) {
        high_water_mark = used;
    }

    if (pct >= WM_EMERGENCY && !emergency_active) {
        emergency_active = true;
        uart_printf("[ALLOC] *** CRITICAL: %d%% heap usage! Emergency reserve active. ***\n", pct);
    } else if (pct >= WM_CRITICAL && pct < WM_EMERGENCY) {
        uart_printf("[ALLOC] WARNING: %d%% heap usage — critical!\n", pct);
    } else if (pct >= WM_WARN && pct < WM_CRITICAL) {
        /* Warn only once per 100 allocations (anti-spam) */
        static u32 last_warn_at = 0;
        if (allocation_count - last_warn_at > 100) {
            uart_printf("[ALLOC] Notice: %d%% heap usage.\n", pct);
            last_warn_at = allocation_count;
        }
    }
}

/* ────────────────────────────────────────────────────────────
 *  kmalloc — GRACEFUL OOM (no panic!)
 *
 *  Allocates `size` bytes on the kernel heap.
 *  Returns NULL on OOM — the caller MUST check!
 *
 *  Null checks:
 *    - size == 0 → warning, NULL
 *    - Heap full (including emergency) → NULL (OOM)
 *    - Alignment check after allocation
 * ────────────────────────────────────────────────────────── */

void *kmalloc(size_t size)
{
    if (size == 0) {
        uart_puts("[ALLOC] WARNING: kmalloc(0)!\n");
        return NULL;
    }

    /* Ensure alignment */
    size = ALIGN_UP(size, ALLOC_ALIGNMENT);

    /*
     * Intelligent OOM handling:
     * 1. Try normal heap first
     * 2. Then emergency reserve (if critical)
     * 3. Then return NULL (NO panic!)
     */

    bool use_emergency = false;
    const u8 *target = heap_current + size;

    /* Check: does it fit in normal heap? */
    if (target > heap_emergency) {
        /* Normal heap full. Try emergency reserve. */
        if (emergency_active && target <= heap_limit) {
            use_emergency = true;
            uart_printf("[ALLOC] OOM: Using emergency reserve (%d bytes requested)\n", (int)size);
        } else {
            /* Emergency also full → real OOM */
            uart_printf("[ALLOC] OOM: No memory! (requested: %d, free: %d)\n",
                        (int)size, (int)(heap_limit - heap_current));
            oom_count++;
            return NULL;   /* <-- NO panic! Caller must check! */
        }
    }

    u8 *ptr = heap_current;
    heap_current += size;

    /* Return zeroed memory */
    memset(ptr, 0, size);

    /* Check alignment */
    if (!IS_ALIGNED((uintptr_t)ptr, ALLOC_ALIGNMENT)) {
        uart_puts("[ALLOC] ERROR: Alignment violated!\n");
        return NULL;
    }

    total_allocated += size;
    allocation_count++;

    /* Watermark check */
    allocator_check_watermark();

    UNUSED(use_emergency);
    return (void*)ptr;
}

/* ────────────────────────────────────────────────────────────
 *  kmalloc_or_panic
 *
 *  Like kmalloc(), but panic() on OOM.
 *  ONLY use for CRITICAL boot allocations!
 * ────────────────────────────────────────────────────────── */

void *kmalloc_or_panic(size_t size)
{
    void *ptr = kmalloc(size);
    if (ptr == NULL) {
        uart_printf("[ALLOC] FATAL: Critical allocation (%d bytes) failed!\n",
                    (int)size);
        panic("Critical allocation failed — system cannot continue");
    }
    return ptr;
}

/* ────────────────────────────────────────────────────────────
 *  kzalloc — Zero allocation
 * ────────────────────────────────────────────────────────── */

void *kzalloc(size_t size)
{
    void *ptr = kmalloc(size);
    if (ptr) memset(ptr, 0, size);
    return ptr;
}

/* ────────────────────────────────────────────────────────────
 *  kcalloc — Array allocation with overflow check
 * ────────────────────────────────────────────────────────── */

void *kcalloc(size_t count, size_t size)
{
    if (count > 0 && size > (SIZE_MAX / count)) {
        uart_puts("[ALLOC] ERROR: kcalloc overflow!\n");
        return NULL;
    }
    return kzalloc(count * size);
}

/* ────────────────────────────────────────────────────────────
 *  kfree
 * ────────────────────────────────────────────────────────── */

void kfree(void *ptr)
{
    if (ptr == NULL) {
        return;
    }

    total_freed++;
    UNUSED(ptr);
    /* Free-list implementation for reuse: TODO */
}

/* ────────────────────────────────────────────────────────────
 *  allocator_check_stack
 *
 *  Checks stack canary for corruption (stack overflow).
 *  Should be called periodically.
 * ────────────────────────────────────────────────────────── */

bool allocator_check_stack(void)
{
    if (stack_canary != STACK_CANARY_MAGIC) {
        uart_puts("[ALLOC] *** STACK OVERFLOW DETECTED! Stack canary corrupt! ***\n");
        uart_printf("[ALLOC] Canary: expected 0x%x, found 0x%x\n",
                    (u32)STACK_CANARY_MAGIC, (u32)stack_canary);
        return false;
    }
    return true;
}

/* ────────────────────────────────────────────────────────────
 *  allocator_stats — Extended statistics with watermark
 * ────────────────────────────────────────────────────────── */

void allocator_stats(void)
{
    u64 used  = (u64)(heap_current - heap_start);
    u64 total = (u64)(heap_emergency - heap_start);
    u64 free  = total - used;
    u32 pct   = (u32)((used * 100) / total);
    u32 hwm_pct = (u32)((high_water_mark * 100) / total);

    uart_puts("[ALLOC] === Memory Statistics ===\n");
    uart_printf("  Heap size:      %d KB\n", (int)(total / 1024));
    uart_printf("  Used:           %d KB (%d%%)\n", (int)(used / 1024), pct);
    uart_printf("  Free:           %d KB (%d%%)\n", (int)(free / 1024), 100 - pct);
    uart_printf("  High-water:     %d KB (%d%%)\n", (int)(high_water_mark / 1024), hwm_pct);
    uart_printf("  Allocations:    %d\n", allocation_count);
    uart_printf("  OOM events:     %d\n", oom_count);
    uart_printf("  Emergency pool: %d KB (%s)\n",
                (int)(EMERGENCY_RESERVE_SIZE / 1024),
                emergency_active ? "ACTIVE" : "ready");
    uart_printf("  Stack canary:   %s\n",
                allocator_check_stack() ? "OK" : "CORRUPT!");
    uart_puts("[ALLOC] ============================\n");
}
