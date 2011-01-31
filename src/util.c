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

static void nick_swap(NickStruct *x, NickStruct *y);

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

static void
nick_swap(NickStruct *x, NickStruct *y)
{
     NickStruct t = *x;

     strcpy(x->nick, y->nick);
     x->rang = y->rang;

     strcpy(y->nick, t.nick);
     y->rang = t.rang;

     return;
}

/* Sort alphabetically nick list */
void
nick_sort_abc(int buf)
{
     int swap = 1;
     NickStruct *ns;

     if(!buf || !(hftirc->cb[buf].umask & UNickSortMask))
          return;

     /* Alphabetical bubble sort */
     while(swap)
     {
          swap = 0;
          for(ns = hftirc->cb[buf].nickhead; ns->next; ns = ns->next)
               if(strcasecmp(ns->nick, ns->next->nick) > 0)
               {
                    swap = 1;
                    nick_swap(ns, ns->next);
               }
     }

    hftirc->cb[buf].umask &= ~UNickSortMask;

    return;
}

void
nick_attach(int buf, NickStruct *nick)
{
     if(hftirc->cb[buf].nickhead)
          hftirc->cb[buf].nickhead->prev = nick;

     nick->next = hftirc->cb[buf].nickhead;
     hftirc->cb[buf].nickhead = nick;

     hftirc->cb[buf].umask |= (UNickSortMask | UNickListMask);

     return;
}

void
nick_detach(int buf, NickStruct *nick)
{
     NickStruct **ns;

     for(ns = &hftirc->cb[buf].nickhead
               ; *ns && *ns != nick
               ; ns = &(*ns)->next);

     *ns = nick->next;

     hftirc->cb[buf].umask |= (UNickSortMask | UNickListMask);

     return;
}

NickStruct*
nickstruct_set(char *nick)
{
     NickStruct *ret;

     ret = malloc(sizeof(NickStruct));

     memset(ret->nick, 0, NICKLEN);
     ret->rang = '\0';

     if(strchr("@+%", nick[0]))
     {
          ret->rang = nick[0];
          strcpy(ret->nick, nick + 1);
     }
     else
          strcpy(ret->nick, nick);

     return ret;
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

