#include "hftirc.h"

#define COLOR_SW    1
#define COLOR_HL    2
#define COLOR_DEF   3
#define COLOR_ENCAD 4
#define COLOR_JOIN  5
#define COLOR_PART  6

#define MASK_CONV     (1 << 2)
#define MASK_DATEPOS  (1 << 3)
#define MASK_ENCAD    (1 << 4)
#define MASK_SYMINFO  (1 << 5)
#define MASK_PARTJOIN (1 << 6)

void
ui_init(void)
{
     int i, j, bg = -1;

     /* Init buffers (only first time) */
     if(hftirc->ft)
     {
          hftirc->selbuf = 0;
          hftirc->nbuf = 1;

          for(i = 0; i < 64; ++i)
          {
               memset(hftirc->cb[i].name, 0, sizeof(hftirc->cb[i].name));
               memset(hftirc->cb[i].topic, 0, sizeof(hftirc->cb[i].topic));

               strcpy(hftirc->cb[i].name, "status");
               hftirc->cb[i].sessid = 0;

               for(j = 0; j < BUFLINES; ++j)
                    memset(hftirc->cb[i].buffer[j], 0, sizeof(hftirc->cb[i].buffer[j]));

               hftirc->cb[i].bufpos = 0;
          }

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
     start_color();
     bg = (use_default_colors() == OK) ? -1 : COLOR_BLACK;
     init_pair(0, bg, bg);
     init_pair(COLOR_SW,    COLOR_BLACK, COLOR_GREEN);
     init_pair(COLOR_HL,    COLOR_YELLOW, bg);
     init_pair(COLOR_DEF,   COLOR_GREEN, bg);
     init_pair(COLOR_ENCAD, COLOR_BLACK, bg);
     init_pair(COLOR_JOIN,  COLOR_CYAN, bg);
     init_pair(COLOR_PART,  COLOR_RED, bg);

     /* Init main window and the borders */
     hftirc->ui->mainwin = newwin(MAINWIN_LINES, COLS, 1, 0);
     scrollok(hftirc->ui->mainwin, TRUE);
     wrefresh(hftirc->ui->mainwin);

     /* Init topic window */
     hftirc->ui->topicwin = newwin(1, COLS, 0, 0);
     wbkgd(hftirc->ui->topicwin, COLOR_PAIR(1));
     wrefresh(hftirc->ui->statuswin);

     /* Init input window */
     hftirc->ui->inputwin = newwin(1, COLS, LINES - 1, 0);
     wmove(hftirc->ui->inputwin, 0, 0);
     wrefresh(hftirc->ui->inputwin);

     /* Init status window (with the hour / current chan) */
     hftirc->ui->statuswin = newwin(1, COLS, LINES - 2, 0);
     wbkgd(hftirc->ui->statuswin, COLOR_PAIR(1));
     wrefresh(hftirc->ui->statuswin);


     refresh();

     return;
}

void
ui_update_statuswin(void)
{
     /* Erase all window content */
     werase(hftirc->ui->statuswin);

     /* Update bg color */
     wbkgd(hftirc->ui->statuswin, COLOR_PAIR(COLOR_SW));

     /* Print date */
     mvwprintw(hftirc->ui->statuswin, 0, 0, "%s", hftirc->date.str);

     /* Pseudo with mode */
     mvwprintw(hftirc->ui->statuswin, 0, strlen(hftirc->date.str) + 1, "(");
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
     wbkgd(hftirc->ui->topicwin, COLOR_PAIR(COLOR_SW));

     /* Write topic */
     waddstr(hftirc->ui->topicwin, hftirc->cb[hftirc->selbuf].topic);

     wrefresh(hftirc->ui->topicwin);

     return;
}

/* Set basic mainwin colors in mask to attribute it in ui_print */
void
ui_manage_print_color(int i, char *str, int *mask)
{
     /* Date special char [::] */
     if(i <= DATELEN && (str[i] == ':' || str[i] == '[' || str[i] == ']'))
          *mask |= COLOR_PAIR(COLOR_DEF);

     /* Nicks <> */
     if(i == DATELEN + 1 && str[i] == '<' && strchr(str + i + 1, '>'))
          *mask |= (COLOR_PAIR(COLOR_ENCAD) | A_BOLD | MASK_DATEPOS | MASK_CONV);
     else if(str[i] == '>' && (*mask & MASK_DATEPOS))
          *mask |= (COLOR_PAIR(COLOR_ENCAD) | A_BOLD);
     else if(str[i - 1] == '>' && (*mask & MASK_DATEPOS))
          *mask &= ~(MASK_DATEPOS);

     /* Info symbol .:. */
     if(!(*mask & MASK_CONV) && i >= DATELEN + 2
               && ((str[i] == '.' && str[i + 1] == ':' && str[i + 2] == '.')
                    || (str[i - 1] == '.' && str[i] == ':' && str[i + 1] == '.')
                    || (str[i - 2] == '.' && str[i - 1] == ':' && str[i] == '.')))
     {
          *mask |= (COLOR_PAIR(COLOR_DEF) | A_UNDERLINE | MASK_SYMINFO);
          if(str[i] == ':')
          {
               *mask |= (A_BOLD);
               *mask &= ~(A_UNDERLINE);
          }
     }
     else if((*mask & MASK_SYMINFO) && str[i - 1] == '.')
          *mask &= ~(MASK_SYMINFO);

     /* Join & par/quit ->>>> / <<<<- */
     if((*mask & MASK_PARTJOIN) || (i >= DATELEN + 3 && i <= DATELEN + 8 && !(*mask & MASK_CONV)
                    && ((str[i] == '-' && str[i + 4] == '>')
                         || (str[i] == '<' && str[i + 4] == '-'))))
     {
          *mask |= (COLOR_PAIR(str[DATELEN + 7] == '-' ? 6 : 5) | MASK_PARTJOIN);

          if(str[i] == ' ' && (str[i - 1] == '>' || str[i - 1] == '-'))
               *mask &= ~(COLOR_PAIR(COLOR_JOIN) | COLOR_PAIR(COLOR_PART) | MASK_PARTJOIN);
     }


     /* All []() */
     if(!(*mask & MASK_CONV) && i > DATELEN && (str[i] == '['
               || str[i] == ']' || str[i] == '(' || str[i] == ')'))
          *mask |= (COLOR_PAIR(COLOR_ENCAD) | A_BOLD | MASK_ENCAD);

     return;
}

void
ui_print(WINDOW *w, char *str)
{
     int i, hmask = A_NORMAL, mask = A_NORMAL;

     if(!str || !w)
          return;

     /* Highlight line */
     if(hftirc->conf.serv && hftirc->selbuf != 0
               && strchr(str, '<') && strchr(str, '>')
               && strstr(str + strlen(hftirc->date.str) + 4,
                    hftirc->conf.serv[hftirc->selses].nick))
               mask |= (COLOR_PAIR(COLOR_HL) | A_BOLD);

     for(i = 0; i < strlen(str); ++i)
     {
          /* Bold and underline */
          if(str[i] == B || str[i] == C('_'))
          {
               hmask ^= (str[i] == B ? A_BOLD : A_UNDERLINE);

               if(str[i + 1] != B && str[i + 1] != C('_'))
                    ++i;
          }

          /* Decoration stuff */
          if(!(mask & COLOR_PAIR(COLOR_HL)))
               ui_manage_print_color(i, str, &mask);

          wattron(w, mask | hmask);
          wprintw(w, "%c", str[i]);
          wattroff(w, mask | hmask);

          /* Clean mask */
          mask &= ~(COLOR_PAIR(COLOR_DEF) | COLOR_PAIR(COLOR_ENCAD)
                    | ((mask & MASK_DATEPOS || mask & MASK_ENCAD) ? A_BOLD : 0)
                    | (mask & MASK_SYMINFO ? (A_UNDERLINE | A_BOLD) : 0));
     }

     return;
}

void
ui_print_buf(int id, char *format, ...)
{
     va_list ap;
     char *buf, *p;

     va_start(ap, format);
     vasprintf(&p, format, ap);

     buf = malloc(sizeof(char) * strlen(p) + strlen(hftirc->date.str) + 3);
     va_end(ap);

     sprintf(buf, "%s %s\n", hftirc->date.str, p);

     strcpy(hftirc->cb[id].buffer[hftirc->cb[id].bufpos], buf);

     hftirc->cb[id].bufpos = (hftirc->cb[id].bufpos < BUFLINES - 1) ? hftirc->cb[id].bufpos + 1 : 0;

     if(id == hftirc->selbuf)
     {
          ui_print(hftirc->ui->mainwin, buf);
          wrefresh(hftirc->ui->mainwin);
     }

     free(buf);
     free(p);

     return;
}

void
ui_draw_buf(int id)
{
     int i;

     if(id < 0 || id > hftirc->nbuf)
          return;

     werase(hftirc->ui->mainwin);

     if(hftirc->cb[id].bufpos > MAINWIN_LINES + 1)
     {
          i = hftirc->cb[id].bufpos - MAINWIN_LINES;

          for(; i < hftirc->cb[id].bufpos; ++i)
               if(i < BUFLINES - 1 &&
                         hftirc->cb[id].buffer[i])
                    ui_print(hftirc->ui->mainwin, hftirc->cb[id].buffer[i]);
     }
     else
          for(i = 0; i < hftirc->cb[id].bufpos; ++i)
               if(hftirc->cb[id].buffer[i])
                    ui_print(hftirc->ui->mainwin, hftirc->cb[id].buffer[i]);

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

     ui_draw_buf(buf);

     return;
}

void
ui_buf_new(const char *name, unsigned int id)
{
     if(!strlen(name))
          strcpy(hftirc->cb[hftirc->nbuf].name, "???");

     ++hftirc->nbuf;

     strcpy(hftirc->cb[hftirc->nbuf - 1].name, name);
     hftirc->cb[hftirc->nbuf - 1].sessid = id;

     return;
}

void
ui_buf_close(int buf)
{
     int i;
     ChanBuf cbnull;

     /* Set a empty Chanbuf to set at the last buf if
      * it is closed */
      for(i = 0; i < BUFLINES; ++i)
           memset(cbnull.buffer[i], 0, sizeof(cbnull.buffer[i]));
      cbnull.bufpos = 0;


     if(buf <= 0 || buf > hftirc->nbuf - 1)
          return;

     if(hftirc->selbuf == buf)
          ui_buf_set(--hftirc->selbuf);

     if(buf != hftirc->nbuf - 1)
          for(i = buf; i < hftirc->nbuf - 1; ++i)
               hftirc->cb[i] = hftirc->cb[i + 1];
     else
          hftirc->cb[hftirc->nbuf - 1] = cbnull;

     --hftirc->nbuf;

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
                    case KEY_PPAGE:
                         ui_buf_set(hftirc->selbuf + 1);
                         break;

                    case KEY_NPAGE:
                         ui_buf_set(hftirc->selbuf - 1);
                         break;

                    case HFTIRC_KEY_ENTER:
                         if(hftirc->ui->ib.pos || wcslen(hftirc->ui->ib.buffer))
                         {
                              memset(buf, 0, BUFSIZE);
                              wcstombs(buf, hftirc->ui->ib.buffer, BUFSIZE);
                              input_manage(buf);
                              werase(hftirc->ui->inputwin);
                              wmemset(hftirc->ui->ib.buffer, 0, BUFSIZE);
                              hftirc->ui->ib.pos
                                   = hftirc->ui->ib.cpos
                                   = hftirc->ui->ib.split = 0;
                              wmove(hftirc->ui->inputwin, 0, 0);
                         }
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

                    case KEY_UP:
                         break;

                    case KEY_DOWN:
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
