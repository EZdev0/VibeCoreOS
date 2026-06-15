/*
 * ============================================================
 *  VibeCore OS — Graphical Recovery Screen Header
 *
 *  Renders directly to framebuffer. Keyboard-driven, no mouse.
 *  Options: 1=FSCK, 2=Snapshot Restore, 3=Reboot.
 * ============================================================
 */

#ifndef _GUI_RECOVERY_H
#define _GUI_RECOVERY_H

#include "types.h"

void gui_recovery_show(const char *reason);
void gui_recovery_trigger(const char *reason);

#endif /* _GUI_RECOVERY_H */
