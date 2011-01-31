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

