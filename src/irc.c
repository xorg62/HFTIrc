/*
 * hftirc2 by Martin Duquesnoy <xorg62@gmail.com>
 * For license, see COPYING
 */

#include <unistd.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>

#include "hftirc.h"
#include "ui.h"
#include "irc.h"

ssize_t
irc_send_raw(struct session *s, const char *format, ...)
{
     char buf[BUFSIZE];
     va_list va_alist;

     va_start(va_alist, format);
     vsnprintf(buf, sizeof(buf), format, va_alist);
     va_end(va_alist);

     if(!s->sock)
          return 1;

     strcat(buf, "\r\n");

     return send(s->sock, buf, strlen(buf), 0);
}

int
irc_connect(struct session *s)
{
     struct hostent *hp;
     struct sockaddr_in a;

     if(!(hp = gethostbyname(s->info->server)))
     {
          ui_print_buf(STATUS_BUFFER, "[HFTIrc] Cannot resolve host '%s'", s->info->server);
          return 1;
     }

     a.sin_family = AF_INET;
     a.sin_port   = htons(s->info->port ? s->info->port : 6667);

     memcpy(&a.sin_addr, hp->h_addr_list[0], (size_t)hp->h_length);

     if((s->sock = socket(AF_INET, SOCK_STREAM, 0)))
     {
          ui_print_buf(STATUS_BUFFER, "[HFTIrc] Cannot create socket");
          return 1;
     }

     fcntl(s->sock, F_SETFL, O_NONBLOCK);

     if(connect(s->sock, (const struct sockaddr*)&a, sizeof(a)) < 0)
     {
          ui_print_buf(STATUS_BUFFER, "[HFTIrc] Cannot connect to %s\n", s->info->server);
          return 1;
     }

     /* Identification */
     irc_send_raw(s, "NICK %s", s->info->nick);
     irc_send_raw(s, "USER %s localhost %s :%s", s->info->username, s->info->server, s->info->realname);
     irc_send_raw(s, "PASS %s", s->info->password);

     s->flags |= SESSION_CONNECTED;
}

void
irc_disconnect(struct session *s)
{
     if(s->sock >= 0)
          close(s->sock);

     s->flags &= ~SESSION_CONNECTED;

     ui_print_buf(STATUS_BUFFER, "[HFTIrc] Session '%s' disconnected", s->info->name);
}

int
irc_process(struct session *s, fd_set *inset)
{
     int i, len, offset;
     unsigned int amount;

     if(!(s->flags & SESSION_CONNECTED))
     {
          ui_print_buf(STATUS_BUFFER, "[HFTIrc] Server '%s' disconnected", s->info->server);
          return 1;
     }

     if(FD_ISSET(s->sock, inset))
     {
          amount = (sizeof(s->inbuf) - 1) - s->inoffset;

          if((len = recv(s->sock, s->inbuf + s->inoffset, amount, 0)) <= 0)
          {
               ui_print_buf(STATUS_BUFFER, "[HFTIrc] Session '%s' failed recv(ernno = %d)", s->info->name, errno);
               return 1;
          }

          s->inoffset += len;

          do
          {
               for(i = offset = 0; i < ((int)(s->inoffset) -1 ); ++i)
                    if(s->inbuf[i] == '\r' && s->inbuf[i + 1] == '\n')
                    {
                         offset = i + 2;
                         break;
                    }

               if(s->inoffset - offset > 0)
                    memmove(s->inbuf, s->inbuf + offset, s->inoffset - offset);
          }
          while(offset > 0);
     }

     return 0;
}
