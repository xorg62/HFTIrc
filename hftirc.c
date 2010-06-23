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

void
signal_handler(int signal)
{
     int b[2];

     switch(signal)
     {
          case SIGWINCH:
               b[0] = LINES;
               b[1] = COLS;
               endwin();
               refresh();
     }

     return;
}

int
main(int argc, char **argv)
{
    struct sigaction sig;
    int i;
    int maxfd = 0;
    fd_set iset, oset;
    static struct timeval tv;

    hftirc     = malloc(sizeof(HFTIrc));
    hftirc->ui = malloc(sizeof(Ui));
    hftirc->ft = 1;

    /* Signal initialisation */
    sig.sa_handler = signal_handler;
    sig.sa_flags   = 0;
    sigaction(SIGWINCH, &sig, NULL);

    hftirc->running = 1;

    ui_init();
    update_date();
    config_parse(CONFPATH);
    irc_init();

    while(hftirc->running)
    {
         tv.tv_sec = 0;
         tv.tv_usec = 10000;

         FD_ZERO(&iset);
         FD_ZERO(&oset);

         FD_SET(STDIN_FILENO, &iset);

         maxfd = STDIN_FILENO;

         for(i = 0; i < hftirc->conf.nserv; ++i)
              irc_add_select_descriptors(hftirc->session[i], &iset, &oset, &maxfd);

         if(select(maxfd + hftirc->conf.nserv + 1, &iset, &oset, NULL, &tv) > 0)
         {
              if(FD_ISSET(STDIN_FILENO, &iset))
                   ui_get_input();
              else
                   for(i = 0; i < hftirc->conf.nserv; ++i)
                        if(hftirc->session[i]->state == STATE_CONNECTED)
                             irc_process_select_descriptors(hftirc->session[i], &iset, &oset);
         }

         update_date();
         ui_update_statuswin();
         ui_update_topicwin();
         refresh();
         wmove(hftirc->ui->inputwin, 0, hftirc->ui->ib.cpos);
         wrefresh(hftirc->ui->inputwin);
    }

    endwin();

    free(hftirc->cb);
    free(hftirc->ui);
    free(hftirc);

    return 0;
}
