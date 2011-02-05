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
#include "input.h"

#define HFTIRC_NB_SPACE   (160)

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
find_bufid(unsigned int id, const char *str)
{
     int i;

     if(!str)
          return 0;

     for(i = 0; i < hftirc->nbuf; ++i)
          if(hftirc->cb[i].name != NULL
            && strlen(hftirc->cb[i].name) > 1)
               if(!strcasecmp(str, hftirc->cb[i].name)
                 && hftirc->cb[i].sessid == id)
                    return i;

     return 0;
}

/* Find session id by Irc Session */
int
find_sessid(IrcSession *session)
{
     int i;

     if(!session)
          return 0;

     for(i = 0; i < hftirc->conf.nserv; ++i)
          if(session == hftirc->session[i])
               break;

     return i;
}

/* Send message to each buffer with session id = sess */
void
msg_sessbuf(int sess, char *str)
{
     int i;

     if(!str)
          return;

     for(i = 1; i < hftirc->nbuf; ++i)
          if(hftirc->cb[i].sessid == sess)
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

/* We need to keep space in input buffer:
 * wcstombs remove what it think as useless space.
 * Here we count how many space there is before
 * convertion and add its after to final string.
 */
size_t
hft_wcstombs(char *str, wchar_t *wstr, int n)
{
     int nspace, i;
     size_t r;
     char ustr[512];

     if(!str || !wstr)
          return 0;

     for(nspace = 0; wstr[nspace] == L' '; ++nspace);

     r = wcstombs(str, (const wchar_t*)wstr, n);

     /* Add missing space to final str */
     for(i = 0; i < strlen(str) + (nspace + 1); ++i)
     {
          if(i < nspace)
               ustr[i] = ' ';
          else
               ustr[i] = str[i];
     }

     strcpy(str, ustr);

     return r;
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

