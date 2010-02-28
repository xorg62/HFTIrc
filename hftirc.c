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
          case SIGSEGV:
               endwin();
               fprintf(stderr, "HFTirc: Segmentation fault.\n");
               break;
     }

     return;
}


void*
thread_process(void *arg)
{
    fd_set fd;
    static struct timeval timeout;

    if(!(int*)arg)
    {
         if(irc_run(hftirc->session))
              WARN("Error", "irc_run failed");
    }
    else
    {
         while(hftirc->running)
         {
              timeout.tv_sec = 0;
              timeout.tv_usec = 10000;
              FD_ZERO(&fd);
              FD_SET(STDIN_FILENO, &fd);

              if(select(STDIN_FILENO + 1, &fd, NULL, NULL, &timeout))
                   if(FD_ISSET(STDIN_FILENO, &fd))
                        ui_get_input();

              update_date();
              ui_update_statuswin();

              refresh();
              wrefresh(hftirc->ui->inputwin);
         }

         pthread_exit(0);
    }

    return NULL;
}

int
main(int argc, char **argv)
{
    struct sigaction sig;
    void *ret;
    pthread_t irct, uit;

    hftirc     = malloc(sizeof(HFTIrc));
    hftirc->ui = malloc(sizeof(Ui));
    hftirc->ft = 1;

    /* Signal initialisation */
    sig.sa_handler = signal_handler;
    sig.sa_flags   = 0;
    sigaction(SIGWINCH, &sig, NULL);
    sigaction(SIGSEGV, &sig, NULL);

    hftirc->running = 1;

    ui_init();

    irc_init();

    pthread_create(&uit, NULL, thread_process, "1");
    pthread_create(&irct, NULL, thread_process, NULL);

    (void)pthread_join(uit, &ret);
    (void)pthread_join(irct, &ret);

    endwin();

    free(hftirc->ui);
    free(hftirc);

    return 0;
}
