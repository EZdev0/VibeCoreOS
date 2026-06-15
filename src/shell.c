/*
 * ============================================================
 *  VibeCore OS — Shell (Command-Line Interface)
 *
 *  Interactive shell with commands:
 *    help     — Show all available commands
 *    clear    — Clear screen
 *    info     — System information
 *    mem      — Memory statistics
 *    tasks    — Task list
 *    fs       — Filesystem info
 *    version  — OS version
 *    crash    — Test crash screen
 *    reboot   — System restart (WIP)
 *    log      — Kernel log (dmesg)
 *    dmesg    — Kernel log (alias)
 *
 *  Runs on UART console and framebuffer (planned).
 * ============================================================
 */

#include "types.h"
#include "kernel.h"
#include "string.h"
#include "framebuffer.h"
#include "shell.h"
#include "interrupt.h"
#include "timer.h"
#include "allocator.h"
#include "recovery.h"
#include "klog.h"
#include "gui_recovery.h"

/* ── External Functions ──────────────────────────────────── */
extern void allocator_stats(void);
extern bool allocator_check_stack(void);
extern u64  timer_get_ms(void);

/* ── Shell Buffer ─────────────────────────────────────────── */
static char cmd_buffer[MAX_CMD_LEN];
static u32  cmd_pos = 0;

/* ── Prompt ───────────────────────────────────────────────── */
#define SHELL_PROMPT  "vibecore> "

/* ── Forward Declarations for Shell Commands ──────────────── */
static void shell_cmd_help(void);
static void shell_cmd_clear(void);
static void shell_cmd_info(void);
static void shell_cmd_mem(void);
static void shell_cmd_version(void);
static void shell_cmd_tasks(void);
static void shell_cmd_fs(void);
static void shell_cmd_crash_test(void);
static void shell_cmd_reboot(void);
static void shell_cmd_gui(void);
static void shell_cmd_log(void);

/* ────────────────────────────────────────────────────────────
 *  shell_run
 *
 *  Main shell loop. Reads commands from UART and
 *  executes them. NEVER returns.
 * ────────────────────────────────────────────────────────── */

void shell_run(void)
{
    uart_puts("\n╔══════════════════════════════════════════════════╗\n");
    uart_puts("║   VibeCore OS Shell — Type 'help' for commands.  ║\n");
    uart_puts("╚══════════════════════════════════════════════════╝\n\n");

    while (1) {
        /* Show prompt */
        uart_puts(SHELL_PROMPT);

        /* Read command */
        cmd_pos = 0;
        memset(cmd_buffer, 0, sizeof(cmd_buffer));

        while (1) {
            char c = uart_getc();

            /* Enter → execute command */
            if (c == '\r' || c == '\n') {
                uart_puts("\n");
                cmd_buffer[cmd_pos] = '\0';
                break;
            }

            /* Backspace */
            if (c == '\b' || c == 127) {
                if (cmd_pos > 0) {
                    cmd_pos--;
                    uart_puts("\b \b");
                }
                continue;
            }

            /* Normal character */
            if (cmd_pos < MAX_CMD_LEN - 1) {
                cmd_buffer[cmd_pos++] = c;
                uart_putc(c);
            }
        }

        /* Skip empty input */
        if (cmd_pos == 0) continue;

        /* Execute command */
        shell_execute(cmd_buffer);
    }
}

/* ────────────────────────────────────────────────────────────
 *  shell_execute
 *
 *  Parses and executes a shell command.
 * ────────────────────────────────────────────────────────── */

void shell_execute(const char *cmd)
{
    CHECK_NULL(cmd);

    /* Parse command (first word only, no arguments)
     * Later: argc/argv implementation */

    /* Find first non-whitespace character */
    while (*cmd == ' ') cmd++;

    if (*cmd == '\0') return;

    /* ── HELP ──────────────────────────────────────────── */
    if (strcmp(cmd, "help") == 0) {
        shell_cmd_help();
    }
    /* ── CLEAR ─────────────────────────────────────────── */
    else if (strcmp(cmd, "clear") == 0) {
        shell_cmd_clear();
    }
    /* ── INFO ──────────────────────────────────────────── */
    else if (strcmp(cmd, "info") == 0 || strcmp(cmd, "sysinfo") == 0) {
        shell_cmd_info();
    }
    /* ── MEM ───────────────────────────────────────────── */
    else if (strcmp(cmd, "mem") == 0 || strcmp(cmd, "memory") == 0) {
        shell_cmd_mem();
    }
    /* ── VERSION ───────────────────────────────────────── */
    else if (strcmp(cmd, "version") == 0 || strcmp(cmd, "ver") == 0) {
        shell_cmd_version();
    }
    /* ── TASKS ─────────────────────────────────────────── */
    else if (strcmp(cmd, "tasks") == 0 || strcmp(cmd, "ps") == 0) {
        shell_cmd_tasks();
    }
    /* ── FS ────────────────────────────────────────────── */
    else if (strcmp(cmd, "fs") == 0 || strcmp(cmd, "df") == 0) {
        shell_cmd_fs();
    }
    /* ── LOG / DMESG ───────────────────────────────────── */
    else if (strcmp(cmd, "log") == 0 || strcmp(cmd, "dmesg") == 0) {
        shell_cmd_log();
    }
    /* ── CRASH (Test) ──────────────────────────────────── */
    else if (strcmp(cmd, "crash") == 0 || strcmp(cmd, "panic") == 0) {
        shell_cmd_crash_test();
    }
    /* ── REBOOT ────────────────────────────────────────── */
    else if (strcmp(cmd, "reboot") == 0) {
        shell_cmd_reboot();
    }
    /* ── GUI ───────────────────────────────────────────── */
    else if (strcmp(cmd, "gui") == 0) {
        shell_cmd_gui();
    }
    /* ── FSCK ──────────────────────────────────────────── */
    else if (strcmp(cmd, "fsck") == 0 || strcmp(cmd, "check") == 0) {
        recovery_shell_fsck();
    }
    /* ── TRASH ──────────────────────────────────────────── */
    else if (strcmp(cmd, "trash") == 0) {
        recovery_shell_trash_list();
    }
    /* ── VERSIONS ───────────────────────────────────────── */
    else if (strcmp(cmd, "versions") == 0 || strcmp(cmd, "verlist") == 0) {
        recovery_shell_versions_list("/");
    }
    /* ── RECOVER <filename> ──────────────────────────────── */
    else if (strncmp(cmd, "recover", 7) == 0) {
        const char *arg = cmd + 7;
        while (*arg == ' ') arg++;
        if (*arg == '\0') {
            uart_puts("  Usage: recover <filename>\n");
            recovery_shell_trash_list();
        } else {
            recovery_trash_restore(arg);
        }
    }
    /* ── RECOVERY ────────────────────────────────────────── */
    else if (strcmp(cmd, "recovery") == 0) {
        gui_recovery_trigger("Manual recovery requested");
    }
    /* ── SNAPSHOT ────────────────────────────────────────── */
    else if (strcmp(cmd, "snapshot") == 0) {
        recovery_snapshot_create();
    }
    /* ── PROTECT ─────────────────────────────────────────── */
    else if (strcmp(cmd, "protect") == 0) {
        uart_puts("  Protected system files (immutable):\n");
        uart_puts("    /kernel8.img, /boot.img, /config.txt\n");
        uart_puts("    /.journal, /.hashes, /.trash, /.versions, /.snapshots\n");
    }
    /* ── Unknown ─────────────────────────────────────────── */
    else {
        uart_printf("  Unknown command: '%s'\n", cmd);
        uart_puts("  Type 'help' for a list of commands.\n");
    }
}

/* ────────────────────────────────────────────────────────────
 *  Shell Commands
 * ────────────────────────────────────────────────────────── */

static void shell_cmd_help(void)
{
    uart_puts("\n");
    uart_puts("  ╔══════════════════════════════════════════════╗\n");
    uart_puts("  ║     VibeCore OS — Available Commands         ║\n");
    uart_puts("  ╠══════════════════════════════════════════════╣\n");
    uart_puts("  ║  help     — This help                       ║\n");
    uart_puts("  ║  clear    — Clear screen (UART)             ║\n");
    uart_puts("  ║  info     — System information              ║\n");
    uart_puts("  ║  mem      — Memory statistics               ║\n");
    uart_puts("  ║  tasks    — Task list (scheduler)           ║\n");
    uart_puts("  ║  fs       — Filesystem info                 ║\n");
    uart_puts("  ║  version  — Show OS version                 ║\n");
    uart_puts("  ║  gui      — Desktop info                    ║\n");
    uart_puts("  ║  log      — Kernel log (dmesg)              ║\n");
    uart_puts("  ║  crash    — Test crash screen               ║\n");
    uart_puts("  ║  fsck     — Filesystem integrity check      ║\n");
    uart_puts("  ║  trash    — Show trash bin                  ║\n");
    uart_puts("  ║  versions — Show file versions              ║\n");
    uart_puts("  ║  recover  — Restore file from trash         ║\n");
    uart_puts("  ║  recovery — Open graphical recovery screen  ║\n");
    uart_puts("  ║  snapshot — Create system backup            ║\n");
    uart_puts("  ║  protect  — Show protected files            ║\n");
    uart_puts("  ║  reboot   — Restart system                  ║\n");
    uart_puts("  ╚══════════════════════════════════════════════╝\n");
    uart_puts("\n");
}

static void shell_cmd_clear(void)
{
    /* ANSI Escape: Clear Screen + Cursor Home */
    uart_puts("\033[2J\033[H");
}

static void shell_cmd_info(void)
{
    uart_puts("\n");
    uart_puts("  ╔══════════════════════════════════════════════╗\n");
    uart_puts("  ║         System Information                   ║\n");
    uart_puts("  ╠══════════════════════════════════════════════╣\n");
    uart_printf("  ║  OS:      VibeCore OS %d.%d.%d", VIBECORE_VERSION_MAJOR, VIBECORE_VERSION_MINOR, VIBECORE_VERSION_PATCH);
    for (int i = 0; i < 26; i++) { uart_putc(' '); }
    uart_puts("║\n");
    uart_puts("  ║  Codename: Photon                            ║\n");
    uart_puts("  ║  Arch:     aarch64 (ARM64)                   ║\n");
    uart_puts("  ║  CPU:      Cortex-A53 / BCM2837              ║\n");
    uart_puts("  ║  RAM:      1 GB (QEMU) / 1 GB (RPi 3B)      ║\n");
    uart_printf("  ║  Uptime:   %d ms\n", (int)timer_get_ms());
    uart_puts("  ║                                              ║\n");
    uart_puts("  ║  Features:                                   ║\n");
    uart_puts("  ║    - BORE-inspired Scheduler                 ║\n");
    uart_puts("  ║    - MMU Memory Protection                   ║\n");
    uart_puts("  ║    - Framebuffer Graphics                    ║\n");
    uart_puts("  ║    - FAT32 Filesystem                        ║\n");
    uart_puts("  ║    - Crash Screen with Diagnostics           ║\n");
    uart_puts("  ║    - Kernel Logging (dmesg)                  ║\n");
    uart_puts("  ╚══════════════════════════════════════════════╝\n");
    uart_puts("\n");
}

static void shell_cmd_mem(void)
{
    uart_puts("\n");
    allocator_stats();

    /* Stack canary check */
    if (!allocator_check_stack()) {
        uart_puts("  [CRITICAL] Stack overflow detected! System unstable.\n");
    }
    uart_puts("\n");
}

static void shell_cmd_version(void)
{
    uart_printf("\n  %s\n", VIBECORE_VERSION_STRING);
    uart_puts("  Build: " __DATE__ " " __TIME__ "\n\n");
}

static void shell_cmd_tasks(void)
{
    uart_puts("\n");
    uart_puts("  ╔══════════════════════════════════════════════╗\n");
    uart_puts("  ║     Scheduler — Task List                    ║\n");
    uart_puts("  ╠══════════════════════════════════════════════╣\n");
    uart_puts("  ║  ID  Name          State    Prio  Runtime    ║\n");
    uart_puts("  ╠══════════════════════════════════════════════╣\n");
    uart_puts("  ║  1   idle          READY    IDLE  0 ms       ║\n");
    uart_puts("  ║  2   shell         RUNNING  HIGH  ...        ║\n");
    uart_puts("  ╚══════════════════════════════════════════════╝\n");
    uart_puts("\n");
}

static void shell_cmd_fs(void)
{
    uart_puts("\n");
    uart_puts("  ╔══════════════════════════════════════════════╗\n");
    uart_puts("  ║     Filesystem Info                          ║\n");
    uart_puts("  ╠══════════════════════════════════════════════╣\n");
    uart_puts("  ║  Type:    FAT32 (RAM-Disk)                   ║\n");
    uart_puts("  ║  Mount:   /                                  ║\n");
    uart_puts("  ║  Free:    ~100 MB                            ║\n");
    uart_puts("  ╚══════════════════════════════════════════════╝\n");
    uart_puts("\n");
}

static void shell_cmd_log(void)
{
    /* Dump the kernel log ring buffer */
    klog_dump();
}

static void shell_cmd_crash_test(void)
{
    klog_warn("Manually triggered crash test (shell command)");
    uart_puts("\n  Simulating kernel panic in 3...\n");
    timer_sleep_ms(1000);
    uart_puts("  2...\n");
    timer_sleep_ms(1000);
    uart_puts("  1...\n");
    timer_sleep_ms(1000);

    /* Trigger explicit crash */
    crash_screen_show("TEST CRASH",
                      "Shell crash test (manually triggered)",
                      0xDEADBEEF, 0xCAFE1234, 0x0BADF00D);
}

static void shell_cmd_reboot(void)
{
    uart_puts("\n  System restarting...\n");
    timer_sleep_ms(500);

    /* Watchdog reboot or reset vector */
    uart_puts("  Reboot: Watchdog reset...\n");
    /* TODO: Watchdog timer for real reboot */
    while (1) { asm volatile("wfi"); }
}

static void shell_cmd_gui(void)
{
    uart_puts("\n");
    if (framebuffer_is_ready()) {
        uart_printf("  Desktop active: %dx%d, 32-bit\n",
                    framebuffer_get_width(), framebuffer_get_height());
        uart_puts("  Rendering: Framebuffer (Mailbox interface)\n");
    } else {
        uart_puts("  Desktop not available (headless mode).\n");
    }
    uart_puts("\n");
}
