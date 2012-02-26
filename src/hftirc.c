/*
 * hftirc2 by Martin Duquesnoy <xorg62@gmail.com>
 * For license, see COPYING
 */

#include <signal.h>
#include <getopt.h>

#include "hftirc.h"

void
signal_handler(int signal)
{
     switch(signal)
     {
     case SIGWINCH:
          break;
     }
}

int
main(int argc, char **argv)
{
     struct sigaction sig;
     struct timeval tv;
     struct session *session;
     struct session_info *info;
     struct buffer *buffer;
     fd_set set;
     int i, n, maxfd, nsession;

     while((i = getopt(argc, argv, "hvc:")) != -1)
     {
          switch(i)
          {
          case 'h':
          default:
               printf("usage: %s [-hv] [-c <file>]\n"
                          "   -h         Show this page\n"
                          "   -v         Show version\n"
                      "   -c <file>  Load a configuration file\n", argv[0]);
               exit(EXIT_SUCCESS);
               break;

          case 'v':
               printf("HFTIrc version: "/*HFTIRC_VERSION*/"\n");
               exit(EXIT_SUCCESS);
               break;

          case 'c':
               break;
          }
     }

     sig.sa_handler = signal_handler;
     sig.sa_flags   = 0;
     sigaction(SIGWINCH, &sig, NULL);

     SLIST_INIT(&H.h.session_info);
     SLIST_INIT(&H.h.session);
     TAILQ_INIT(&H.h.buffer);

     H.flags |= HFTIRC_RUNNING;

     while(H.flags & HFTIRC_RUNNING)
     {
          tv.tv_sec = 0;
          tv.tv_usec = 250000;

          FD_ZERO(&set);
          FD_SET(STDIN_FILENO, &set);
          maxfd = STDIN_FILENO;

          nsession = 0;
          SLIST_FOREACH(session, &H.h.session, next)
               if(session->sock > 0 && (session->flags & SESSION_CONNECTED))
               {
                    if(maxfd < session->sock)
                         maxfd = session->sock;
                    FD_SET(session->sock, &set);
                    ++nsession;
               }

          if(select(maxfd + n + 1, &set, NULL, NULL, &tv) > 0)
          {
               if(FD_ISSET(STDIN_FILENO, &set))
                    ui_get_input();
               /*else
                    SLIST_FOREACH(session, &H.h.session, next)
                         if(irc_run_process(session, &set))
                         session->flags &= ~SESSION_CONNECTED;*/
          }

          ui_update();
     }

     while(!SLIST_EMPTY(&H.h.session))
     {
          session = SLIST_FIRST(&H.h.session);
          SLIST_REMOVE_HEAD(&H.h.session, next);
          free(session->mode);
          free(session);
     }

     while(!SLIST_EMPTY(&H.h.session_info))
     {
          info = SLIST_FIRST(&H.h.session_info);
          SLIST_REMOVE_HEAD(&H.h.session_info, next);
          free(info->server);
          free(info->name);
          free(info->nick);
          free(info->username);
          free(info->realname);
          free(info->password);
          free(info);
     }

     while(!TAILQ_EMPTY(&H.h.buffer))
          ui_buffer_remove(TAILQ_FIRST(&H.h.buffer));

     endwin();

}
