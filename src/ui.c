/*
 * hftirc2 by Martin Duquesnoy <xorg62@gmail.com>
 * For license, see COPYING
 */

#include "hftirc.h"
#include "ui.h"
#include "util.h"

struct buffer*
ui_buffer_new(struct session *session, char *name)
{
     struct buffer *b;

     b = xcalloc(1, sizeof(struct buffer));

     b->session = session;
     b->name    = xstrdup(name);
     b->topic   = NULL;
     b->id      = ACT_NO;

     SLIST_INIT(&b->lines);
     SLIST_INIT(&b->nicks);

     TAILQ_INSERT_TAIL(&H.h.buffer, b, next);

     return b;
}

void
ui_buffer_remove(struct buffer *b)
{
     struct buffer_line *bl;

     free(b->name);
     free(b->topic);

     while(!SLIST_EMPTY(&b->lines))
     {
          bl = SLIST_FIRST(&b->lines);
          SLIST_REMOVE_HEAD(&b->lines, next);
          free(bl->line);
          free(bl);
     }

     free(b);
}

void
ui_init(void)
{
     struct buffer *b;

     /* Create status buffer at first init */
     if(H.flags & HFTIRC_FIRST_TIME)
     {
          b = ui_buffer_new(NULL, "status");
          b->topic = xstrdup("HFTIrc2 -- status buffer");
          H.bufsel = b;
          H.flags ^= HFTIRC_FIRST_TIME;
     }

     setlocale(LC_ALL, "");
     initscr();
     raw();
     noecho();
     keypad(stdscr, true);
     curs_set(false);

     /* Input buffer stuff */
     H.ui.ib.split = 0;
     H.ui.tcolor = COLOR_THEME_DEFAULT;

     /* Color support -- I can haz colors? */
     if(has_colors())
          ui_init_color();

     /* Init main window and the borders */
     H.ui.mainwin = newwin(MAINWIN_LINES, COLS - (H.ui.nicklist ? NICKLIST_SIZE : 0), 1, 0);
     scrollok(H.ui.mainwin, true);
     wrefresh(H.ui.mainwin);

     /* Init topic window */
     H.ui.topicwin = newwin(1, COLS, 0, 0);
     wbkgd(H.ui.topicwin, COLOR_SW);
     wrefresh(H.ui.statuswin);

     /* Init nicklist window */
     H.ui.nicklistwin = newwin(LINES - 3, NICKLIST_SIZE, 1, COLS - NICKLIST_SIZE);
     wrefresh(H.ui.nicklistwin);

     /* Init input window */
     H.ui.inputwin = newwin(1, COLS, LINES - 1, 0);
     wmove(H.ui.inputwin, 0, 0);
     H.ui.ib.nhisto = 1;
     H.ui.ib.histpos = 0;
     wrefresh(H.ui.inputwin);

     /* Init status window (with the hour / current chan) */
     H.ui.statuswin = newwin(1, COLS, LINES - 2, 0);
     wbkgd(H.ui.statuswin, COLOR_SW);
     wrefresh(H.ui.statuswin);

     refresh();
}

int
ui_color(int fg, int bg)
{
     int i;
     short f, b;

     if(bg == COLOR_BLACK && H.ui.bg != COLOR_BLACK)
          bg = H.ui.bg;

     for(i = 0; i < H.ui.c + 1; ++i)
          if(pair_content(i, &f, &b) == OK && f == fg && b == bg)
               return COLOR_PAIR(i);

     return 0;
}
