/*
 * ============================================================
 *  VibeCore OS — GUI / Desktop Header
 * ============================================================
 */

#ifndef _GUI_H
#define _GUI_H

#include "types.h"

void gui_desktop_init(void);
void gui_draw_taskbar(void);
void gui_draw_icon(i32 x, i32 y, const char *label, char symbol, Color color, const char *tooltip);
void gui_draw_window(i32 x, i32 y, i32 w, i32 h, const char *title);
void gui_update_clock(u32 hours, u32 minutes);

#endif /* _GUI_H */
