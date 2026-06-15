/*
 * ============================================================
 *  VibeCore OS — First Boot Setup
 *
 *  Shows graphical welcome screen on first boot,
 *  then drops to shell. No interactive prompts.
 * ============================================================
 */

#include "types.h"
#include "kernel.h"
#include "string.h"
#include "gui_welcome.h"

void setup_wizard_run(void)
{
    /* Show graphical welcome screen on framebuffer */
    gui_welcome_show();

    uart_puts("[SETUP] Configured: username=vibecore, hostname=vibecore-pi\n");
}
