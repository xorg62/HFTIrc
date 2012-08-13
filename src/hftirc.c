/*
 * hftirc2 by Martin Duquesnoy <xorg62@gmail.com>
 * For license, see COPYING
 */

#include <signal.h>
#include <getopt.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

#include "hftirc.h"
#include "ui.h"
#include "config.h"
#include "irc.h"

void
signal_handler(int signal)
{
     int oc, ol, d;
     (void)d;

     switch(signal)
     {
     /* Terminal resize */
     case SIGWINCH:
          oc = COLS;
          ol = LINES;

          endwin();
          refresh();
          getmaxyx(stdscr, d, d);

          ui_init();
          ui_get_input();
          ui_print_buf(STATUS_BUFFER,
                       "[HFTIrc] *** Terminal resized (%dx%d -> %dx%d)",
                       ol, oc, LINES, COLS);
          ui_buffer_set(H.bufsel);

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
     char path[MAX_PATH_LEN] = { 0 };
     fd_set set;
     int i, maxfd, nsession;

     sprintf(path, "%s/.config/hftirc/hftirc.conf", getenv("HOME"));

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
               strncpy(path, optarg, sizeof(path));
               break;
          }
     }

     H.confpath = path;

     sig.sa_handler = signal_handler;
     sig.sa_flags   = 0;
     sigaction(SIGWINCH, &sig, NULL);

     SLIST_INIT(&H.h.session_info);
     SLIST_INIT(&H.h.session);
     TAILQ_INIT(&H.h.buffer);

     H.flags |= (HFTIRC_RUNNING | HFTIRC_FIRST_TIME);

     config_init();
     ui_init();

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

          if(select(maxfd + nsession + 1, &set, NULL, NULL, &tv) > 0)
          {
               if(FD_ISSET(STDIN_FILENO, &set))
                    ui_get_input();
               else

                    SLIST_FOREACH(session, &H.h.session, next)
                         if(irc_process(session, &set))
                              session->flags &= ~SESSION_CONNECTED;
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

     return 0;
}
