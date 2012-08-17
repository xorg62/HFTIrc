/*
 * hftirc2 by Martin Duquesnoy <xorg62@gmail.com>
 * For license, see COPYING
 */

#include "hftirc.h"
#include "ui.h"
#include "irc.h"
#include "event.h"
#include "util.h"

#define SPLIT_NICK_HOST(buf, nick, host)                                \
     do {                                                               \
          nick = (char*)buf;                                            \
          if((host = strchr(nick, '!')))                                \
          {                                                             \
               *host = '\0';                                            \
               ++host;                                                  \
          }                                                             \
     } while( /* CONSTCOND */ 0);

void
event_dump(struct session *s, int code, const char *prefix, const char **params, int paramindex)
{
     int i = 0;
     char buf[BUFSIZE] = { 0 };

     for(; i < paramindex; ++i)
     {
          if(i)
               strcat(buf, " ");
          strcat(buf, params[i]);
     }

     ui_print_buf(STATUS_BUFFER, "[%s] *** (%s): '%s'", s->info->name, prefix, buf);
}

void
event_ping(struct session *s, int code, const char *prefix, const char **params, int paramindex)
{
     irc_send_raw(s, "PONG %s", params[0]);
}

void
event_join(struct session *s, int code, const char *prefix, const char **params, int paramindex)
{
     char *p, *nick, *host;

     s->buf_event = ui_buffer_find(s, params[0]);

     SPLIT_NICK_HOST(prefix, nick, host);

     if(!strcmp(nick, s->info->nick))
     {
          if(s->buf_event != STATUS_BUFFER)
               ui_buffer_set(s->buf_event);
          else
          {
               s->buf_event = ui_buffer_new(s, params[0]);
               ui_buffer_set(s->buf_event);
          }
     }

     if(!(H.ignore_flags & IGNORE_JOIN))
          ui_print_buf(s->buf_event, " ->>>> %s (%s) has joined %s", nick, host, params[0]);
}

void
event_privmsg(struct session *s, int code, const char *prefix, const char **params, int paramindex)
{
     char *nick, *host;

     if(paramindex < 2)
          return;

     SPLIT_NICK_HOST(prefix, nick, host)

     /*******************
      * Private messages
      */
     if(!strcmp(params[0], s->info->nick))
     {
          /* New buffer for new private conversation */
          if((s->buf_event = ui_buffer_find(s, nick)) == STATUS_BUFFER)
          {
               s->buf_event = ui_buffer_new(s, nick);
          }
     }
     /*******************
      * Channel messages
      */
     else
     {
          /* Unknown message root, print it in status buffer */
          if(!(s->buf_event = ui_buffer_find(s, params[0])))
               s->buf_event = STATUS_BUFFER;
     }

     ui_print_buf(s->buf_event, "<%s> %s", nick, params[1]);
}
