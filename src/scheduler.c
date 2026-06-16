/*
 * ============================================================
 *  VibeCore OS — BORE-inspired Task Scheduler
 *
 *  BORE = "Burst-Oriented Response Enhancer"
 *  Inspired by the CachyOS BORE scheduler:
 *    - Identifies "bursty" (interactive) tasks
 *    - Boosts their priority for better responsiveness
 *    - Prevents CPU starvation with fairness guarantees
 *
 *  Features:
 *    - Round-Robin with priority queue
 *    - Burst detection for interactive tasks
 *    - Preemption (via timer tick)
 *    - Idle task (WFE loop)
 * ============================================================
 */

#include "types.h"
#include "kernel.h"
#include "string.h"
#include "scheduler.h"
#include "timer.h"

/* ── Task States ──────────────────────────────────────────── */
typedef enum {
    TASK_READY = 0,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_SLEEPING,
    TASK_ZOMBIE,
    TASK_DEAD
} TaskState;

/* ── Task Structure ───────────────────────────────────────── */
typedef struct task {
    u64         id;               /* Unique task ID */
    /* flawfinder: ignore */
    char        name[32];         /* Task name */
    void        (*entry)(void);   /* Entry point */
    TaskState   state;            /* Current state */
    TaskPriority base_priority;   /* Base priority */
    TaskPriority effective_priority; /* Effective (boosted) priority */

    /* BORE Metrics */
    u64         burst_credits;    /* Credits for burst behavior */
    u64         last_run_tick;    /* Last execution (tick) */
    u64         total_runtime;    /* Total CPU time (ticks) */
    u64         sleep_since_tick;

    /* Context (simplified: function only, no register saves needed) */
    void        *stack_ptr;

    struct task *next;
} Task;

/* ── Global Scheduler Data ────────────────────────────────── */
static Task *task_list      = NULL;
static Task *current_task   = NULL;
static u64   next_task_id   = 0;
static u32   task_count     = 0;

/* ── BORE Constants ───────────────────────────────────────── */
#define BURST_CREDIT_MAX      100     /* Max burst credits */
#define BURST_CREDIT_BOOST    20      /* Boost per burst event */
#define BURST_DECAY_RATE      1       /* Decay per tick */
#define BURST_THRESHOLD       50      /* Threshold for "bursty" classification */

/* ── Idle Task ────────────────────────────────────────────── */
static void idle_task_entry(void)
{
    while (1) {
        asm volatile("wfe");
    }
}

/* ────────────────────────────────────────────────────────────
 *  scheduler_init
 *
 *  Creates the idle task and initializes the scheduler.
 * ────────────────────────────────────────────────────────── */

void scheduler_init(void)
{
    uart_puts("[SCHED] BORE scheduler initialized.\n");
    task_list = NULL;
    current_task = NULL;
    next_task_id = 1;
    task_count = 0;

    /* Create idle task (Prio 0, runs when nothing else is ready) */
    scheduler_create_task("idle", idle_task_entry, PRIO_IDLE);
    uart_puts("[SCHED] Idle task created.\n");
}

/* ────────────────────────────────────────────────────────────
 *  scheduler_create_task
 *
 *  Creates a new task and adds it to the ready queue.
 * ────────────────────────────────────────────────────────── */

Task *scheduler_create_task(const char *name, void (*entry)(void), TaskPriority prio)
{
    if (!name || !entry) return NULL;

    /* Allocate task structure (simplified: global pool) */
    static Task task_pool[MAX_TASKS];
    static u32 pool_idx = 0;

    if (pool_idx >= MAX_TASKS) {
        uart_puts("[SCHED] ERROR: Maximum task count reached!\n");
        return NULL;
    }

    Task *task = &task_pool[pool_idx++];
    memset(task, 0, sizeof(Task));

    task->id = next_task_id++;
    /* flawfinder: ignore */
    strncpy(task->name, name, 31);
    task->name[31] = '\0';
    task->entry = entry;
    task->state = TASK_READY;
    task->base_priority = prio;
    task->effective_priority = prio;
    task->burst_credits = 0;
    task->last_run_tick = 0;

    /* Insert into linked list */
    if (task_list == NULL) {
        task_list = task;
    } else {
        Task *t = task_list;
        while (t->next) t = t->next;
        t->next = task;
    }

    task_count++;
    uart_printf("[SCHED] Task '%s' (ID=%d, Prio=%d) created.\n",
                name, (int)task->id, (int)prio);
    return task;
}

/* ────────────────────────────────────────────────────────────
 *  scheduler_preempt
 *
 *  Called on every timer tick.
 *  Decides whether the current task should be preempted.
 *  (BORE: Update burst credits, boost priorities)
 * ────────────────────────────────────────────────────────── */

void scheduler_preempt(void)
{
    Task *best = NULL;
    int best_score = -1;

    /* Update burst credits for all tasks */
    for (Task *t = task_list; t != NULL; t = t->next) {
        /* Decay: credits slowly expire */
        if (t->burst_credits > 0) {
            t->burst_credits = (t->burst_credits > BURST_DECAY_RATE)
                               ? (t->burst_credits - BURST_DECAY_RATE) : 0;
        }

        /* BORE: Bursty? Apply boost */
        if (t->burst_credits > BURST_THRESHOLD) {
            t->effective_priority = MIN(t->base_priority + 1, PRIO_REALTIME);
        } else {
            t->effective_priority = t->base_priority;
        }

        /* Score: priority × 1000 - runtime (anti-starvation) */
        int score = (int)t->effective_priority * 1000 - (int)(t->total_runtime % 1000);

        if (t->state == TASK_READY && score > best_score) {
            best_score = score;
            best = t;
        } else if (t->state == TASK_RUNNING) {
            /* Current task gets slight bonus (avoids thrashing) */
            score += 100;
            if (score > best_score) {
                best_score = score;
                best = t;
            }
        }
    }

    /* Switch task if needed */
    if (best != NULL && best != current_task) {
        if (current_task) {
            current_task->state = TASK_READY;
        }
        current_task = best;
        current_task->state = TASK_RUNNING;
        current_task->last_run_tick = timer_get_ms();
    }

    /* Increase burst credits for current task */
    if (current_task && current_task->state == TASK_RUNNING) {
        current_task->total_runtime++;
        current_task->burst_credits = MIN(current_task->burst_credits + 1, BURST_CREDIT_MAX);
    }
}

/* ────────────────────────────────────────────────────────────
 *  scheduler_boost
 *
 *  Manually increase burst credits for an interactive task.
 *  Called e.g. on keyboard input.
 * ────────────────────────────────────────────────────────── */

void scheduler_boost(Task *task)
{
    if (task == NULL) return;
    task->burst_credits = MIN(task->burst_credits + BURST_CREDIT_BOOST, BURST_CREDIT_MAX);
    uart_printf("[SCHED] Task '%s' boosted (Credits: %d)\n",
                task->name, (int)task->burst_credits);
}

/* ────────────────────────────────────────────────────────────
 *  scheduler_yield
 *
 *  Task voluntarily gives up the CPU.
 * ────────────────────────────────────────────────────────── */

void scheduler_yield(void)
{
    if (current_task) {
        current_task->state = TASK_READY;
    }
    scheduler_preempt();
}

/* ────────────────────────────────────────────────────────────
 *  scheduler_block / scheduler_unblock
 * ────────────────────────────────────────────────────────── */

void scheduler_block(Task *task)
{
    if (task == NULL) return;
    task->state = TASK_BLOCKED;
}

void scheduler_unblock(Task *task)
{
    if (task == NULL) return;
    task->state = TASK_READY;
    /* Boost credits: task was blocked and is now active again */
    scheduler_boost(task);
}

/* ────────────────────────────────────────────────────────────
 *  scheduler_get_current
 * ────────────────────────────────────────────────────────── */

Task *scheduler_get_current(void)
{
    return current_task;
}

void scheduler_run_current(void)
{
    if (current_task && current_task->entry) {
        current_task->state = TASK_RUNNING;
        current_task->entry();
    }
}
