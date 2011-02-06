/*
 * copyright (c) 2010 Martin Duquesnoy <xorg62@gmail.com>
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
#define ROSTERSIZE    20

/* Some keys */
#define HFTIRC_KEY_ENTER  (10)
#define HFTIRC_KEY_ALTBP  (27)
#define HFTIRC_END_COLOR  (15)

/* Test control-bind */
#define IS_CTRLK(c)  (c > 0 && c < 32)

/* Colors lists */
#define COLOR_THEME_DEFAULT  COLOR_BLUE
#define COLOR_SW      (ui_color(COLOR_BLACK, hftirc->ui->tcolor))
#define COLOR_SW2     (ui_color(COLOR_WHITE, hftirc->ui->tcolor))
#define COLOR_HL      (ui_color(COLOR_YELLOW, hftirc->ui->bg) | A_BOLD)
#define COLOR_WROTE   (ui_color(COLOR_CYAN, hftirc->ui->bg))
#define COLOR_ROSTER  (ui_color(hftirc->ui->tcolor, hftirc->ui->bg))
#define COLOR_ACT     (ui_color(COLOR_WHITE,  hftirc->ui->tcolor) | A_UNDERLINE)
#define COLOR_HLACT   (ui_color(COLOR_RED, hftirc->ui->tcolor) | A_BOLD | A_UNDERLINE)
#define COLOR_LASTPOS (ui_color(COLOR_BLUE, hftirc->ui->bg | A_BOLD ))

/* mIRC colours:
 * From http://www.mirc.net/newbie/colors.php:
 *
 *  0  white	 8  yellow
 *  1  black	 9  lightgreen
 *  2  blue	 10  cyan
 *  3  green	 11	lightcyan
 *  4  lightred 12	lightblue
 *  5  brown	 13  pink
 *  6  purple	 14  grey
 *  7  orange	 15  lightg
 *
 */
const struct { int c, m; } irccol[] =
{
     { COLOR_WHITE,   A_BOLD   },
     { COLOR_BLACK,   A_NORMAL },
     { COLOR_BLUE,    A_NORMAL },
     { COLOR_GREEN,   A_NORMAL },
     { COLOR_RED,     A_BOLD   },
     { COLOR_RED,     A_NORMAL },
     { COLOR_MAGENTA, A_NORMAL },
     { COLOR_YELLOW,  A_NORMAL },
     { COLOR_YELLOW,  A_BOLD   },
     { COLOR_GREEN,   A_BOLD   },
     { COLOR_CYAN,    A_NORMAL },
     { COLOR_CYAN,    A_BOLD   },
     { COLOR_BLUE,    A_BOLD   },
     { COLOR_MAGENTA, A_BOLD   },
     { COLOR_BLACK,   A_BOLD   },
     { COLOR_WHITE,   A_NORMAL },
};

void
ui_init(void)
{
     /* Init buffers (only first time) */
     if(hftirc->ft)
     {
          hftirc->selbuf = 0;
          hftirc->ui->nicklist = hftirc->conf.nicklist;

          ui_buf_new("status", 0);

          hftirc->ft = 0;
     }

     setlocale(LC_ALL, "");
     initscr();
     raw();
     noecho();
     keypad(stdscr, TRUE);
     curs_set(FALSE);

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
     hftirc->ui->mainwin = newwin(MAINWIN_LINES, COLS - (hftirc->ui->nicklist ? ROSTERSIZE : 0), 1, 0);
     scrollok(hftirc->ui->mainwin, TRUE);
     wrefresh(hftirc->ui->mainwin);

     /* Init topic window */
     hftirc->ui->topicwin = newwin(1, COLS, 0, 0);
     wbkgd(hftirc->ui->topicwin, COLOR_SW);
     wrefresh(hftirc->ui->statuswin);

     /* Init nicklist window */
     hftirc->ui->nicklistwin = newwin(LINES - 3, ROSTERSIZE, 1, COLS - ROSTERSIZE);
     wrefresh(hftirc->ui->nicklistwin);

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
     int i, j, n = 0;

     start_color();

     hftirc->ui->bg = ((use_default_colors() == OK) ? -1 : COLOR_BLACK);
     hftirc->ui->c = 1;

     for(i = 0; i < 8; ++i)
          for(j = 0; j < 8; ++j)
               init_pair(++n, j, (!i ? hftirc->ui->bg : i));

     hftirc->ui->c = n;

     hftirc->ui->tcolor = hftirc->conf.tcolor;

     return;
}

int
ui_color(int fg, int bg)
{
     int i;
     short f, b;

     if(bg == COLOR_BLACK && hftirc->ui->bg != COLOR_BLACK)
          bg = hftirc->ui->bg;

     for(i = 0; i < hftirc->ui->c + 1; ++i)
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
     int i, j, c, x, y;

     /* Erase all window content */
     werase(hftirc->ui->statuswin);

     /* Update bg color */
     wbkgd(hftirc->ui->statuswin, COLOR_SW);

     /* Print date */
     mvwprintw(hftirc->ui->statuswin, 0, 0, "[%s]", hftirc->date.str);

     /* Pseudo with mode */
     mvwprintw(hftirc->ui->statuswin, 0, strlen(hftirc->date.str) + 3, "(");
     PRINTATTR(hftirc->ui->statuswin, COLOR_SW2, hftirc->session[hftirc->selses]->nick);
     waddch(hftirc->ui->statuswin, '(');
     PRINTATTR(hftirc->ui->statuswin, A_UNDERLINE | COLOR_SW2, hftirc->conf.serv[hftirc->selses].mode);
     waddstr(hftirc->ui->statuswin, "))");

     /* Info about current serv/channel */
     wprintw(hftirc->ui->statuswin, " (%d:", hftirc->selbuf);
     PRINTATTR(hftirc->ui->statuswin, COLOR_SW2,  hftirc->conf.serv[hftirc->selses].name);

     /* if connected or not ( {} needed for macro ) */
     if(!hftirc->session[hftirc->selses]->connected)
     {
          PRINTATTR(hftirc->ui->statuswin, A_BOLD, " (Disconnected)");
     }

     waddch(hftirc->ui->statuswin, '/');
     PRINTATTR(hftirc->ui->statuswin, A_UNDERLINE | COLOR_SW2, hftirc->cb[hftirc->selbuf].name);
     waddch(hftirc->ui->statuswin, ')');

     /* Activity */
     wprintw(hftirc->ui->statuswin, " (Bufact: ");

     /* First pritority is when ISCHAN(..) == 0, second for == j.
      * Priority: Private conversation, highlight channel, and normal active channel.
      */
     for(j = 0; j < 2; ++j)
          for ( c = 2; c > 0; --c )
               for(i = 0; i < hftirc->nbuf; ++i)
                    if(ISCHAN(hftirc->cb[i].name[0]) == j
                              && hftirc->cb[i].act == c)
                    {
                         wattron(hftirc->ui->statuswin, ((c == 2) ? COLOR_HLACT : COLOR_ACT));
                         wprintw(hftirc->ui->statuswin, "%d", i);
                         wattroff(hftirc->ui->statuswin, A_UNDERLINE);
                         wprintw(hftirc->ui->statuswin, ":%s", hftirc->cb[i].name);
                         wattroff(hftirc->ui->statuswin, ((c == 2) ? COLOR_HLACT : COLOR_ACT));
                         waddch(hftirc->ui->statuswin, ' ');
                    }

     /* Remove last char in () -> a space and put the ) instead it */
     getyx(hftirc->ui->statuswin, x, y);
     wmove(hftirc->ui->statuswin, x, y - 1);
     waddch(hftirc->ui->statuswin, ')');

     wrefresh(hftirc->ui->statuswin);

     return;
}

void
ui_update_topicwin(void)
{
     /* Check if this is needed */
     if(!(hftirc->cb[hftirc->selbuf].umask & UTopicMask))
          return;

     /* Erase all window content */
     werase(hftirc->ui->topicwin);

     /* Update bg color */
     wbkgd(hftirc->ui->topicwin, COLOR_SW);

     /* Write topic */
     /*   Channel   */
     if(ISCHAN(hftirc->cb[hftirc->selbuf].name[0]))
          waddstr(hftirc->ui->topicwin, hftirc->cb[hftirc->selbuf].topic);
     /*   Other    */
     else
          wprintw(hftirc->ui->topicwin, "%s (%s)",
                    hftirc->cb[hftirc->selbuf].name, hftirc->conf.serv[hftirc->selses].name);

     wrefresh(hftirc->ui->topicwin);

     hftirc->cb[hftirc->selbuf].umask &= ~UTopicMask;

     return;
}

void
ui_update_nicklistwin(void)
{
     int i, c, p;
     char ord[4] = { '@', '%', '+', '\0' };
     NickStruct *ns;

     nick_sort_abc(hftirc->selbuf);

     if(!hftirc->ui->nicklist
               || !(hftirc->cb[hftirc->selbuf].umask & UNickListMask))
          return;

     werase(hftirc->ui->nicklistwin);

     /* Travel in nick linked list */
     /* Order with [ord]:
      *  | @foo
      *  | %foo
      *  | +foo
      *  |  foo
      */
     for(i = c = p = 0; i < LEN(ord); ++i)
     {
          for(ns = hftirc->cb[hftirc->selbuf].nickhead;
                    ns && c < ((LINES - 3) + hftirc->cb[hftirc->selbuf].nicklistscroll); /* </// Scroll limit */
                    ns = ns->next)
          {
               if(ns->rang == ord[i])
               {
                    if(p >= hftirc->cb[hftirc->selbuf].nicklistscroll)
                         wprintw(hftirc->ui->nicklistwin, " %c%s\n", (ns->rang ? ns->rang : ' '), ns->nick);

                    ++c;
                    ++p;
               }
          }
     }

     /* Draw | separation bar */
     wattron(hftirc->ui->nicklistwin, COLOR_ROSTER);

     for(i = 0; i < LINES - 3; ++i)
          mvwaddch(hftirc->ui->nicklistwin, i, 0, ACS_VLINE);

     wattroff(hftirc->ui->nicklistwin, COLOR_ROSTER);

     wrefresh(hftirc->ui->nicklistwin);

     hftirc->cb[hftirc->selbuf].umask &= ~UNickListMask;

     return;
}

void
ui_print(WINDOW *w, char *str, int n)
{
     int i;
     int hmask = A_NORMAL, mask = A_NORMAL, lastposmask = A_NORMAL;
     int fg = 15, bg  = 1, mcol = 0, lcol = 0;
     char *p, nick[128] = { 0 };

     if(!str || !w)
          return;

     /* Wrote lines */
     if(hftirc->conf.serv && hftirc->selbuf != 0 && strlen(str)
               && (p = strchr(str, '<')) && sscanf(p, "%128s", nick)
               && strstr(nick, hftirc->session[hftirc->selses]->nick))
          mask |= COLOR_WROTE;

     /* Highlight line */
     if(hftirc->conf.serv && hftirc->selbuf != 0 && strlen(str)
               && strchr(str, '<') && strchr(str, '>')
               && strcasestr(str + strlen(hftirc->date.str) + 4,  hftirc->session[hftirc->selses]->nick))
     {
          mask &= ~(COLOR_WROTE);
          mask |= COLOR_HL;
     }

     /* Last position tracker with bold line */
     if(hftirc->conf.lastlinepos && hftirc->cb[hftirc->selbuf].lastposbold == n)
          lastposmask |= COLOR_LASTPOS;

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

               /* Cancel masks */
               case HFTIRC_END_COLOR:
                    hmask &= ~lcol;
                    mask = 0;
                    break;

               /* mIRC©®™ colors */
               case C('c'):
                    if(lcol)
                        hmask &= ~lcol;

                    /* Set fg color first if there is no coma */
                    if(isdigit(str[i + 1]))
                    {
                         fg = (str[i + 1] - '0');
                         ++i;

                         if(isdigit(str[i + 1]))
                         {
                              fg = fg * 10 + (str[i + 1] - '0');
                              ++i;
                         }
                         mcol = 1;
                    }

                    /* bg color if coma */
                    if(str[i + 1] == ',' && isdigit(str[i + 2]))
                    {
                         bg = (str[i + 2] - '0');
                         i += 2;

                         if(isdigit(str[i + 1]))
                         {
                              bg = bg * 10 + (str[i + 1] - '0');
                              ++i;
                         }

                         if(fg == bg)
                              fg = 1;
                         mcol = 1;
                    }

                    if(mcol)
                            hmask ^= (lcol = (ui_color(irccol[fg % COLORMAX].c, irccol[bg % COLORMAX].c)
                                           | irccol[fg % COLORMAX].m));

                    break;

               default:
                    /* simple waddch doesn't work with some char */
                    wattron(w, hmask | mask | lastposmask);
                    wprintw(w, "%c", str[i]);
                    wattroff(w, hmask | mask | lastposmask);

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
          ui_print(hftirc->ui->mainwin, buf, 0);
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
          if(hftirc->conf.serv && id  && ((((strchr(buf, '<') && strchr(buf, '>')) || strchr(buf, '*'))
                              && strcasestr(buf + strlen(hftirc->date.str) + 4, hftirc->session[hftirc->selses]->nick))
                         || !ISCHAN(hftirc->cb[id].name[0])))
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

     i = (hftirc->cb[id].bufpos + hftirc->cb[id].scrollpos) - MAINWIN_LINES;

     for(; i < (hftirc->cb[id].bufpos + hftirc->cb[id].scrollpos); ++i)
          if(i < BUFLINES)
          {
               if(i < 0 && hftirc->cb[id].buffer[BUFLINES + i])
                    ui_print(hftirc->ui->mainwin, hftirc->cb[id].buffer[BUFLINES + i], BUFLINES + i);
               else
                    ui_print(hftirc->ui->mainwin, ((i >= 0) ? hftirc->cb[id].buffer[i] : "\n"), i);
          }

     wrefresh(hftirc->ui->mainwin);

     return;
}

void
ui_buf_set(int buf)
{
     if(buf < 0 || buf > hftirc->nbuf - 1)
          return;

     hftirc->cb[hftirc->selbuf].lastposbold = hftirc->cb[hftirc->selbuf].bufpos - 1;
     hftirc->prevbuf = hftirc->selbuf;
     hftirc->selbuf = buf;
     hftirc->selses = hftirc->cb[buf].sessid;
     hftirc->cb[buf].act = 0;
     hftirc->cb[buf].umask |= (UTopicMask | UNickListMask);

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

     for(j = 0; j < BUFLINES; cbs[i].buffer[j++] = NULL);

     cbs[i].bufpos = cbs[i].scrollpos = cbs[i].act = 0;
     cbs[i].naming = cbs[i].nicklistscroll = 0;
     cbs[i].lastposbold = -1;
     cbs[i].sessid = id;
     cbs[i].umask |= (UTopicMask | UNickListMask);

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

     ui_buf_set(hftirc->prevbuf);

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
     if(buf < 0
               || buf > hftirc->nbuf - 1
               || hftirc->cb[buf].scrollpos >= 0
               || hftirc->cb[buf].scrollpos > BUFLINES)
          return;

     hftirc->cb[buf].scrollpos += 2;

     if(buf == hftirc->selbuf)
          ui_draw_buf(buf);

     return;
}

void
ui_nicklist_toggle(void)
{
     delwin(hftirc->ui->mainwin);

     if((hftirc->ui->nicklist = !hftirc->ui->nicklist))
     {
          hftirc->ui->nicklistwin = newwin(LINES - 3, ROSTERSIZE, 1, COLS - ROSTERSIZE);
          wrefresh(hftirc->ui->nicklistwin);
          hftirc->ui->mainwin = newwin(MAINWIN_LINES, COLS - ROSTERSIZE, 1, 0);
          hftirc->cb[hftirc->selbuf].umask |= UNickListMask;
          ui_update_nicklistwin();
     }
     else
     {
          delwin(hftirc->ui->nicklistwin);
          hftirc->ui->mainwin = newwin(MAINWIN_LINES, COLS, 1, 0);
     }

     scrollok(hftirc->ui->mainwin, TRUE);

     ui_draw_buf(hftirc->selbuf);

     return;
}

void
ui_nicklist_scroll(int v)
{
     if(hftirc->cb[hftirc->selbuf].nicklistscroll + v < 0
               || hftirc->cb[hftirc->selbuf].nicklistscroll + v > hftirc->cb[hftirc->selbuf].nnick + 2)
          return;

     hftirc->cb[hftirc->selbuf].nicklistscroll += v;

     hftirc->cb[hftirc->selbuf].umask |= UNickListMask;
     ui_update_nicklistwin();

     return;
}

void
ui_buf_swap(int n)
{
    ChanBuf old_buffer;

    if(!n || !hftirc->selbuf || hftirc->selbuf == 0
          ||  n <= 0 || n > hftirc->nbuf -1
          || n == hftirc->selbuf)
    {
        return;
    }

    old_buffer = hftirc->cb[ n ];
    hftirc->cb[ n ] = hftirc->cb[ hftirc->selbuf ];
    hftirc->cb[ hftirc->selbuf ] = old_buffer;

    ui_buf_set(n);

    return;
}

void
ui_set_color_theme(int col)
{
     if(col < -1 || col > 7)
          return;

     hftirc->ui->tcolor = col;
     hftirc->cb[hftirc->selbuf].umask |= (UTopicMask | UNickListMask);

     return;
}

void
ui_refresh_curpos(void)
{
     wchar_t wc;

     /* Draw cursor */
     wmove(hftirc->ui->inputwin, 0, hftirc->ui->ib.cpos);
     hftirc_waddwch(hftirc->ui->inputwin, A_REVERSE,
               (!(wc = *(hftirc->ui->ib.buffer + hftirc->ui->ib.pos))
                ? ' ' : wc));

     wrefresh(hftirc->ui->inputwin);

     return;
}

void
ui_get_input(void)
{
     int i, j, n, b = 1, t;
     wint_t c;
     wchar_t tmpbuf[BUFSIZE], *cmp;
     char buf[BUFSIZE];

     switch((t = get_wch(&c)))
     {
          case ERR: break;
          default:
               switch(c)
               {
                    case KEY_F(1):
                    case C('p'):
                         ui_buf_set(hftirc->selbuf - 1);
                         break;

                    case KEY_F(2):
                    case C('n'):
                         ui_buf_set(hftirc->selbuf + 1);
                         break;

                    case KEY_F(3):
                         ui_nicklist_toggle();
                         break;

                    case KEY_F(11):
                         ui_nicklist_scroll(-3);
                         break;

                    case KEY_F(12):
                         ui_nicklist_scroll(+3);
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
                              hftirc->ui->ib.pos = hftirc->ui->ib.cpos = hftirc->ui->ib.split = hftirc->ui->ib.hits = 0;
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
                              hftirc->ui->ib.cpos = hftirc->ui->ib.pos = wcslen(hftirc->ui->ib.buffer);

                              if(hftirc->ui->ib.pos >= COLS - 1)
                              {
                                   hftirc->ui->ib.split = hftirc->ui->ib.pos - (COLS - 1);
                                   hftirc->ui->ib.spting = 1;
                                   hftirc->ui->ib.cpos = COLS - 1;
                              }

                              /* Ctrl-key are 2 char long */
                              for(i = 0; i < wcslen(hftirc->ui->ib.buffer); ++i)
                                   if(IS_CTRLK(hftirc->ui->ib.buffer[i]))
                                   {
                                        ++hftirc->ui->ib.cpos;
                                        if(hftirc->ui->ib.spting)
                                             ++hftirc->ui->ib.split;
                                   }
                         }
                         break;

                    case KEY_DOWN:
                         if(hftirc->ui->ib.nhisto && hftirc->ui->ib.histpos > 0
                                   && hftirc->ui->ib.histpos < hftirc->ui->ib.nhisto)
                         {
                              wmemset(hftirc->ui->ib.buffer, 0, BUFSIZE);
                              wcscpy(hftirc->ui->ib.buffer, hftirc->ui->ib.histo[hftirc->ui->ib.nhisto - --hftirc->ui->ib.histpos]);
                              werase(hftirc->ui->inputwin);
                              hftirc->ui->ib.cpos = hftirc->ui->ib.pos = wcslen(hftirc->ui->ib.buffer);

                              if(hftirc->ui->ib.pos >= COLS - 1)
                              {
                                   hftirc->ui->ib.split = hftirc->ui->ib.pos - (COLS - 1);
                                   hftirc->ui->ib.spting = 1;
                                   hftirc->ui->ib.cpos = COLS - 1;
                              }

                              /* Ctrl-key are 2 char long */
                              for(i = 0; i < wcslen(hftirc->ui->ib.buffer); ++i)
                                   if(IS_CTRLK(hftirc->ui->ib.buffer[i]))
                                   {
                                        ++hftirc->ui->ib.cpos;
                                        if(hftirc->ui->ib.spting)
                                             ++hftirc->ui->ib.split;
                                   }
                         }
                         else
                              werase(hftirc->ui->inputwin);

                         break;

                    case KEY_LEFT:
                         if(hftirc->ui->ib.pos >= 1
                           && hftirc->ui->ib.cpos >= 1)
                         {
                              if(IS_CTRLK(hftirc->ui->ib.buffer[hftirc->ui->ib.pos - 1]))
                                   --(hftirc->ui->ib.cpos);

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
                         }
                         break;

                    case KEY_RIGHT:
                         if(hftirc->ui->ib.buffer[hftirc->ui->ib.pos] != 0)
                         {
                              if(IS_CTRLK(hftirc->ui->ib.buffer[hftirc->ui->ib.pos]))
                                   ++(hftirc->ui->ib.cpos);

                              ++(hftirc->ui->ib.pos);

                              if(hftirc->ui->ib.spting)
                              {
                                   werase(hftirc->ui->inputwin);
                                   ++(hftirc->ui->ib.split);
                              }
                              else if(hftirc->ui->ib.cpos == COLS -1 && !hftirc->ui->ib.spting)
                              {
                                   werase(hftirc->ui->inputwin);
                                   ++(hftirc->ui->ib.split);
                              }
                              else if(hftirc->ui->ib.cpos != COLS - 1)
                                   ++(hftirc->ui->ib.cpos);
                         }
                         break;

                    /* Alt-Backspace, Erase last word */
                    case HFTIRC_KEY_ALTBP:
                         if(hftirc->ui->ib.pos > 1)
                         {
                              for(i = hftirc->ui->ib.pos - 1;
                                        (i + 1) && hftirc->ui->ib.buffer[i - 1] != ' '; --i)
                              {
                                   /* Ctrl-key */
                                   if(IS_CTRLK(hftirc->ui->ib.buffer[hftirc->ui->ib.pos - 1]))
                                   {
                                        wmove(hftirc->ui->inputwin, 0, --hftirc->ui->ib.cpos);
                                        wdelch(hftirc->ui->inputwin);
                                   }

                                   --(hftirc->ui->ib.pos);

                                   if(hftirc->ui->ib.spting)
                                   {
                                        werase(hftirc->ui->inputwin);
                                        --(hftirc->ui->ib.split);

                                        if(hftirc->ui->ib.split <= 0)
                                             hftirc->ui->ib.spting = 0;
                                   }
                                   else
                                        --(hftirc->ui->ib.cpos);

                                   wmove(hftirc->ui->inputwin, 0, hftirc->ui->ib.cpos);

                                   if(hftirc->ui->ib.pos >= 0)
                                        for(j = hftirc->ui->ib.pos;
                                                  hftirc->ui->ib.buffer[j];
                                                  hftirc->ui->ib.buffer[j] = hftirc->ui->ib.buffer[j + 1], ++j);
                                   wdelch(hftirc->ui->inputwin);
                              }
                              ui_get_input();
                         }
                         break;

                    case 127:
                    case KEY_BACKSPACE:
                         if(hftirc->ui->ib.pos > 0)
                         {
                              /* Ctrl-key */
                              if(IS_CTRLK(hftirc->ui->ib.buffer[hftirc->ui->ib.pos - 1]))
                              {
                                   wmove(hftirc->ui->inputwin, 0, --hftirc->ui->ib.cpos);
                                   wdelch(hftirc->ui->inputwin);
                              }

                              --(hftirc->ui->ib.pos);

                              if(hftirc->ui->ib.spting)
                              {
                                   werase(hftirc->ui->inputwin);
                                   --(hftirc->ui->ib.split);

                                   if(hftirc->ui->ib.split <= 0)
                                        hftirc->ui->ib.spting = 0;
                              }
                              else
                                   --(hftirc->ui->ib.cpos);

                              wmove(hftirc->ui->inputwin, 0, hftirc->ui->ib.cpos);

                              if(hftirc->ui->ib.pos >= 0)
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

                         if(hftirc->ui->ib.spting || (int)wcslen(hftirc->ui->ib.buffer) > COLS - 1)
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

                    case '\t':
                         if(hftirc->ui->ib.prev == c)
                         {
                              ++hftirc->ui->ib.hits;
                              hftirc->ui->ib.found = 0;
                         }
                         else
                         {
                              hftirc->ui->ib.hits = 1;
                              wcscpy(tmpbuf, hftirc->ui->ib.buffer);
                         }

                         if(hftirc->ui->ib.pos)
                         {
                              cmp = (hftirc->ui->ib.buffer[0] == '/' && !wcschr(tmpbuf, ' '))
                                   /* Input /cmd completion */
                                   ? complete_input(hftirc->selbuf, hftirc->ui->ib.hits, tmpbuf)
                                   /* Nick completion */
                                   : complete_nick(hftirc->selbuf, hftirc->ui->ib.hits, tmpbuf, &b);

                              if(cmp)
                              {
                                   hftirc->ui->ib.found = 1;
                                   memset(hftirc->ui->ib.buffer, 0, BUFSIZE);
                                   wcscpy(hftirc->ui->ib.buffer, tmpbuf);
                                   wcscat(hftirc->ui->ib.buffer, cmp);
                                   wcscat(hftirc->ui->ib.buffer, ((b) ? L" " : L": "));
                                   free(cmp);
                              }
                         }

                         /* To circular it */
                         if(!hftirc->ui->ib.found)
                              hftirc->ui->ib.hits = 0;

                         werase(hftirc->ui->inputwin);
                         hftirc->ui->ib.pos = hftirc->ui->ib.cpos = wcslen(hftirc->ui->ib.buffer);

                         break;

                    default:
                         if((c > 0 && wcslen(hftirc->ui->ib.buffer) < BUFSIZE))
                         {
                              /* Ctrl-key */
                              if(IS_CTRLK(c))
                                   ++hftirc->ui->ib.cpos;

                              if(hftirc->ui->ib.buffer[hftirc->ui->ib.pos] != '\0')
                                   for(i = (int)wcslen(hftirc->ui->ib.buffer);
                                             i != hftirc->ui->ib.pos - 1;
                                             hftirc->ui->ib.buffer[i] = hftirc->ui->ib.buffer[i - 1], --i);

                              hftirc->ui->ib.buffer[hftirc->ui->ib.pos] = c;

                              if(hftirc->ui->ib.pos >= COLS - 1)
                              {
                                   ++hftirc->ui->ib.split;
                                   --hftirc->ui->ib.cpos;
                                   hftirc->ui->ib.spting = 1;
                              }

                              hftirc->ui->ib.hits = 1;

                              ++(hftirc->ui->ib.pos);
                              ++(hftirc->ui->ib.cpos);
                         }
                         break;
               }
               break;
     }

     hftirc->ui->ib.prev = c;

     werase(hftirc->ui->inputwin);

     hftirc->ui->ib.cpos = (hftirc->ui->ib.cpos < 0 ? 0 : hftirc->ui->ib.cpos);

     mvwaddwstr(hftirc->ui->inputwin, 0, 0,
               hftirc->ui->ib.buffer + hftirc->ui->ib.split);

     wcstombs(buf, hftirc->ui->ib.buffer, BUFSIZE);

     /* /<num> to go on the buffer num */
     if(buf[0] == '/' &&
        ((isdigit(buf[1]) && (n = atoi(&buf[1])) >= 0 && n < 10) /* /n   */
         || (buf[1] == ' ' && (n = atoi(&buf[2])) > 9)))         /* / nn */
     {
          ui_buf_set(n);
          werase(hftirc->ui->inputwin);
          wmemset(hftirc->ui->ib.buffer, 0, BUFSIZE);

          hftirc->ui->ib.pos = hftirc->ui->ib.cpos = hftirc->ui->ib.split
               = hftirc->ui->ib.hits = 0;

          wmove(hftirc->ui->inputwin, 0, 0);
     }

     wrefresh(hftirc->ui->inputwin);

     return;
}
