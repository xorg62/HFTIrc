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
#define STATUS_BUFFER TAILQ_FIRST(&H.h.buffer)

/* Test control-bind */
#define IS_CTRLK(c)  (c > 0 && c < 32)
#define CTRLK(c)     (c & 037)

#define HFTIRC_KEY_ENTER  (10)
#define HFTIRC_KEY_ALTBP  (27)
#define HFTIRC_KEY_DELALL CTRLK('u')

void ui_init(void);
int ui_color(int fg, int bg);
struct buffer* ui_buffer_new(struct session *session, const char *name);
void ui_buffer_remove(struct buffer *b);
void ui_get_input(void);
void ui_update(void);
void ui_print_buf(struct buffer *b, char *fmt, ...);
void ui_buffer_set(struct buffer *b);
void ui_buffer_scroll(struct buffer *b, int n);

static inline struct buffer*
ui_buffer_gb_id(int n)
{
     struct buffer *b;

     TAILQ_FOREACH(b, &H.h.buffer, next)
          if(b->id == n)
               return b;

     return NULL;
}

static inline struct buffer*
ui_buffer_find(struct session *s, const char *name)
{
     struct buffer *b;

     if(!name)
          return STATUS_BUFFER;

     TAILQ_FOREACH(b, &H.h.buffer, next)
          if(b->session == s && !strcmp(name, b->name))
               return b;

     return STATUS_BUFFER;
}

#endif
