/*
 * hftirc2 by Martin Duquesnoy <xorg62@gmail.com>
 * For license, see COPYING
 */

#include <ctype.h>

#include "hftirc.h"
#include "ui.h"
#include "util.h"
#include "input.h"

struct buffer*
ui_buffer_new(struct session *session, char *name)
{
     struct buffer *b, *p = TAILQ_LAST(&H.h.buffer, bsub);

     b = xcalloc(1, sizeof(struct buffer));

     b->session = session;
     b->name    = xstrdup(name);
     b->topic   = NULL;
     b->act     = ACT_NO;
     b->id      = (p ? p->id + 1 : 0);
     b->nline   = 0;

     TAILQ_INIT(&b->lines);
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

     while(!TAILQ_EMPTY(&b->lines))
     {
          bl = TAILQ_FIRST(&b->lines);
          TAILQ_REMOVE(&b->lines, bl, next);
          free(bl->line);
          free(bl);
     }

     TAILQ_REMOVE(&H.h.buffer, b, next);
     free(b);
}

static void
ui_init_color(void)
{
     int i, j, n = 0;

     start_color();

     H.ui.bg = ((use_default_colors() == OK) ? -1 : COLOR_BLACK);
     H.ui.c = 1;

     for(i = 0; i < COLORS; ++i)
          for(j = 0; j < COLORS; ++j)
               init_pair(++n, i, (!j ? H.ui.bg : j));

     H.ui.c = n;
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

          b = ui_buffer_new(NULL, "b1");
          b->topic = xstrdup("buffer 1");
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

void
ui_update(void)
{
     werase(H.ui.statuswin);
     wbkgd(H.ui.statuswin, COLOR_SW);
     mvwprintw(H.ui.statuswin, 0, 0, "[dev:4:20] (%d:%s)", H.bufsel->id, H.bufsel->name);
     wrefresh(H.ui.statuswin);

     werase(H.ui.topicwin);
     wbkgd(H.ui.topicwin, COLOR_SW);
     waddstr(H.ui.topicwin, H.bufsel->topic);
     wrefresh(H.ui.topicwin);
}

static void
ui_print_line(struct buffer_line *bl)
{
     waddstr(H.ui.mainwin, bl->line);
}

static void
ui_update_buf(void)
{
     struct buffer_line *b;

     werase(H.ui.mainwin);

     TAILQ_FOREACH(b, &H.bufsel->lines, next)
          ui_print_line(b);

     wrefresh(H.ui.mainwin);
}


static struct buffer_line*
ui_buffer_line_new(struct buffer *b, char *str)
{
     struct buffer_line *h, *bl = xcalloc(1, sizeof(struct buffer_line));

     bl->line = str;
     bl->id = b->nline++;

     TAILQ_INSERT_TAIL(&b->lines, bl, next);

     /* Remove head if histo length is reached */
     if(b->nline >= BUFHISTOLEN)
     {
          h = TAILQ_FIRST(&b->lines);
          TAILQ_REMOVE(&b->lines, h, next);
          free(h->line);
          free(h);
          --b->nline;
     }

     return bl;
}

void
ui_print_buf(struct buffer *b, char *fmt, ...)
{
     struct buffer_line *bl;
     char *str, *fstr;
     va_list args;

     va_start(args, fmt);
     (void)vasprintf(&str, fmt, args);
     va_end(args);

     xasprintf(&fstr, "[dev:4:20]  %s\n", str);

     bl = ui_buffer_line_new(b, fstr);

     if(b == H.bufsel)
     {
          ui_print_line(bl);
          wrefresh(H.ui.mainwin);
     }

     free(str);
}

void
ui_buffer_set(struct buffer *b)
{
     b->act = ACT_NO;

     H.bufsel = b;

     if(b != TAILQ_FIRST(&H.h.buffer))
          H.sessionsel = b->session;

     ui_update_buf();
}

void
ui_get_input(void)
{
     struct buffer *bf;
     struct inputbuffer *ib = &H.ui.ib;
     char buf[BUFSIZE];
     int i, j, n, t;
     wint_t c;

     switch((t = get_wch(&c)))
     {
     case ERR: break;
     default:
          switch(c)
          {
          case KEY_F(1):
          case CTRLK('p'):
               if(!(bf = TAILQ_PREV(H.bufsel, bsub, next)))
                    bf = TAILQ_LAST(&H.h.buffer, bsub);

               ui_buffer_set(bf);
               break;

          case KEY_F(2):
          case CTRLK('n'):
               if(!(bf = TAILQ_NEXT(H.bufsel, next)))
                    bf = TAILQ_FIRST(&H.h.buffer);

               ui_buffer_set(bf);
               break;

          case KEY_HOME:
               wmove(H.ui.inputwin, 0, 0);
               ib->pos = ib->cpos = 0;
               ib->spting = ib->split = 0;
               break;

          case KEY_END:
               wmove(H.ui.inputwin, 0, (int)wcslen(ib->buffer));
               ib->pos = (int)wcslen(ib->buffer);

               if(ib->spting || (int)wcslen(ib->buffer) > COLS - 1)
               {
                    werase(H.ui.inputwin);
                    ib->spting = 1;
                    ib->cpos = COLS - 1;
                    ib->split = (int)wcslen(ib->buffer) - COLS + 1;
               }
               else
                    ib->cpos = (int)wcslen(ib->buffer);
               break;

          case KEY_DC:
               wdelch(H.ui.inputwin);

               if(ib->buffer[ib->pos] != 0 && ib->pos >= 0)
                    for(i = ib->pos;
                        ib->buffer[i];
                        ib->buffer[i] = ib->buffer[i + 1], ++i);
               break;

          case KEY_UP:
               if(ib->nhisto)
               {
                    if(ib->histpos >= ib->nhisto)
                         ib->histpos = 0;

                    wmemset(ib->buffer, 0, BUFSIZE);
                    wcscpy(ib->buffer, ib->histo[ib->nhisto - ++ib->histpos]);
                    werase(H.ui.inputwin);
                    ib->cpos = ib->pos = wcslen(ib->buffer);

                    if(ib->pos >= COLS - 1)
                    {
                         ib->split = ib->pos - (COLS - 1);
                         ib->spting = 1;
                         ib->cpos = COLS - 1;
                    }
               }
               break;

          case KEY_DOWN:
               if(ib->nhisto && ib->histpos > 0 && ib->histpos < ib->nhisto)
               {
                    wmemset(ib->buffer, 0, BUFSIZE);
                    wcscpy(ib->buffer, ib->histo[ib->nhisto - (--ib->histpos)]);
                    werase(H.ui.inputwin);
                    ib->cpos = ib->pos = wcslen(ib->buffer);

                    if(ib->pos >= COLS - 1)
                    {
                         ib->split = ib->pos - (COLS - 1);
                         ib->spting = 1;
                         ib->cpos = COLS - 1;
                    }
               }
               else
                    werase(H.ui.inputwin);

               break;


          case KEY_LEFT:
               if(ib->pos >= 1 && ib->cpos >= 1)
               {
                    --ib->pos;

                    if(ib->spting)
                    {
                         werase(H.ui.inputwin);
                         --ib->split;

                         if(ib->split <= 1)
                              ib->spting = 0;
                    }
                    else
                         --ib->cpos;
               }
               break;

          case KEY_RIGHT:
               if(ib->buffer[ib->pos] != 0)
               {
                    ++ib->pos;

                    if(ib->spting)
                    {
                         werase(H.ui.inputwin);
                         ++ib->split;
                    }
                    else if(ib->cpos == COLS -1 && !ib->spting)
                    {
                         werase(H.ui.inputwin);
                         ++ib->split;
                    }
                    else if(ib->cpos != COLS - 1)
                         ++ib->cpos;
               }
               break;

          case HFTIRC_KEY_DELALL:
               werase(H.ui.inputwin);
               ib->cpos = ib->pos = 0;
               ib->spting = ib->split = 0;
               wmemset(ib->buffer, 0, BUFSIZE);
               break;

          /* Alt-Backspace / ^W, Erase last word */
          case HFTIRC_KEY_ALTBP:
          case CTRLK('w'):
               if(ib->pos > 1)
               {
                    for(i = ib->pos - 1; (i + 1) && ib->buffer[i - 1] != ' '; --i)
                    {
                         --ib->pos;

                         if(ib->spting)
                         {
                              werase(H.ui.inputwin);
                              --ib->split;

                              if(ib->split <= 0)
                                   ib->spting = 0;
                         }
                         else
                              --ib->cpos;

                         wmove(H.ui.inputwin, 0, ib->cpos);

                         if(ib->pos >= 0)
                              for(j = ib->pos;
                                  ib->buffer[j];
                                  ib->buffer[j] = ib->buffer[j + 1], ++j);
                         wdelch(H.ui.inputwin);
                    }
                    ui_get_input();
               }
               break;

          case 127:
          case KEY_BACKSPACE:
               if(ib->pos > 0)
               {
                    --ib->pos;

                    if(ib->spting)
                    {
                         werase(H.ui.inputwin);
                         --ib->split;

                         if(ib->split <= 0)
                              ib->spting = 0;
                    }
                    else
                         --ib->cpos;

                    wmove(H.ui.inputwin, 0, ib->cpos);

                    if(ib->pos >= 0)
                         for(i = ib->pos;
                             ib->buffer[i];
                             ib->buffer[i] = ib->buffer[i + 1], ++i);
                    wdelch(H.ui.inputwin);
               }
               break;

          case HFTIRC_KEY_ENTER:
               if(ib->pos || wcslen(ib->buffer))
               {
                    memset(buf, 0, BUFSIZE);

                    /* Histo */
                    if(ib->nhisto + 1 > HISTOLEN)
                    {
                         for(i = ib->nhisto - 1; i > 1; --i)
                              wcscpy(ib->histo[i], ib->histo[i - 1]);

                         ib->nhisto = 0;
                    }

                    /* Store in histo array */
                    wcscpy(ib->histo[ib->nhisto++], ib->buffer);
                    ib->histpos = 0;
                    wcstombs(buf, ib->buffer, BUFSIZE);
                    input_manage(buf);

                    werase(H.ui.inputwin);
                    wmemset(ib->buffer, 0, BUFSIZE);
                    ib->pos = ib->cpos = ib->split = ib->hits = 0;
               }
               break;

          default:
               if(ib->buffer[ib->pos] != '\0')
                    for(i = (int)wcslen(ib->buffer);
                        i != ib->pos - 1;
                        ib->buffer[i] = ib->buffer[i - 1], --i);

               ib->buffer[ib->pos] = c;

               if(ib->pos >= COLS - 1)
               {
                    ++ib->split;
                    --ib->cpos;
                    ib->spting = 1;
               }

               ib->hits = 1;

               ++ib->pos;
               ++ib->cpos;
               break;

          }
     }

     ib->prev = c;
     werase(H.ui.inputwin);
     ib->cpos = (ib->cpos < 0 ? 0 : ib->cpos);

     for(i = 0; (c = *(ib->buffer + ib->split + i)); ++i)
          hftirc_waddwch(H.ui.inputwin, (i == ib->cpos ? A_REVERSE : A_NORMAL), c);

     if(ib->cpos == wcslen(ib->buffer) || ib->cpos == COLS - 1 || ib->spting)
          waddch(H.ui.inputwin, ' ' | A_REVERSE);

     /* /<num> to go on the buffer num */
     if(ib->buffer[0] == '/')
     {
          if((isdigit((char)ib->buffer[1]) && (n = atoi((char*)&ib->buffer[1])) >= 0 && n < 10)  /* /n   */
             || (ib->buffer[1] == ' ' && (n = atoi((char*)&ib->buffer[2])) > 9))                 /* / nn */
          {
               if((bf = ui_buffer_gb_id(n)))
               {
                    ui_buffer_set(bf);
                    werase(H.ui.inputwin);
                    wmemset(ib->buffer, 0, BUFSIZE);
                    ib->pos = ib->cpos = ib->split = ib->hits = 0;
                    wmove(H.ui.inputwin, 0, 0);
               }
          }
     }

     wrefresh(H.ui.inputwin);
}
