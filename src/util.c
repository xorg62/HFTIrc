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

struct { char name[10]; int id; } colordef[] =
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

/** calloc with error support
 * \param nmemb Number of objects
 * \param size size of single object
 * \return non null void pointer
*/
void*
xcalloc(size_t nmemb, size_t size)
{
     void *ret;

     if((ret = calloc(nmemb, size)) == NULL)
          err(EXIT_FAILURE, "calloc(%zu * %zu)", nmemb, size);

     return ret;
}

void*
xmalloc(size_t nmemb, size_t size)
{
     void *ret;

     if(SIZE_MAX / nmemb < size)
          err(EXIT_FAILURE, "xmalloc(%zu, %zu), "
                    "size_t overflow detected", nmemb, size);

     if((ret = malloc(nmemb * size)) == NULL)
          err(EXIT_FAILURE, "malloc(%zu)", nmemb * size);

     return ret;
}

/** asprintf wrapper
 * \param strp target string
 * \param fmt format
 * \return non zero integer
 */
int
xasprintf(char **strp, const char *fmt, ...)
{
     int ret;
     va_list args;

     va_start(args, fmt);
     ret = vasprintf(strp, fmt, args);
     va_end(args);

     if (ret == -1)
          err(EXIT_FAILURE, "asprintf(%s)", fmt);

     return ret;
}

/** strdup with error support
 * \param str char pointer
 * \retun non null void pointer
 */
char *
xstrdup(const char *str)
{
     char *ret;

     if (str == NULL || (ret = strdup(str)) == NULL)
          err(EXIT_FAILURE, "strdup(%s)", str);

     return ret;
}


void
update_date(void)
{
     hftirc->date.tm = localtime(&hftirc->date.lt);
     hftirc->date.lt = time(NULL);

     strftime(hftirc->date.str, sizeof(hftirc->date.str), hftirc->conf.datef, hftirc->date.tm);

     return;
}

/* Find buffer pointer with name */
ChanBuf*
find_buf(IrcSession *s, const char *str)
{
     ChanBuf *cb;

     if(!str)
          return hftirc->statuscb;

     for(cb = hftirc->cbhead; cb; cb = cb->next)
          if(cb->name && strlen(cb->name) > 1)
               if(!strcasecmp(str, cb->name) && cb->session == s)
                    return cb;

     return hftirc->statuscb;
}

/* Find buffer pointer with id */
ChanBuf*
find_buf_wid(int id)
{
     ChanBuf *cb;

     if(id < 0 || id > hftirc->nbuf - 1)
          return NULL;

     for(cb = hftirc->cbhead; cb; cb = cb->next)
          if(cb->id == id)
               return cb;

     return NULL;
}

/* Send message to each buffer with session id = sess */
void
msg_sessbuf(IrcSession *session, char *str)
{
     ChanBuf *cb;

     if(!str)
          return;

     for(cb = hftirc->cbhead->next; cb; cb = cb->next)
          if(cb->session == session)
               ui_print_buf(cb, str);

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

     if(!str || !(p = calloc(BUFFERSIZE, sizeof(char))))
          return NULL;

     va_start(ap, str);
     vsnprintf(p, BUFFERSIZE, str, ap);
     va_end(ap);

     if(!color)
          strcpy(ret, p);
     else
          snprintf(ret, sizeof(ret), "%c%d%s%c", HFTIRC_COLOR, color, p, HFTIRC_END_COLOR);

     FREEPTR(&p);

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
complete_nick(ChanBuf *cb, unsigned int hits, wchar_t *start, int *beg)
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

     for(ns = cb->nickhead; ns; ns = ns->next)
     {
          swprintf(wbuf, BUFSIZE, L"%s", ns->nick);
          if(!hft_wcsncasecmp(wbuf, start, wcslen(start)))
               if(++c == hits)
                    return wcsdup(wbuf + wcslen(start));
     }

     return NULL;
}

wchar_t*
complete_input(ChanBuf *cb, unsigned int hits, wchar_t *start)
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

