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
     struct session *s;
     struct session_info info =
     {
          .password = "",
          .port = 6667,
          .nick = "hftircuser",
          .username = "HFTIrcuser",
          .realname = "HFTIrcuser",
          .autojoin = NULL,
          .nautojoin = 0
     };

     if(strlen(input))
     {
          SLIST_FOREACH(s, &H.h.session, next)
               if(!strcmp(input, s->info->server))
               {
                    ui_print_buf(STATUS_BUFFER, "[HFTirc] Already connected to '%s'", input);
                    return;
               }

          info.name = xstrdup(input);
          info.server = info.name;

          s = xcalloc(1, sizeof(struct session));
          s->info = &info;
          SLIST_INSERT_HEAD(&H.h.session, s, next);

          irc_connect(s);
          H.sessionsel = s;
     }
     else
          ui_print_buf(STATUS_BUFFER, "Usage: /connect <address>");
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
     { "connect", 7, input_connect },
     { "join",    4, input_join },
     { "quit",    4, input_quit },
     { "raw",     3, input_raw },
     { "say",     3, input_say },
     { "/",       1, input_say },
     { NULL, 0, NULL },
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
               if(!strncmp(input, input_list[i].cmd, input_list[i].len))
               {
                    input += input_list[i].len;
                    REMOVE_SPACE(input);
                    input_list[i].func(input);
                    break;
               }
     }
     else
          input_say(input);
}
