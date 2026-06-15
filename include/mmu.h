/*
 * ============================================================
 *  VibeCore OS — MMU Header
 * ============================================================
 */

#ifndef _MMU_H
#define _MMU_H

#include "types.h"

void mmu_init(void);
void mmu_map_page(u64 va, u64 pa, u64 attrs);

#endif /* _MMU_H */
