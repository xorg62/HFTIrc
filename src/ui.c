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
#include "ui.h"

void
ui_init(void)
{
     IrcSession *is;

     /* Init buffers (only first time) */
     if(hftirc->ft)
     {
          hftirc->ui->nicklist = hftirc->conf.nicklist;
          hftirc->nbuf = 0;
          hftirc->statuscb = ui_buf_new("status", hftirc->selsession);
          hftirc->selcb = hftirc->statuscb;

          hftirc->ui->tcolor = hftirc->conf.tcolor;

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

          for(is = hftirc->sessionhead; is; is = is->next)
               free(is);

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
     hftirc->ui->ib.nhisto = 1;
     hftirc->ui->ib.histpos = 0;
     wrefresh(hftirc->ui->inputwin);

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

     for(i = 0; i < COLORS; ++i)
          for(j = 0; j < COLORS; ++j)
               init_pair(++n, i, (!j ? hftirc->ui->bg : j));

     hftirc->ui->c = n;

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
          if(pair_content(i, &f, &b) == OK && f == fg && b == bg)
               return COLOR_PAIR(i);

     return 0;
}

void
ui_update_statuswin(void)
{
     int j, c, x, y;
     ChanBuf *cb;

     if(!hftirc->selcb)
          return;

     /* Erase all window content */
     werase(hftirc->ui->statuswin);

     /* Update bg color */
     wbkgd(hftirc->ui->statuswin, COLOR_SW);

     /* Print date */
     mvwprintw(hftirc->ui->statuswin, 0, 0, "[%s]", hftirc->date.str);

     /* Pseudo with mode */
     mvwprintw(hftirc->ui->statuswin, 0, strlen(hftirc->date.str) + 3, "(");
     PRINTATTR(hftirc->ui->statuswin, COLOR_SW2, hftirc->selsession->nick);
     waddch(hftirc->ui->statuswin, '(');
     PRINTATTR(hftirc->ui->statuswin, COLOR_SW2, hftirc->selsession->mode);
     waddstr(hftirc->ui->statuswin, "))");

     /* Info about current serv/channel */
     wprintw(hftirc->ui->statuswin, " (%d:", hftirc->selcb->id);
     PRINTATTR(hftirc->ui->statuswin, COLOR_SW2,  hftirc->selsession->name);

     if(!hftirc->selsession->connected)
          PRINTATTR(hftirc->ui->statuswin, A_BOLD, " (Disconnected)");

     waddch(hftirc->ui->statuswin, '/');
     PRINTATTR(hftirc->ui->statuswin, COLOR_SW2, hftirc->selcb->name);
     waddch(hftirc->ui->statuswin, ')');

     /* Activity */
     wprintw(hftirc->ui->statuswin, " (Bufact: ");

     /* First pritority is when ISCHAN(..) == 0, second for == j.
      * Priority: Private conversation, highlight channel, and normal active channel.
      */
     for(j = 0; j < 2; ++j)
          for ( c = 2; c > 0; --c )
               for(cb = hftirc->cbhead; cb; cb = cb->next)
                    if(ISCHAN(cb->name[0]) == j && cb->act == c && cb != hftirc->statuscb)
                    {
                         wattron(hftirc->ui->statuswin, ((c == 2) ? COLOR_HLACT : COLOR_ACT));
                         wprintw(hftirc->ui->statuswin, "%d", cb->id);
                         wprintw(hftirc->ui->statuswin, ":%s", cb->name);
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
     if(!hftirc->selcb || !(hftirc->selcb->umask & UTopicMask))
          return;

     /* Erase all window content */
     werase(hftirc->ui->topicwin);

     /* Update bg color */
     wbkgd(hftirc->ui->topicwin, COLOR_SW);

     /* Write topic */
     /*   Channel   */
     if(ISCHAN(hftirc->selcb->name[0]))
          waddstr(hftirc->ui->topicwin, hftirc->selcb->topic);
     /*   Other    */
     else
          wprintw(hftirc->ui->topicwin, "%s (%s)",
                    hftirc->selcb->name, hftirc->selsession->name);

     wrefresh(hftirc->ui->topicwin);

     hftirc->selcb->umask &= ~UTopicMask;

     return;
}

void
ui_update_nicklistwin(void)
{
     int i, c, p;
     char ord[4] = { '@', '%', '+', '\0' };
     NickStruct *ns;

     if(!hftirc->selcb)
          return;

     nick_sort_abc(hftirc->selcb);

     if(!hftirc->ui->nicklist
               || !(hftirc->selcb->umask & UNickListMask))
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
          for(ns = hftirc->selcb->nickhead;
                    ns && c < ((LINES - 3) + hftirc->selcb->nicklistscroll); /* </// Scroll limit */
                    ns = ns->next)
          {
               if(ns->rang == ord[i])
               {
                    if(p >= hftirc->selcb->nicklistscroll)
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

     hftirc->selcb->umask &= ~UNickListMask;

     return;
}

void
ui_print(WINDOW *w, char *str, int n)
{
     int i;
     unsigned int hmask = A_NORMAL;
     unsigned int mask = A_NORMAL;
     unsigned int lastposmask = A_NORMAL;
     unsigned int colmask = A_NORMAL;
     int fg = 15, bg  = 1, mcol = 0, lcol = 0;

     if(!str || !w)
          return;

     /* Last position tracker with bold line */
     if(hftirc->conf.lastlinepos && hftirc->selcb->lastposbold == n)
          lastposmask |= COLOR_LASTPOS;

     for(i = 0; i < (strlen(str) < BUFFERSIZE ? strlen(str) : BUFFERSIZE); ++i)
     {
          switch(str[i])
          {
               /* Stop wrote color at the end of the nick */
               /* Bold */
               case B:
                    mask ^= A_BOLD;
                    break;

               /* Underline */
               case U:
                    mask ^= A_UNDERLINE;
                    break;

               /* Reversed color */
               case V:
                    mask ^= A_REVERSE;
                    break;

               /* Cancel masks */
               case HFTIRC_END_COLOR:
                    hmask &= ~lcol;
                    mask = colmask = 0;
                    break;

               /* HFTIrc colors, for interface use */
               case HFTIRC_COLOR:
                    if(isdigit(str[i + 1]))
                    {
                         fg = (str[++i] - '0');

                         if(isdigit(str[i + 1]))
                              fg = fg * 10 + (str[++i] - '0');

                         if(fg > -1 && fg < 16)
                              colmask ^= (ui_color(hftirccol[fg].c, hftirc->ui->bg) | hftirccol[fg].m);

                         fg = 0;
                    }
                    break;

               /* mIRC©®™ colors */
               case MIRC_COLOR:
                    if(lcol)
                        hmask &= ~lcol;

                    /* Set fg color first if there is no coma */
                    if(isdigit(str[i + 1]))
                    {
                         fg = (str[++i] - '0');

                         if(isdigit(str[i + 1]))
                              fg = fg * 10 + (str[++i] - '0');

                         mcol = 1;
                    }

                    /* bg color if coma */
                    if(str[i + 1] == ',' && isdigit(str[i + 2]))
                    {
                         bg = (str[i + 2] - '0');
                         i += 2;

                         if(isdigit(str[i + 1]))
                              bg = bg * 10 + (str[++i] - '0');

                         if(fg == bg)
                              fg = 1;
                         mcol = 1;
                    }

                    if(mcol)
                            hmask ^= (lcol = (ui_color(mirccol[fg % COLORMAX].c, mirccol[bg % COLORMAX].c)
                                           | mirccol[fg % COLORMAX].m));

                    break;

               default:
                    /* simple waddch doesn't work with some char */
                    wattron(w, colmask | hmask | mask | lastposmask);
                    wprintw(w, "%c", str[i]);
                    wattroff(w, colmask | hmask | mask | lastposmask);

                    break;
          }
     }

     return;
}

void
ui_print_buf(ChanBuf *cb, char *format, ...)
{
     int i;
     va_list ap;
     char *p, *buf;

     if(!cb || !(p = calloc(BUFFERSIZE, sizeof(char))))
          return;

     /* Get format */
     va_start(ap, format);
     vsnprintf(p, BUFFERSIZE, format, ap);
     va_end(ap);

     /* Clean ...[part]... we need in buffer */
     for(i = 0; i < BUFFERSIZE; ++i)
          cb->buffer[(cb->bufpos * BUFFERSIZE) + i] = '\0';

     /* Set buffer line */
     snprintf(&cb->buffer[cb->bufpos * BUFFERSIZE], BUFFERSIZE, "%s %s\n", hftirc->date.str, p);
     buf = &cb->buffer[cb->bufpos * BUFFERSIZE];

     /* New buffer position */
     cb->bufpos = (cb->bufpos < BUFLINES - 1) ? cb->bufpos + 1 : 0;

     /* Print on buffer if cb = selected buf */
     if(cb == hftirc->selcb && !cb->scrollpos)
     {
          ui_print(hftirc->ui->mainwin, buf, 0);
          wrefresh(hftirc->ui->mainwin);
     }

     /* Activity management:
      *   1: Normal acitivity on the buffer (talking, info..)
      *   2: Highlight activity on the buffer
      */
     if(cb != hftirc->selcb)
     {
          if(cb->act != 2)
               cb->act = 1;

          /* Highlight test (if hl or private message) */
          if(hftirc->conf.serv && cb && ((((strchr(buf, '<') && strchr(buf, '>')) || strchr(buf, '*'))
           && strcasestr(buf + strlen(hftirc->date.str) + 4, hftirc->selsession->nick)) || !ISCHAN(cb->name[0])))
               /* No HL on status buffer (0) */
               cb->act = (cb != hftirc->statuscb) ? 2 : 1;
     }

     buf = NULL;
     FREEPTR(&p);

     return;
}

void
ui_draw_buf(ChanBuf *cb)
{
     int i = 0;

     if(!cb)
          return;

     for(i = (cb->bufpos + cb->scrollpos) - MAINWIN_LINES; i < (cb->bufpos + cb->scrollpos); ++i)
          if(i < BUFLINES)
          {
               if(i < 0 && cb->buffer[(BUFLINES + i) * BUFFERSIZE])
                    ui_print(hftirc->ui->mainwin, &cb->buffer[(BUFLINES + i) * BUFFERSIZE], BUFLINES + i);
               else
                    ui_print(hftirc->ui->mainwin, ((i >= 0) ? &cb->buffer[i * BUFFERSIZE] : "\n"), i);
          }

     wrefresh(hftirc->ui->mainwin);

     return;
}

/* Argument is not a ChanBuf pointer but and id
 * for an easier use with user interface.
 */
void
ui_buf_set(int buf)
{
     ChanBuf *c, *cb;

     if(!(cb = find_buf_wid(buf)))
          return;

     if(hftirc->selcb)
     {
          hftirc->selcb->lastposbold = hftirc->selcb->bufpos - 1;

          /* Find selcb real pointer */
          for(c = hftirc->cbhead; c && c != hftirc->selcb; c = c->next);
          hftirc->prevcb = (hftirc->prevcb == hftirc->selcb ? hftirc->statuscb : c);
     }
     else
          hftirc->prevcb = hftirc->statuscb;

     /* Set selected cb */
     hftirc->selcb = cb;

     cb->act = 0;
     cb->umask |= (UTopicMask | UNickListMask);

     if(cb != hftirc->statuscb)
          hftirc->selsession = cb->session;

     ui_draw_buf(cb);

     return;
}

ChanBuf*
ui_buf_new(const char *name, IrcSession *session)
{
     ChanBuf *cb;

     if(!strlen(name))
          name = strdup("???");

     cb = (ChanBuf*)calloc(1, sizeof(ChanBuf));

     HFTLIST_ATTACH_END(hftirc->cbhead, ChanBuf, cb);

     cb->id = hftirc->nbuf++;

     cb->buffer = (char*)calloc(BUFLINES * BUFFERSIZE, sizeof(char));

     strcpy(cb->name, name);
     cb->bufpos = cb->scrollpos = cb->act = 0;
     cb->naming = cb->nicklistscroll = 0;
     cb->lastposbold = -1;
     cb->session = session;
     cb->umask |= (UTopicMask | UNickListMask);
     cb->nickhead = NULL;

     if(ISCHAN(name[0]))
          ui_buf_set(cb->id);

     return cb;
}

void
ui_buf_close(ChanBuf *cb)
{
     NickStruct *ns;
     ChanBuf *c;
     int n;

     if(!cb || cb == hftirc->statuscb || cb->id > hftirc->nbuf - 1)
          return;

     --hftirc->nbuf;

     /* Free nick of chan */
     for(ns = cb->nickhead; ns; ns = ns->next)
          nick_detach(cb, ns);

     FREEPTR(&cb->nickhead);
     FREEPTR(&cb->buffer);

     HFTLIST_DETACH(hftirc->cbhead, ChanBuf, cb);

     /* Re-set id */
     for(n = 0, c = hftirc->cbhead; c; c->id = n++, c = c->next);

     if(!hftirc->prevcb)
          hftirc->prevcb = hftirc->statuscb;

     ui_buf_set(hftirc->prevcb->id);

     return;
}

void
ui_scroll_up(ChanBuf *cb)
{
     if(!cb || cb->bufpos + cb->scrollpos - 1 < 0)
          return;

     cb->scrollpos -= (MAINWIN_LINES / 2);

     if(cb == hftirc->selcb)
          ui_draw_buf(cb);

     return;
}

void
ui_scroll_down(ChanBuf *cb)
{
     if(!cb || cb->scrollpos >= 0 || cb->scrollpos > BUFLINES)
          return;

     cb->scrollpos += (MAINWIN_LINES / 2);

     if(cb == hftirc->selcb)
          ui_draw_buf(cb);

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
          hftirc->selcb->umask |= UNickListMask;
          ui_update_nicklistwin();
     }
     else
     {
          delwin(hftirc->ui->nicklistwin);
          hftirc->ui->mainwin = newwin(MAINWIN_LINES, COLS, 1, 0);
     }

     scrollok(hftirc->ui->mainwin, TRUE);

     ui_draw_buf(hftirc->selcb);

     return;
}

void
ui_nicklist_scroll(int v)
{
     if(hftirc->selcb->nicklistscroll + v < 0
               || hftirc->selcb->nicklistscroll + v > hftirc->selcb->nnick + 2)
          return;

     hftirc->selcb->nicklistscroll += v;

     hftirc->selcb->umask |= UNickListMask;
     ui_update_nicklistwin();

     return;
}

void
ui_buf_swap(int n)
{
  /*  ChanBuf old_buffer;

    if(!n || !hftirc->selcb || !strcmp(hftirc->selcb->name, "status")
                   ||  n <= 0 || n > hftirc->nbuf -1 || n == hftirc->selcb->id)
              return;

    old_buffer = hftirc->cb[ n ];
    hftirc->cb[ n ] = hftirc->cb[ hftirc->selbuf ];
    hftirc->cb[ hftirc->selbuf ] = old_buffer;

    ui_buf_set(n);*/

    return;
}

void
ui_set_color_theme(int col)
{
     if(col < -1 || col > 7)
          return;

     hftirc->ui->tcolor = col;
     hftirc->selcb->umask |= (UTopicMask | UNickListMask);

     return;
}

void
ui_refresh_curpos(void)
{
     wchar_t wc;

     /* Draw cursor */
     wmove(hftirc->ui->inputwin, 0, hftirc->ui->ib.cpos);
     hftirc_waddwch(hftirc->ui->inputwin, A_REVERSE,
               (!(wc = hftirc->ui->ib.buffer[hftirc->ui->ib.pos]) ? ' ' : wc));

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
                         ui_buf_set(hftirc->selcb->id - 1);
                         break;

                    case KEY_F(2):
                    case C('n'):
                         ui_buf_set(hftirc->selcb->id + 1);
                         break;

                    case KEY_F(3):
                         ui_nicklist_toggle();
                         break;

                    case KEY_F(11):
                         ui_nicklist_scroll(-(MAINWIN_LINES / 2));
                         break;

                    case KEY_F(12):
                         ui_nicklist_scroll(+(MAINWIN_LINES / 2));
                         break;

                    case KEY_PPAGE:
                         ui_scroll_up(hftirc->selcb);
                         break;

                    case KEY_NPAGE:
                         ui_scroll_down(hftirc->selcb);
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

                    /* Alt-Backspace / ^W, Erase last word */
                    case HFTIRC_KEY_ALTBP:
                    case C('w'):
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

                    case HFTIRC_KEY_DELALL:
                         werase(hftirc->ui->inputwin);
                         hftirc->ui->ib.cpos = hftirc->ui->ib.pos = 0;
                         hftirc->ui->ib.spting = hftirc->ui->ib.split = 0;
                         wmemset(hftirc->ui->ib.buffer, 0, BUFSIZE);
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
                                   ? complete_input(hftirc->selcb, hftirc->ui->ib.hits, tmpbuf)
                                   /* Nick completion */
                                   : complete_nick(hftirc->selcb, hftirc->ui->ib.hits, tmpbuf, &b);

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

     ui_refresh_curpos();

     return;
}
