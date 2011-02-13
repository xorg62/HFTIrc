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

#include "input.h"
#include "ui.h"

struct { char name[10]; int id; } colordef[9] =
{
     { "black",   COLOR_BLACK },
     { "red",     COLOR_RED },
     { "green",   COLOR_GREEN },
     { "yellow",  COLOR_YELLOW },
     { "blue",    COLOR_BLUE },
     { "magenta", COLOR_MAGENTA },
     { "cyan",    COLOR_CYAN },
     { "white",   COLOR_WHITE }
};

void
update_date(void)
{
     hftirc->date.tm = localtime(&hftirc->date.lt);
     hftirc->date.lt = time(NULL);

     strftime(hftirc->date.str, sizeof(hftirc->date.str), "%m-%d %H:%M:%S", hftirc->date.tm);

     return;
}

/* Find buffer id with name */
int
find_bufid(IrcSession *s, const char *str)
{
     int i;

     if(!str)
          return 0;

     for(i = 0; i < hftirc->nbuf; ++i)
          if(hftirc->cb[i].name != NULL
            && strlen(hftirc->cb[i].name) > 1)
               if(!strcasecmp(str, hftirc->cb[i].name)
                 && hftirc->cb[i].session == s)
                    return i;

     return 0;
}

/* Send message to each buffer with session id = sess */
void
msg_sessbuf(IrcSession *session, char *str)
{
     int i;

     if(!str)
          return;

     for(i = 1; i < hftirc->nbuf; ++i)
          if(hftirc->cb[i].session == session)
               ui_print_buf(i, str);

     return;
}

/* For compatibility with FreeBSD 7.x */
static int
hft_wcsncasecmp(const wchar_t *s1, const wchar_t *s2, int n)
{
     int lc1, lc2, diff;

     if(!s1 || !s2)
          return -1;

     for(lc1 = lc2 = diff = 0 ; n-- > 0 ;++s1, ++s2)
     {

          lc1 = towlower(*s1);
          lc2 = towlower(*s2);
          diff = lc1 - lc2;

          if(diff)
               return diff;

          if(!lc1)
               return 0;
     }

     return 0;
}

int
color_to_id(char *name)
{
     int i;

     for(i = 0; i < LEN(colordef); ++i)
          if(!strcasecmp(colordef[i].name, name))
               return colordef[i].id;

     return COLOR_THEME_DEF;
}

/* Return string of str with color sequence wanted */
char*
colorstr(int color, char *str, ...)
{
     va_list ap;
     static char ret[512] = { 0 };
     char *p;

     if(!str)
          return NULL;

     va_start(ap, str);
     vasprintf(&p, str, ap);

     va_end(ap);

     if(!color)
          strcpy(ret, p);
     else
          snprintf(ret, 512, "%c%d%s%c", HFTIRC_COLOR, color, p, HFTIRC_END_COLOR);

     free(p);

     return ret;
}

/* Generate a color for each nick and return string with
 * nick string and color sequence
 */
char*
nick_color(char *nick)
{
     int i, col;
     static char ret[NICKLEN + 6] = { 0 };

     if(!nick)
          return NULL;

     if(!hftirc->conf.nickcolor)
          return nick;

     /* To find color number, we add all char of the nick string */
     for(i = col = 0; nick[i]; col += tolower(nick[i++]));

     /* Check if color is different of black & hl color */
     for(col %= LastCol; col == Black || col == LightYellow; ++col);

     sprintf(ret, "%c%d%s%c", HFTIRC_COLOR, abs(col), nick, HFTIRC_END_COLOR);

     return ret;
}

int
hftirc_waddwch(WINDOW *w, unsigned int mask, wchar_t wch)
{
     int ret = 0;
     cchar_t cch;
     wchar_t wstr[2] = { wch, 0 };

     wattron(w, mask);

     if(setcchar(&cch, wstr, A_NORMAL, 0, NULL) == OK)
          ret = wadd_wch(w, &cch);

     wattroff(w, mask);

     return ret;
}

wchar_t*
complete_nick(int buf, unsigned int hits, wchar_t *start, int *beg)
{
     NickStruct *ns;
     wchar_t wbuf[BUFSIZE] = { 0 };
     int i, c = 0;

     if(!start || hits <= 0)
          return NULL;

     /* If the nick is located in middle of text */
     for(i = wcslen(start) + 1; i != 0; --i)
          if(start[i] == ' ')
          {
               start += i + 1;
               break;
          }

     *beg = i;

     for(ns = hftirc->cb[buf].nickhead; ns; ns = ns->next)
     {
          swprintf(wbuf, BUFSIZE, L"%s", ns->nick);
          if(!hft_wcsncasecmp(wbuf, start, wcslen(start)))
               if(++c == hits)
                    return wcsdup(wbuf + wcslen(start));
     }

     return NULL;
}

wchar_t*
complete_input(int buf, unsigned int hits, wchar_t *start)
{
     wchar_t wbuf[BUFSIZE] = { 0 };
     int i, c = 0;

     if(!start || start[0] != '/' || hits <= 0)
          return NULL;

     /* Erase / */
     ++start;

     for(i = 0; i < LEN(input_struct); ++i)
     {
          swprintf(wbuf, BUFSIZE, L"%s", input_struct[i].cmd);
          if(!hft_wcsncasecmp(wbuf, start, wcslen(start)))
               if(++c == hits)
                    return wcsdup(wbuf + wcslen(start));
     }

     return NULL;
}

