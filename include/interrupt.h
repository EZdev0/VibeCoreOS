/*
 * ============================================================
 *  VibeCore OS — Interrupt/Exception Header
 * ============================================================
 */

#ifndef _INTERRUPT_H
#define _INTERRUPT_H

#include "types.h"

void crash_screen_show(const char *title, const char *desc,
                       u64 esr, u64 elr, u64 far);
int  snprintf_local(char *buf, size_t max, const char *fmt, ...);

#endif /* _INTERRUPT_H */
