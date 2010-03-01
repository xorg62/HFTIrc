#include "hftirc.h"

void
signal_handler(int signal)
{
     switch(signal)
     {
          case SIGWINCH:
               endwin();
               sleep(1);
               ui_init();
               waddwstr(hftirc->ui->inputwin, hftirc->ui->ib.buffer);
               ui_buf_set(hftirc->selbuf);
               break;
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

    update_date();
    ui_init();
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

         if(select(maxfd + hftirc->conf.nserv, &iset, &oset, NULL, &tv) > 0)
         {
              if(FD_ISSET(STDIN_FILENO, &iset))
                   ui_get_input();
              else
                   for(i = 0; i < hftirc->conf.nserv; ++i)
                        if(irc_is_connected(hftirc->session[i]))
                             if(irc_process_select_descriptors(hftirc->session[i], &iset, &oset))
                                  ui_print_buf(0, "Error: irc_process_select failed with %s",
                                            hftirc->conf.serv[i].adress);
         }

         update_date();
         ui_update_statuswin();
         refresh();
         wmove(hftirc->ui->inputwin, 0, hftirc->ui->ib.cpos);
         wrefresh(hftirc->ui->inputwin);
    }

    endwin();

    free(hftirc->session);
    free(hftirc->ui);
    free(hftirc);

    return 0;
}
