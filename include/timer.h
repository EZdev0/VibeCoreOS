/*
 * ============================================================
 *  VibeCore OS — Timer Header
 * ============================================================
 */

#ifndef _TIMER_H
#define _TIMER_H

#include "types.h"

void timer_init(void);

#include "boot_anim.h"
#include "crashlog.h"
void timer_sleep_ms(u32 ms);
void timer_usleep(u32 us);
u64  timer_get_ticks(void);
u64  timer_get_ms(void);

#endif /* _TIMER_H */
