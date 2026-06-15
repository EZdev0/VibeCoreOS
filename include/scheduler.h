/*
 * ============================================================
 *  VibeCore OS — Scheduler Header
 * ============================================================
 */

#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#include "types.h"

typedef enum {
    PRIO_IDLE    = 0,
    PRIO_LOW     = 1,
    PRIO_NORMAL  = 2,
    PRIO_HIGH    = 3,
    PRIO_REALTIME = 4
} TaskPriority;

typedef struct task Task;

void  scheduler_init(void);
void  scheduler_preempt(void);
Task *scheduler_create_task(const char *name, void (*entry)(void), TaskPriority prio);
void  scheduler_boost(Task *task);
void  scheduler_yield(void);
void  scheduler_block(Task *task);
void  scheduler_unblock(Task *task);
Task *scheduler_get_current(void);
void  scheduler_run_current(void);

#endif /* _SCHEDULER_H */
