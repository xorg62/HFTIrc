/*
 * Copyright (c) 2010 Martin Duquesnoy <xorg62@gmail.com>
 * Copyright (c) 2010 Natal Ngétal <hobbestigrou>
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
#include "ui.h"

static void nick_swap(NickStruct *x, NickStruct *y);

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
nick_sort_abc(ChanBuf *cb)
{
     int swap = 1;
     int oldnml;
     NickStruct *ns;

     if(!cb || !(cb->umask & UNickSortMask))
          return;

     oldnml = cb->nickmaxlen;

     /* Count nicks for nnick / max nick lenght for nicklist size */
     for(cb->nickmaxlen = 1, cb->nnick = 0, ns = cb->nickhead; ns; ns = ns->next)
     {
          ++cb->nnick;
          cb->nickmaxlen = (strlen(ns->nick) > cb->nickmaxlen) ? strlen(ns->nick) : cb->nickmaxlen;
     }

     if((cb->nickmaxlen += 3) != oldnml)
          cb->umask |= UNickListMask;

     /* Alphabetical bubble sort */
     while(swap)
     {
          swap = 0;
          for(ns = cb->nickhead; ns && ns->next; ns = ns->next)
               if(strcasecmp(ns->nick, ns->next->nick) > 0)
               {
                    swap = 1;
                    nick_swap(ns, ns->next);
               }
     }

    cb->umask &= ~UNickSortMask;

    return;
}

void
nick_attach(ChanBuf *cb, NickStruct *nick)
{
     if(!cb)
          return;

     HFTLIST_ATTACH(cb->nickhead, nick);

     cb->umask |= (UNickSortMask | UNickListMask);

     return;
}

void
nick_detach(ChanBuf *cb, NickStruct *nick)
{
     if(!cb)
          return;

     HFTLIST_DETACH(cb->nickhead, NickStruct, nick);

     cb->umask |= (UNickSortMask | UNickListMask);

     return;
}

NickStruct*
nickstruct_set(char *nick)
{
     NickStruct *ret;

     ret = calloc(1, sizeof(NickStruct));

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

