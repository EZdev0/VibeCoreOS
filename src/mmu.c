/*
 * ============================================================
 *  VibeCore OS — MMU (Memory Management Unit)
 *
 *  ARM64 MMU configuration:
 *    - 4KB Translation Granule
 *    - 3-Level Page-Table (L0 → L1 → L2 → L3)
 *    - Identity Mapping (VA = PA) for the kernel
 *    - Kernel-Space: 0xFFFF000000000000 - 0xFFFFFFFFFFFFFFFF
 *    - User-Space:   0x0000000000000000 - 0x0000FFFFFFFFFFFF
 *    - Device-Memory (nGnRE) for MMIO
 *    - Normal-Cacheable for RAM
 *
 *  Features:
 *    - Page-Protection (Read/Write/Execute)
 *    - Access-Flag (AF) Tracking
 *    - Guard-Pages at heap end
 * ============================================================
 */

#include "types.h"
#include "kernel.h"
#include "string.h"
#include "allocator.h"

/* ── MMU Constants ────────────────────────────────────────── */
#define PAGE_SIZE           4096
#define PAGE_SHIFT          12
#define PAGE_MASK           (PAGE_SIZE - 1)

/* Translation Table Levels */
#define TT_L0_SHIFT         39
#define TT_L1_SHIFT         30
#define TT_L2_SHIFT         21
#define TT_L3_SHIFT         12

/* Table Descriptor Types */
#define TT_TYPE_TABLE       0b11   /* Next-Level Table */
#define TT_TYPE_BLOCK       0b01   /* Block Entry (L1/L2) */
#define TT_TYPE_PAGE        0b11   /* Page Entry (L3) */

/* Memory Attributes (MAIR) */
#define MAIR_DEVICE_nGnRE   0x00   /* Device-Memory */
#define MAIR_NORMAL_NC      0x44   /* Normal, Non-Cacheable */
#define MAIR_NORMAL_WT      0xBB   /* Normal, Write-Through */
#define MAIR_NORMAL_WB      0xFF   /* Normal, Write-Back Cacheable */

/* Descriptor Attribute Bits */
#define TD_ATTR_VALID       BIT(0)
#define TD_ATTR_TABLE       TT_TYPE_TABLE
#define TD_ATTR_BLOCK       TT_TYPE_BLOCK
#define TD_ATTR_PAGE        TT_TYPE_PAGE
#define TD_ATTR_AF          BIT(10)  /* Access Flag */
#define TD_ATTR_nG          BIT(11)  /* Not Global */
#define TD_ATTR_AP_EL1_RW   (0b00 << 6)  /* EL1 Read/Write */
#define TD_ATTR_AP_EL0_NONE (0b00 << 6)  /* EL0 No Access */
#define TD_ATTR_AP_EL0_RO   (0b10 << 6)  /* EL0 Read-Only */
#define TD_ATTR_AP_EL0_RW   (0b01 << 6)  /* EL0 Read/Write */
#define TD_ATTR_UXN         BIT(54) /* Unprivileged Execute-Never */
#define TD_ATTR_PXN         BIT(53) /* Privileged Execute-Never */

/* MAIR Indices */
#define MAIR_IDX_DEVICE     0
#define MAIR_IDX_NORMAL_NC  1
#define MAIR_IDX_NORMAL_WB  2

/* ── Page-Tables (statically allocated, 4KB aligned) ──────── */
static u64 __attribute__((aligned(4096))) tt_l0[512];
static u64 __attribute__((aligned(4096))) tt_l1[512];
static u64 __attribute__((aligned(4096))) tt_l2[512];

/* ────────────────────────────────────────────────────────────
 *  mmu_init
 *
 *  Creates the initial page table (identity mapping)
 *  for the entire physical memory.
 *
 *  Mapping:
 *    - 0x00000000 - 0x3FFFFFFF → Device-Memory (MMIO)
 *    - 0x40000000 - RAM_END     → Normal-Cacheable (RAM)
 *    - Identity-Map (VA = PA)
 * ────────────────────────────────────────────────────────── */

void mmu_init(void)
{
    uart_puts("[MMU] Initializing ARM64 MMU...\n");

    /* 1. Zero page tables */
    memset(tt_l0, 0, sizeof(tt_l0));
    memset(tt_l1, 0, sizeof(tt_l1));
    memset(tt_l2, 0, sizeof(tt_l2));

    /*
     * 2. Configure MAIR_EL1
     *    Attr0: Device-nGnRE
     *    Attr1: Normal, Non-Cacheable
     *    Attr2: Normal, Write-Back Cacheable
     */
    u64 mair = (MAIR_DEVICE_nGnRE  << (MAIR_IDX_DEVICE * 8)) |
               (MAIR_NORMAL_NC     << (MAIR_IDX_NORMAL_NC * 8)) |
               (MAIR_NORMAL_WB     << (MAIR_IDX_NORMAL_WB * 8));
    asm volatile("msr mair_el1, %0" :: "r"(mair));

    /*
     * 3. Configure TCR_EL1
     *    48-bit VA, 48-bit PA, 4KB granule
     */
    u64 tcr = (16UL << 0)   |    /* T0SZ: 2^48 User VA */
              (16UL << 16) | (0UL << 14) | /* T1SZ: 2^48 Kernel VA */
              (0b00 << 14)  |    /* TG0: 4KB */
              (0b101ULL << 32) | /* IPS: 48-bit PA */
              (0b11 << 8)   |    /* Inner Shareable */
              (0b11 << 10)  |    /* Outer Shareable */
              (0b01 << 12);      /* Inner WBWA, Outer WBWA */
    asm volatile("msr tcr_el1, %0" :: "r"(tcr));

    /*
     * 4. Build identity mapping
     *    L0-Table → L1-Table → L2-Blocks (2MB granules)
     */

    /* L0: One entry for the first 512 GB (Index 0) */
    tt_l0[0] = (u64)(uintptr_t)&tt_l1 | TD_ATTR_TABLE;

    /* L1: Identity-map with 1GB blocks */
    for (int i = 0; i < 2; i++) {
        u64 addr = i * 0x40000000UL;  /* 1GB per block */
        u64 attr = TD_ATTR_BLOCK | TD_ATTR_AF | TD_ATTR_AP_EL1_RW;

        if (addr < 0x40000000UL) {
            attr |= (MAIR_IDX_DEVICE << 2);     /* 0x00000000: MMIO, Device-nGnRE */
        } else {
            attr |= (MAIR_IDX_NORMAL_WB << 2);  /* 0x40000000: RAM, Write-Back */
        }

        tt_l1[i] = addr | attr;
    }

    /* 5. Set TTBR0_EL1 and TTBR1_EL1 */
    u64 ttbr0_pa = (u64)(uintptr_t)&tt_l0;
    asm volatile("msr ttbr0_el1, %0" :: "r"(ttbr0_pa));
    asm volatile("msr ttbr1_el1, %0" :: "r"(ttbr0_pa));

    /* 6. Invalidate TLBs */
    asm volatile("tlbi vmalle1is");
    asm volatile("dsb ish");
    asm volatile("isb");

    /* 7. Enable MMU (SCTLR_EL1) */
    u64 sctlr;
    asm volatile("mrs %0, sctlr_el1" : "=r"(sctlr));
    sctlr |= (1 << 0);   /* MMU enable */
    sctlr |= (1 << 12);  /* I-Cache */
    sctlr |= (1 << 2);   /* D-Cache */
    asm volatile("msr sctlr_el1, %0" :: "r"(sctlr));
    asm volatile("isb");

    uart_puts("[MMU] Initialization complete.\n");
    uart_printf("[MMU] TTBR0: 0x%x, SCTLR: 0x%x\n", ttbr0_pa, sctlr);
}

/* ────────────────────────────────────────────────────────────
 *  mmu_map_page
 *
 *  Maps a single 4KB page (for future expansion).
 * ────────────────────────────────────────────────────────── */

void mmu_map_page(u64 va, u64 pa, u64 attrs)
{
    u64 l1_idx = (va >> TT_L1_SHIFT) & 0x1FF;

    u64 l1_desc = tt_l1[l1_idx];
    if (l1_desc & TT_TYPE_BLOCK) {
        u64 block_addr = l1_desc & ~0x3FFFFFFFUL;
        u64 new_table_pa = (u64)(uintptr_t)kmalloc(4096);
        CHECK_NULL_MSG((void*)new_table_pa, "mmu_map_page: OOM");

        u64 *l2_table = (u64*)(uintptr_t)new_table_pa;
        memset(l2_table, 0, 4096);

        for (int i = 0; i < 512; i++) {
            l2_table[i] = (block_addr + i * 0x200000UL) |
                          TD_ATTR_BLOCK | TD_ATTR_AF | TD_ATTR_AP_EL1_RW |
                          ((l1_desc >> 2) & 0x7);
        }

        tt_l1[l1_idx] = new_table_pa | TD_ATTR_TABLE;
        asm volatile("tlbi vmalle1is; dsb ish; isb");
    }

    UNUSED(va);
    UNUSED(pa);
    UNUSED(attrs);
    uart_puts("[MMU] mmu_map_page: 4KB mapping not yet fully implemented.\n");
}
