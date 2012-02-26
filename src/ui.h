/*
 * hftirc2 by Martin Duquesnoy <xorg62@gmail.com>
 * For license, see COPYING
 */

#ifndef UI_H
#define UI_H

#include "hftirc.h"

/* Colors shortcuts */
#define COLOR_THEME_DEFAULT COLOR_BLUE
#define COLOR_SW            ui_color(COLOR_BLACK, H.ui.tcolor)
#define COLOR_SW2           ui_color(COLOR_WHITE, H.ui.tcolor)
#define COLOR_ROSTER        ui_color(H.ui.tcolor, H.ui.bg)
#define COLOR_ACT           ui_color(COLOR_WHITE,  H.ui.tcolor)
#define COLOR_HLACT         (ui_color(COLOR_RED, H.ui.tcolor) | A_BOLD)
#define COLOR_LASTPOS       ui_color(COLOR_BLUE, H.ui.bg | A_BOLD)

#define NICKLIST_SIZE 20
#define MAINWIN_LINES (LINES - 2)

void ui_init(void);
int ui_color(int fg, int bg);
struct buffer* ui_buffer_new(struct session *session, char *name);
void ui_buffer_remove(struct buffer *b);



#endif
