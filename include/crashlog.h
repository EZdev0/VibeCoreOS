/*
 * ============================================================
 *  VibeCore OS — Crash Log Header
 * ============================================================
 */

#ifndef _CRASHLOG_H
#define _CRASHLOG_H

#include "types.h"

void crash_log_init(void);
void crash_log_write(const char *title, const char *desc,
                     u64 esr, u64 elr, u64 far);
void crash_log_check_previous(void);

#endif /* _CRASHLOG_H */
