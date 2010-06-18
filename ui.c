/*
 * Copyright (c) 2010 Martin Duquesnoy <xorg62@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "hftirc.h"

/* Will be configurable */
#define COLORMAX      16

/* Colors lists */
#define COLOR_THEME  COLOR_GREEN
#define COLOR_SW     (ui_color(COLOR_BLACK,  COLOR_THEME))
#define COLOR_HL     (ui_color(COLOR_YELLOW, hftirc->ui->bg) | A_BOLD)
#define COLOR_WROTE  (ui_color(COLOR_CYAN, hftirc->ui->bg))
#define COLOR_ACT    (ui_color(COLOR_BLACK,  COLOR_THEME) | A_BOLD | A_UNDERLINE)
#define COLOR_HLACT  (ui_color(COLOR_YELLOW, COLOR_THEME) | A_BOLD | A_UNDERLINE)

void
ui_init(void)
{
     /* Init buffers (only first time) */
     if(hftirc->ft)
     {
          hftirc->selbuf = 0;

          ui_buf_new("status", 0);

          hftirc->ft = 0;
     }

     setlocale(LC_ALL,"");
     initscr();
     raw();
     noecho();
     keypad(stdscr, TRUE);

     /* Check the termnial size */
     if((LINES < 15 || COLS < 35) && hftirc->running == 1)
     {
          endwin();
          fprintf(stderr, "HFTIrc error: Terminal too small (%dx%d)\n"
                    "Minimal size : 15x35\n", LINES, COLS);
          free(hftirc->session);
          free(hftirc->ui);
          free(hftirc);

          exit(EXIT_FAILURE);
     }
     else
          hftirc->running = 2;

     /* Input buffer stuff */
     hftirc->ui->ib.split = 0;

     /* Color support */
     if(has_colors())
          ui_init_color();

     /* Init main window and the borders */
     hftirc->ui->mainwin = newwin(MAINWIN_LINES, COLS, 1, 0);
     scrollok(hftirc->ui->mainwin, TRUE);
     wrefresh(hftirc->ui->mainwin);

     /* Init topic window */
     hftirc->ui->topicwin = newwin(1, COLS, 0, 0);
     wbkgd(hftirc->ui->topicwin, COLOR_SW);
     wrefresh(hftirc->ui->statuswin);

     /* Init input window */
     hftirc->ui->inputwin = newwin(1, COLS, LINES - 1, 0);
     wmove(hftirc->ui->inputwin, 0, 0);
     wrefresh(hftirc->ui->inputwin);
     hftirc->ui->ib.nhisto = 1;
     hftirc->ui->ib.histpos = 0;

     /* Init status window (with the hour / current chan) */
     hftirc->ui->statuswin = newwin(1, COLS, LINES - 2, 0);
     wbkgd(hftirc->ui->statuswin, COLOR_SW);
     wrefresh(hftirc->ui->statuswin);

     refresh();

     return;
}

void
ui_init_color(void)
{
     int i, cn;

     start_color();

     hftirc->ui->bg = ((use_default_colors() == OK) ? -1 : COLOR_BLACK);
     hftirc->ui->c = 1;

     for(cn = ((COLOR_PAIRS >= 256) ? 16 : 8), i = 1; i < ((COLOR_PAIRS >= 256) ? 256 : COLOR_PAIRS); ++i)
          init_pair(i, ((i - 1) % cn), (((i - 1) < cn) ? -1 : (i - 1) / cn));

     hftirc->ui->c = i;

     return;
}


int
ui_color(int fg, int bg)
{
     int i;
     short f, b;

     for(i = 1; i < hftirc->ui->c + 1; ++i)
     {
          pair_content(i, &f, &b);

          if(f == fg && b == bg)
               return COLOR_PAIR(i);
     }

     return 0;
}

void
ui_update_statuswin(void)
{
     int i, x, y;

     /* Erase all window content */
     werase(hftirc->ui->statuswin);

     /* Update bg color */
     wbkgd(hftirc->ui->statuswin, COLOR_SW);

     /* Print date */
     mvwprintw(hftirc->ui->statuswin, 0, 0, "[%s]", hftirc->date.str);

     /* Pseudo with mode */
     mvwprintw(hftirc->ui->statuswin, 0, strlen(hftirc->date.str) + 3, "(");
     PRINTATTR(hftirc->ui->statuswin, A_BOLD, hftirc->conf.serv[hftirc->selses].nick);
     waddch(hftirc->ui->statuswin, '(');
     PRINTATTR(hftirc->ui->statuswin, A_BOLD | A_UNDERLINE, hftirc->conf.serv[hftirc->selses].mode);
     waddstr(hftirc->ui->statuswin, "))");

     /* Info about current serv/channel */
     wprintw(hftirc->ui->statuswin, " (%d:", hftirc->selbuf);
     PRINTATTR(hftirc->ui->statuswin, A_BOLD,  hftirc->conf.serv[hftirc->selses].name);
     waddch(hftirc->ui->statuswin, '/');
     PRINTATTR(hftirc->ui->statuswin, A_BOLD | A_UNDERLINE, hftirc->cb[hftirc->selbuf].name);
     waddch(hftirc->ui->statuswin, ')');

     /* Activity */
     wprintw(hftirc->ui->statuswin, " (Bufact: ");
     for(i = 0; i < hftirc->nbuf; ++i)
          if(hftirc->cb[i].act)
          {
               wattron(hftirc->ui->statuswin,
                         ((hftirc->cb[i].act == 2) ? COLOR_HLACT : COLOR_ACT));
               wprintw(hftirc->ui->statuswin, "%d", i);
               wattroff(hftirc->ui->statuswin,
                         ((hftirc->cb[i].act == 2) ? COLOR_HLACT : COLOR_ACT));
               waddch(hftirc->ui->statuswin, ' ');
          }

     /* Remove last char in () -> a space and put the ) instead it */
     getyx(hftirc->ui->statuswin, x, y);
     wmove(hftirc->ui->statuswin, x, y - 1);
     waddch(hftirc->ui->statuswin, ')');

     /* Print hftirc version */
     mvwprintw(hftirc->ui->statuswin, 0,
               COLS - strlen("HFTIrc "HFTIRC_VERSION),
               "HFTIrc "HFTIRC_VERSION);

     wrefresh(hftirc->ui->statuswin);

     return;
}

void
ui_update_topicwin(void)
{
     /* Erase all window content */
     werase(hftirc->ui->topicwin);

     /* Update bg color */
     wbkgd(hftirc->ui->topicwin, COLOR_SW);

     /* Write topic */
     waddstr(hftirc->ui->topicwin, hftirc->cb[hftirc->selbuf].topic);

     wrefresh(hftirc->ui->topicwin);

     return;
}


void
ui_print(WINDOW *w, char *str)
{
     int i, mask = A_NORMAL;
     char *p, nick[128] = { 0 };

     if(!str || !w)
          return;

     /* Wrote lines */
     if(hftirc->conf.serv && hftirc->selbuf != 0 && strlen(str)
               && (p = strchr(str, '<')) && sscanf(p, "%128s", nick)
               && strstr(nick, hftirc->conf.serv[hftirc->selses].nick))
          mask |= COLOR_WROTE;

     /* Highlight line */
     if(hftirc->conf.serv && hftirc->selbuf != 0 && strlen(str)
               && strchr(str, '<') && strchr(str, '>')
               && strstr(str + strlen(hftirc->date.str) + 4,  hftirc->conf.serv[hftirc->selses].nick))
     {
          mask &= ~(COLOR_WROTE);
          mask |= COLOR_HL;
     }

     for(i = 0; i < strlen(str); ++i)
     {
          switch(str[i])
          {
               /* Bold */
               case B:
                    mask ^= A_BOLD;
                    break;

               /* Underline */
               case U:
                    mask ^= A_UNDERLINE;
                    break;

               /* mIRC©®™ colors (not needed for now.) */
               case C('c'):
                    break;

               default:
                    /* simple waddch doesn't work with some char */
                    wattron(w, mask);
                    wprintw(w, "%c", str[i]);
                    wattroff(w, mask);

                    break;
          }
     }

     return;
}

void
ui_print_buf(int id, char *format, ...)
{
     va_list ap;
     char *buf, *p;

     if(id < 0 || id > hftirc->nbuf - 1)
          return;

     va_start(ap, format);
     vasprintf(&p, format, ap);

     buf = malloc(sizeof(char) * strlen(p) + strlen(hftirc->date.str) + 3);
     va_end(ap);

     sprintf(buf, "%s %s\n", hftirc->date.str, p);

     hftirc->cb[id].buffer[hftirc->cb[id].bufpos] = strdup(buf);

     hftirc->cb[id].bufpos = (hftirc->cb[id].bufpos < BUFLINES - 1) ? hftirc->cb[id].bufpos + 1 : 0;

     if(id == hftirc->selbuf && !hftirc->cb[id].scrollpos)
     {
          ui_print(hftirc->ui->mainwin, buf);
          wrefresh(hftirc->ui->mainwin);
     }

     /* Activity management:
      *   1: Normal acitivity on the buffer (talking, info..)
      *   2: Highlight activity on the buffer
      */
     if(id != hftirc->selbuf)
     {
          if(hftirc->cb[id].act != 2)
               hftirc->cb[id].act = 1;

          /* Highlight test (if hl or private message) */
          if(hftirc->conf.serv && id && ((strchr(buf, '<') && strchr(buf, '>')
                              && strstr(buf + strlen(hftirc->date.str) + 4, hftirc->conf.serv[hftirc->selses].nick))
                         || (hftirc->cb[id].name[0] != '#' && hftirc->cb[id].name[0] != '&')))
               /* No HL on status buffer (0) */
               hftirc->cb[id].act = (id) ? 2 : 1;
     }

     free(buf);
     free(p);

     return;
}

void
ui_draw_buf(int id)
{
     int i = 0;

     if(id < 0 || id > hftirc->nbuf - 1)
          return;

     werase(hftirc->ui->mainwin);

     i = (hftirc->cb[id].bufpos + hftirc->cb[id].scrollpos) - MAINWIN_LINES;

     for(; i < (hftirc->cb[id].bufpos + hftirc->cb[id].scrollpos); ++i)
          if(i < BUFLINES)
               ui_print(hftirc->ui->mainwin, ((i >= 0) ? hftirc->cb[id].buffer[i] : "\n"));

     wrefresh(hftirc->ui->mainwin);

     return;
}

void
ui_buf_set(int buf)
{
     if(buf < 0 || buf > hftirc->nbuf - 1)
          return;

     hftirc->selbuf = buf;
     hftirc->selses = hftirc->cb[buf].sessid;
     hftirc->cb[buf].act = 0;

     ui_draw_buf(buf);

     return;
}

void
ui_buf_new(const char *name, unsigned int id)
{
     int i, j;
     ChanBuf *cbs = NULL;

     if(!strlen(name))
          name = strdup("???");

     ++hftirc->nbuf;
     i = hftirc->nbuf - 1;

     cbs = calloc(hftirc->nbuf, sizeof(ChanBuf));

     for(j = 0; j < i; cbs[j] = hftirc->cb[j], ++j);

     free(hftirc->cb);

     memset(cbs[i].topic, 0, sizeof(cbs[i].topic));
     strcpy(cbs[i].name, name);

     SLIST_INIT(&cbs[i].nickhead);

     for(j = 0; j < BUFLINES;cbs[i].buffer[j++] = NULL);

     cbs[i].bufpos = cbs[i].scrollpos = cbs[i].act = 0;
     cbs[i].sessid = id;
     cbs[i].naming = 0;

     hftirc->cb = calloc(hftirc->nbuf + 1, sizeof(ChanBuf));

     for(i = 0; i < hftirc->nbuf; hftirc->cb[i] = cbs[i], ++i);

     free(cbs);

     ui_buf_set(i);

     return;
}

void
ui_buf_close(int buf)
{
     int i, n;
     ChanBuf *cbs = NULL;

     if(buf <= 0 || buf > hftirc->nbuf - 1)
          return;

     --hftirc->nbuf;

     cbs = calloc(hftirc->nbuf, sizeof(ChanBuf));

     for(i = n = 0; i < hftirc->nbuf + 1; ++i)
          if(i != buf)
               cbs[n++] = hftirc->cb[i];

     free(hftirc->cb);

     hftirc->cb = calloc(hftirc->nbuf, sizeof(ChanBuf));

     for(i = 0; i < hftirc->nbuf; hftirc->cb[i] = cbs[i], ++i);

     free(cbs);

     if(hftirc->selbuf == buf)
          ui_buf_set(buf - 1);

     return;
}

void
ui_scroll_up(int buf)
{
     if(buf < 0 || buf > hftirc->nbuf - 1
               || hftirc->cb[buf].bufpos + hftirc->cb[buf].scrollpos - 1 < 0)
          return;

     hftirc->cb[buf].scrollpos -= 2;

     if(buf == hftirc->selbuf)
          ui_draw_buf(buf);

     return;
}

void
ui_scroll_down(int buf)
{
     if(buf < 0 || buf > hftirc->nbuf - 1
               || hftirc->cb[buf].scrollpos >= 0)
          return;

     hftirc->cb[buf].scrollpos += 2;

     if(buf == hftirc->selbuf)
          ui_draw_buf(buf);

     return;
}

void
ui_get_input(void)
{
     int i, t;
     wint_t c;
     char buf[BUFSIZE];

     switch((t = get_wch(&c)))
     {
          case ERR: break;
          default:
               switch(c)
               {
                    case KEY_F(1):
                         ui_buf_set(hftirc->selbuf - 1);
                         break;

                    case KEY_F(2):
                         ui_buf_set(hftirc->selbuf + 1);
                         break;

                    case KEY_PPAGE:
                         ui_scroll_up(hftirc->selbuf);
                         break;

                    case KEY_NPAGE:
                         ui_scroll_down(hftirc->selbuf);
                         break;

                    case HFTIRC_KEY_ENTER:
                         if(hftirc->ui->ib.pos || wcslen(hftirc->ui->ib.buffer))
                         {
                              memset(buf, 0, BUFSIZE);

                              /* Histo */
                              if(hftirc->ui->ib.nhisto + 1 > HISTOLEN)
                              {
                                   for(i = hftirc->ui->ib.nhisto - 1; i > 1; --i)
                                        wcscpy(hftirc->ui->ib.histo[i], hftirc->ui->ib.histo[i - 1]);

                                   hftirc->ui->ib.nhisto = 0;
                              }
                              /* Store in histo array */
                              wcscpy(hftirc->ui->ib.histo[hftirc->ui->ib.nhisto++], hftirc->ui->ib.buffer);
                              hftirc->ui->ib.histpos = 0;

                              wcstombs(buf, hftirc->ui->ib.buffer, BUFSIZE);
                              input_manage(buf);
                              werase(hftirc->ui->inputwin);
                              wmemset(hftirc->ui->ib.buffer, 0, BUFSIZE);
                              hftirc->ui->ib.pos = hftirc->ui->ib.cpos = hftirc->ui->ib.split = 0;
                              wmove(hftirc->ui->inputwin, 0, 0);
                         }
                         break;

                    case KEY_UP:
                         if(hftirc->ui->ib.nhisto)
                         {
                              if(hftirc->ui->ib.histpos >= hftirc->ui->ib.nhisto)
                                   hftirc->ui->ib.histpos = 0;

                              wmemset(hftirc->ui->ib.buffer, 0, BUFSIZE);
                              wcscpy(hftirc->ui->ib.buffer, hftirc->ui->ib.histo[hftirc->ui->ib.nhisto - ++hftirc->ui->ib.histpos]);
                              werase(hftirc->ui->inputwin);
                              wmove(hftirc->ui->inputwin, 0, (hftirc->ui->ib.cpos = hftirc->ui->ib.pos = wcslen(hftirc->ui->ib.buffer)));
                         }
                         break;

                    case KEY_DOWN:
                         if(hftirc->ui->ib.nhisto && hftirc->ui->ib.histpos > 0
                                   && hftirc->ui->ib.histpos < hftirc->ui->ib.nhisto)
                         {
                              wmemset(hftirc->ui->ib.buffer, 0, BUFSIZE);
                              wcscpy(hftirc->ui->ib.buffer, hftirc->ui->ib.histo[hftirc->ui->ib.nhisto - --hftirc->ui->ib.histpos]);
                              werase(hftirc->ui->inputwin);
                              wmove(hftirc->ui->inputwin, 0, (hftirc->ui->ib.cpos = hftirc->ui->ib.pos = wcslen(hftirc->ui->ib.buffer)));
                         }
                         else
                              werase(hftirc->ui->inputwin);

                         break;

                    case KEY_LEFT:
                         if(hftirc->ui->ib.pos >= 1
                           && hftirc->ui->ib.cpos >= 1)
                         {
                              --(hftirc->ui->ib.pos);

                              if(hftirc->ui->ib.spting)
                              {
                                    werase(hftirc->ui->inputwin);
                                   --(hftirc->ui->ib.split);

                                   if(hftirc->ui->ib.split <= 1)
                                        hftirc->ui->ib.spting = 0;
                              }
                              else
                                   --(hftirc->ui->ib.cpos);

                              wmove(hftirc->ui->inputwin, 0, hftirc->ui->ib.cpos);
                         }
                         break;

                    case KEY_RIGHT:
                         if(hftirc->ui->ib.buffer[hftirc->ui->ib.pos] != 0)
                         {
                              ++(hftirc->ui->ib.pos);

                              if(hftirc->ui->ib.spting)
                              {
                                   werase(hftirc->ui->inputwin);
                                   ++(hftirc->ui->ib.split);
                              }
                              else if(hftirc->ui->ib.cpos == COLS -1
                                        && !hftirc->ui->ib.spting)
                              {
                                   werase(hftirc->ui->inputwin);
                                   ++(hftirc->ui->ib.split);
                              }
                              else if(hftirc->ui->ib.cpos != COLS - 1)
                                   ++(hftirc->ui->ib.cpos);


                              wmove(hftirc->ui->inputwin, 0, hftirc->ui->ib.cpos);
                         }
                         break;

                    case 127:
                    case KEY_BACKSPACE:
                         if(hftirc->ui->ib.pos >= 1)
                         {
                              --(hftirc->ui->ib.pos);

                              if(hftirc->ui->ib.spting)
                              {
                                   werase(hftirc->ui->inputwin);
                                   --(hftirc->ui->ib.split);

                                   if(hftirc->ui->ib.split <= 1)
                                        hftirc->ui->ib.spting = 0;
                              }
                              else
                                   --(hftirc->ui->ib.cpos);

                              wmove(hftirc->ui->inputwin, 0, hftirc->ui->ib.cpos);

                              for(i = hftirc->ui->ib.pos;
                                        hftirc->ui->ib.buffer[i];
                                        hftirc->ui->ib.buffer[i] = hftirc->ui->ib.buffer[i + 1], ++i);
                              wdelch(hftirc->ui->inputwin);
                         }
                         break;

                    case KEY_DC:
                         wdelch(hftirc->ui->inputwin);

                         if(hftirc->ui->ib.buffer[hftirc->ui->ib.pos] != 0 && hftirc->ui->ib.pos >= 0)
                              for(i = hftirc->ui->ib.pos;
                                        hftirc->ui->ib.buffer[i];
                                        hftirc->ui->ib.buffer[i] = hftirc->ui->ib.buffer[i + 1], ++i);

                         break;

                    case KEY_HOME:
                         wmove(hftirc->ui->inputwin, 0, 0);
                         hftirc->ui->ib.pos = hftirc->ui->ib.cpos = 0;
                         hftirc->ui->ib.spting = hftirc->ui->ib.split = 0;
                         break;

                    case KEY_END:
                         wmove(hftirc->ui->inputwin, 0, (int)wcslen(hftirc->ui->ib.buffer));
                         hftirc->ui->ib.pos = (int)wcslen(hftirc->ui->ib.buffer);

                         if(hftirc->ui->ib.spting
                                   || (int)wcslen(hftirc->ui->ib.buffer) > COLS - 1)
                         {
                              werase(hftirc->ui->inputwin);
                              hftirc->ui->ib.spting = 1;
                              hftirc->ui->ib.cpos = COLS - 1;
                              hftirc->ui->ib.split = (int)wcslen(hftirc->ui->ib.buffer) - COLS + 1;
                         }
                         else
                              hftirc->ui->ib.cpos = (int)wcslen(hftirc->ui->ib.buffer);
                         break;

                    case KEY_RESIZE:
                         break;

                    default:
                         if(c > 0 && wcslen(hftirc->ui->ib.buffer) < BUFSIZE)
                         {
                              if(hftirc->ui->ib.buffer[hftirc->ui->ib.pos] != '\0')
                                   for(i = (int)wcslen(hftirc->ui->ib.buffer);
                                             i != hftirc->ui->ib.pos - 1;
                                             hftirc->ui->ib.buffer[i] = hftirc->ui->ib.buffer[i - 1], --i);

                              hftirc->ui->ib.buffer[hftirc->ui->ib.pos] = c;

                              if(hftirc->ui->ib.cpos >= COLS - 1)
                              {
                                   ++hftirc->ui->ib.split;
                                   --hftirc->ui->ib.cpos;
                                   hftirc->ui->ib.spting = 1;
                              }

                              ++(hftirc->ui->ib.pos);
                              ++(hftirc->ui->ib.cpos);
                              wmove(hftirc->ui->inputwin, 0, hftirc->ui->ib.cpos);
                         }
                         break;
               }
               break;
     }

     mvwaddwstr(hftirc->ui->inputwin, 0, 0,
               hftirc->ui->ib.buffer + hftirc->ui->ib.split);

     wrefresh(hftirc->ui->inputwin);

     return;
}
