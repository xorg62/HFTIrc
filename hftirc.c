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
               sleep(1);
               ui_init();
               ui_print_buf(0, "Terminal resized: (%dx%d -> %dx%d)", b[0], b[1], LINES, COLS);
               ui_buf_set(hftirc->selbuf);
               break;
     }

     return;
}

void
draw_logo(void)
{
     int i;
     const char *logo[] =
     {
          "---------------------------------------",
          "     __  __  ______ ______",
          "    / / / / /  ___//_   _/____",
          "   / /_/ / / /_     / /  /_ _/ _ __  ___",
          "  /  _  / /  _/    / /   / / / '__// __/",
          " / / / / / /      / /   / / / /  / (__",
          "/_/ /_/ /_/      /_/  /___//_/   \\___/",
          "Hackers Feeding Themselves irc client.",
          "",
          "--------------------------------------"
     };

     for(i = 0; i < LEN(logo); ++i)
          ui_print_buf(0, "[HFTIrc]  %c%s", B, logo[i]);

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
    config_parse(CONFPATH);
    update_date();
    draw_logo();
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
                        if(irc_is_connected(hftirc->session[i]))
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

    free(hftirc->ui);
    free(hftirc);

    return 0;
}
