/*
 * hftirc2 by Martin Duquesnoy <xorg62@gmail.com>
 * For license, see COPYING
 */

#include "hftirc.h"
#include "input.h"
#include "util.h"
#include "ui.h"
#include "irc.h"

#define SESSION_CHECK()                                              \
     if(!H.sessionsel)                                               \
     {                                                               \
          ui_print_buf(STATUS_BUFFER, "[HFTIrc] No session");        \
          return;                                                    \
     }

static void
input_connect(const char *input)
{
     if(strlen(input))
     {
          struct session *s;
          struct session_info *info;

          SLIST_FOREACH(s, &H.h.session, next)
               if(!strcmp(input, s->info->server))
               {
                    ui_print_buf(STATUS_BUFFER, "[HFTirc] Already connected to '%s'", input);
                    return;
               }

          /* Session infos */
          info            = xcalloc(1, sizeof(struct session_info));
          info->password  = NULL;
          info->port      = 6667;
          info->server    = xstrdup(input);
          info->name      = xstrdup(input);
          info->nick      = xstrdup("hftircuser");
          info->username  = xstrdup("HFTIrcuser");
          info->realname  = xstrdup("HFTIrcuser");
          info->autojoin  = NULL;
          info->nautojoin = 0;
          SLIST_INSERT_HEAD(&H.h.session_info, info, next);

          /* Session ! */
          s = xcalloc(1, sizeof(struct session));
          s->info = info;
          SLIST_INSERT_HEAD(&H.h.session, s, next);

          irc_connect(s);
          H.sessionsel = s;
     }
     else
          ui_print_buf(STATUS_BUFFER, "Usage: /connect <address>");
}

static void
input_disconnect(const char *input)
{
     SESSION_CHECK();

     irc_disconnect(H.sessionsel);
     ui_print_buf(STATUS_BUFFER, "[%s] Session disconnected", H.sessionsel->info->name);
}

static void
input_join(const char *input)
{
     SESSION_CHECK();

     if(!input && !ISACHAN(H.bufsel->name))
     {
          ui_print_buf(STATUS_BUFFER, "Usage: /join #<channel>");
          return;
     }

     if(irc_send_raw(H.sessionsel, "JOIN %s", (strlen(input) ? input : H.bufsel->name)))
          ui_print_buf(STATUS_BUFFER, "Error: Can't use JOIN command");
}

static void
input_nick(const char *input)
{
     SESSION_CHECK();

     if(!(H.sessionsel->flags & SESSION_MOTD))
     {
          free(H.sessionsel->info->nick);
          H.sessionsel->info->nick = xstrdup(input);
     }

     if(!irc_send_raw(H.sessionsel, "NICK %s", input))
          ui_print_buf(STATUS_BUFFER, "Usage: /nick <nickname>");
}

static void
input_part(const char *input)
{
     SESSION_CHECK();

     if(H.bufsel == STATUS_BUFFER)
          return;

     if(irc_send_raw(H.sessionsel, "PART %s :%s", H.bufsel->name, input))
          ui_print_buf(STATUS_BUFFER, "Error: Can't use PART command");
     else
          ui_buffer_remove(H.bufsel);
}

static void
input_raw(const char *input)
{
     SESSION_CHECK();

     if(!input)
     {
          ui_print_buf(STATUS_BUFFER, "Usage: /raw <rawdata>");
          return;
     }

     if(irc_send_raw(H.sessionsel, "%s", input))
          ui_print_buf(STATUS_BUFFER, "Error: Can't send raw command");
}

static void
input_reconnect(const char *input)
{
     SESSION_CHECK();

     if(H.sessionsel->flags & SESSION_CONNECTED)
          input_disconnect(NULL);

     if(irc_connect(H.sessionsel))
          ui_print_buf(STATUS_BUFFER, "Error: Can't connect to '%s'", H.sessionsel->info->server);
}

static void
input_say(const char *input)
{
     SESSION_CHECK();

     if(!input)
     {
          ui_print_buf(STATUS_BUFFER, "Usage: /say <blah>");
          return;
     }

     if(irc_send_raw(H.sessionsel, "PRIVMSG %s :%s", H.bufsel->name, input))
          ui_print_buf(STATUS_BUFFER, "Error: Can't send privmsg");
     else
          ui_print_buf(H.bufsel, "<%s> %s", H.sessionsel->info->nick, input);
}

static void
input_quit(const char *input)
{
     struct session *s;

     SLIST_FOREACH(s, &H.h.session, next)
          irc_send_raw(s, "QUIT :%s", input);

     H.flags ^= HFTIRC_RUNNING;
}

/*
 * Input management: Index array + associativ loop
 */
static const struct
{
     char *cmd;
     int len;
     void (*func)(const char *arg);
} input_list[] =
{
     { "connect",    7,  input_connect },
     { "disconnect", 10, input_disconnect },
     { "join",       4,  input_join },
     { "nick",       4,  input_nick },
     { "part",       4,  input_part },
     { "quit",       4,  input_quit },
     { "raw",        3,  input_raw },
     { "reconnect",  9,  input_reconnect },
     { "say",        3,  input_say },
     { "/",          1,  input_say },
     { NULL,         0,  NULL },
};

void
input_manage(char *input)
{
     int i = 0;

     if(*input == '/')
     {
          ++input;

          REMOVE_SPACE(input);

          for(; input_list[i].cmd; ++i)
          {
               if(!strncmp(input, input_list[i].cmd, input_list[i].len))
               {
                    input += input_list[i].len;
                    REMOVE_SPACE(input);
                    input_list[i].func(input);
                    break;
               }
          }
     }
     else
          input_say(input);
}
