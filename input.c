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

const InputStruct input_struct[] =
{
     { "join",       input_join },
     { "nick",       input_nick },
     { "quit",       input_quit },
     { "part",       input_part },
     { "kick",       input_kick },
     { "names",      input_names },
     { "topic",      input_topic },
     { "query",      input_query },
     { "me",         input_me },
     { "msg",        input_msg },
     { "whois",      input_whois },
     { "close",      input_close },
     { "raw",        input_raw },
     { "umode",      input_umode },
     { "serv",       input_serv },
     { "redraw",     input_redraw },
     { "connect",    input_connect },
     { "server",     input_connect },
     { "disconnect", input_disconnect },
     { "away",       input_away },
     { "ctcp",       input_ctcp },
     { "help",       input_help },
};

void
input_manage(const char *input)
{
     int i, n;

     if(input[0] == '/')
     {
          ++input;

          /* /<num> to go on the buffer num */
          if(sscanf(input, "%d", &n) == 1)
          {
               ui_buf_set(n);
               return;
          }

          for(i = 0; i < LEN(input_struct); ++i)
               if(!strncmp(input, input_struct[i].cmd, strlen(input_struct[i].cmd)))
                    input_struct[i].func(input + strlen(input_struct[i].cmd));
     }
     else
     {
          if(hftirc->conf.nserv
                    && irc_cmd_msg(hftirc->session[hftirc->selses],
                         hftirc->cb[hftirc->selbuf].name, input))
               WARN("Error", "Can't send message");
          else
               ui_print_buf(hftirc->selbuf, "<%s> %s", hftirc->conf.serv[hftirc->selses].nick, input);
     }

     return;
}

void
input_join(const char *input)
{
     DSINPUT(input);
     NOSERVRET();

     if(input[0] != '#' && input[0] != '&')
     {
          WARN("Error", "Usage: /join #<channel>");

          return;
     }

     /* Last arg -> password (TODO) */
     if(irc_cmd_join(hftirc->session[hftirc->selses], input, NULL))
          WARN("Error", "Can't use JOIN command");

     return;
}

void
input_nick(const char *input)
{
     DSINPUT(input);
     NOSERVRET();

     if(irc_cmd_nick(hftirc->session[hftirc->selses], input))
          WARN("Error", "Can't change nick or invalid nick");

     return;
}

void
input_quit(const char *input)
{
     DSINPUT(input);

     if(hftirc->conf.nserv)
          irc_cmd_quit(hftirc->session[hftirc->selses], input);

     hftirc->running = 0;

     return;
}

void
input_names(const char *input)
{
     NOSERVRET();

     if(irc_cmd_names(hftirc->session[hftirc->selses], hftirc->cb[hftirc->selbuf].name))
          WARN("Error", "Can't get names list");

     return;
}

void
input_help(const char *input)
{
     ui_print_buf(0, "Help: ...");

     return;
}

void
input_topic(const char *input)
{
     DSINPUT(input);
     NOSERVRET();

     if(strlen(input) > 0)
     {
          if(irc_cmd_topic(hftirc->session[hftirc->selses], hftirc->cb[hftirc->selbuf].name, input))
               WARN("Error", "Can't change topic");
     }
     else
          ui_print_buf(hftirc->selbuf, "  *** Topic of %s: %s",
                    hftirc->cb[hftirc->selbuf].name,
                    hftirc->cb[hftirc->selbuf].topic);
     return;
}

void
input_part(const char *input)
{
     DSINPUT(input);
     NOSERVRET();

     /* irc_cmd_part can't send a part message... */
     if(irc_send_raw(hftirc->session[hftirc->selses], "PART %s :%s",
                    hftirc->cb[hftirc->selbuf].name, input))
          WARN("Error", "Can't use PART command");
     else
          ui_buf_close(hftirc->selbuf);

     return;
}

void
input_me(const char *input)
{
     DSINPUT(input);
     NOSERVRET();

     if(irc_cmd_me(hftirc->session[hftirc->selses], hftirc->cb[hftirc->selbuf].name, input))
          WARN("Error", "Can't send action message");
     else
          ui_print_buf(hftirc->selbuf, " %c* %s%c %s", B, hftirc->conf.serv[hftirc->selses].nick, B, input);

     return;
}

void
input_msg(const char *input)
{
     int i, b = 0;
     char nick[64] = { 0 };
     char msg[BUFSIZE] = { 0 };

     DSINPUT(input);
     NOSERVRET();

     if(strlen(input) > 0)
     {
          for(i = 0; input[i] != ' ' && input[i]; nick[i] = input[i], ++i);

          if(input[i] == ' ')
          {
               input += strlen(nick);
               DSINPUT(input);
               for(i = 0; input[i]; msg[i] = input[i], ++i);
               ++b;
          }
     }

     if(!b)
     {
          WARN("Error", "Usage: /msg <nick/channel> <message>");

          return;
     }
     else
          if(irc_cmd_msg(hftirc->session[hftirc->selses], nick, msg))
               WARN("Error", "Can't send MSG");

     return;
}

void
input_kick(const char *input)
{
     int i;
     char nick[64] = { 0 };
     char reason[BUFSIZE] = { 0 };

     DSINPUT(input);
     NOSERVRET();

     if(strlen(input) > 0)
     {
          for(i = 0; input[i] != ' ' && input[i]; nick[i] = input[i], ++i);

          if(input[i] == ' ')
          {
               input += strlen(nick);
               DSINPUT(input);
               for(i = 0; input[i]; reason[i] = input[i], ++i);
          }
     }
     else
     {
          WARN("Error", "Usage: /kick <nick> <reason(optional)>");

          return;
     }

     if(irc_cmd_kick(hftirc->session[hftirc->selses], nick, hftirc->cb[hftirc->selbuf].name, reason))
          WARN("Error", "Can't kick");

     return;
}

void
input_whois(const char *input)
{
     DSINPUT(input);
     NOSERVRET();

     if(strlen(input) > 0)
     {
          if(irc_cmd_whois(hftirc->session[hftirc->selses], input))
               WARN("Error", "Can't use WHOIS");
     }
     /* No input -> whois current private nick */
     else
     {
          if(hftirc->cb[hftirc->selbuf].name[0] == '#'
                    || hftirc->cb[hftirc->selbuf].name[0] == '&')
               WARN("Error", "Usage: /whois <nick>");
          else if(irc_cmd_whois(hftirc->session[hftirc->selses], hftirc->cb[hftirc->selbuf].name))
               WARN("Error", "Can't use WHOIS");
     }

     return;
}

void
input_query(const char *input)
{
     int i;

     DSINPUT(input);
     NOSERVRET();

     if(strlen(input) > 0)
     {
          for(i = 0; i < hftirc->nbuf; ++i)
               if(!strcmp(hftirc->cb[i].name, input)
                         && hftirc->cb[i].sessid == hftirc->selses)
               {
                    ui_buf_set(i);
                    return;
               }

          ui_buf_new(input, hftirc->selses);
          ui_buf_set(hftirc->nbuf - 1);
          ui_print_buf(hftirc->nbuf - 1, "  *** Query with %s", input);
     }
     else
          WARN("Error", "Usage: /query <nick>");

     return;
}

void
input_close(const char *input)
{
     if(hftirc->selbuf == 0)
          return;

     if(hftirc->cb[hftirc->selbuf].name[0] == '#'
               || hftirc->cb[hftirc->selbuf].name[0] == '&')
          input_part(NULL);

     ui_buf_close(hftirc->selbuf);

     return;
}

void
input_raw(const char *input)
{
     DSINPUT(input);
     NOSERVRET();

     if(strlen(input) > 0)
     {
          if(irc_send_raw(hftirc->session[hftirc->selses], "%s", input))
               WARN("Error", "Can't use raw");
     }
     else
          WARN("Error", "Usage: /raw <cmd>");

     return;
}

void
input_umode(const char *input)
{
     DSINPUT(input);
     NOSERVRET();

     if(!strlen(input))
          WARN("Error", "Usage: /umode <mode>");
     else if(irc_cmd_user_mode(hftirc->session[hftirc->selses], input))
          WARN("Error", "Can't set user mode");

     return;
}

void
input_serv(const char *input)
{
     int i;

     DSINPUT(input);

     if(hftirc->selbuf != 0)
          return;

     if(strlen(input) > 0)
     {
          for(i = 0; i < hftirc->conf.nserv; ++i)
               if(!strcasecmp(hftirc->conf.serv[i].name, input))
                    hftirc->selses = i;
     }
     else
     {
          if(hftirc->selses + 1 >= hftirc->conf.nserv)
               hftirc->selses = 0;
          else
               ++hftirc->selses;
     }

     refresh();

     return;
}

void
input_redraw(const char *input)
{
     endwin();
     ui_init();
     ui_buf_set(hftirc->selbuf);

     return;
}

void
input_connect(const char *input)
{
     int i = -1, a = 0;
     ServInfo defsi = { " ", " ", " ", 6667, "hftircuser", " ", "HFTIrcuser", "HFTIrcuser"};

     DSINPUT(input);

     if(strlen(input) > 0)
     {
          if(hftirc->conf.nserv > NSERV)
          {
               WARN("Error", "Too much connected servers");

               return;
          }

          /* Check if serv is already used (but disconnected) in hftirc */
          for(i = 0; i < hftirc->conf.nserv; ++i)
               if(!strcmp(input, hftirc->conf.serv[i].adress)
                         || !strcasecmp(input, hftirc->conf.serv[i].name))
               {
                    if(irc_is_connected(hftirc->session[i]))
                    {
                         WARN("Error", "Already connected to this server");
                         return;
                    }
                    else
                    {
                         ++a;
                         break;
                    }
               }

          if(!a)
          {
               ++hftirc->conf.nserv;
               i = (hftirc->conf.nserv - 1);

               hftirc->conf.serv[i] = defsi;
               strcpy(hftirc->conf.serv[i].name, input);
               strcpy(hftirc->conf.serv[i].adress, input);
          }

          hftirc->session[i] = irc_create_session(&hftirc->callbacks);

          if(irc_connect(hftirc->session[i],
                         hftirc->conf.serv[i].adress,
                         hftirc->conf.serv[i].port,
                         hftirc->conf.serv[i].password,
                         hftirc->conf.serv[i].nick,
                         hftirc->conf.serv[i].username,
                         hftirc->conf.serv[i].realname))
               ui_print_buf(0, "Error: Can't connect to %s", hftirc->conf.serv[i].adress);

          hftirc->selses = i;
     }
     else
          WARN("Error", "Usage: /connect <adress>  or  /server <adress>");

     return;
}

void
input_disconnect(const char *input)
{
     int i = hftirc->selses;

     DSINPUT(input);
     NOSERVRET();

     if(strlen(input) > 0)
     {
          for(i = 0; i < hftirc->conf.nserv; ++i)
               if(!strcasecmp(hftirc->conf.serv[i].name, input))
                    break;

          irc_disconnect(hftirc->session[i]);
     }

     irc_disconnect(hftirc->session[i]);

     ui_print_buf(0, "[%s] *** %c%s%c is now Disconnected",
               hftirc->conf.serv[i].name, B, hftirc->conf.serv[i].name, B);

     return;
}

void
input_away(const char *input)
{
     int i;

     DSINPUT(input);
     NOSERVRET();

     if(strlen(input) > 0)
     {
          for(i = 0; i < hftirc->conf.nserv; ++i)
               if(irc_send_raw(hftirc->session[i], "AWAY :%s", input))
                    WARN("Error", "Can't send AWAY");

     }
     else
          for(i = 0; i < hftirc->conf.nserv; ++i)
               if(irc_send_raw(hftirc->session[i],  "AWAY :"))
                    WARN("Error", "Can't send AWAY");

     return;
}

void
input_ctcp(const char *input)
{
     int i;
     char nick[64] = { 0 };
     char request[BUFSIZE] = { 0 };

     DSINPUT(input);
     NOSERVRET();

     if(strlen(input) > 0)
     {
          for(i = 0; input[i] != ' ' && input[i]; nick[i] = input[i], ++i);

          if(input[i] == ' ')
          {
               input += strlen(nick);
               DSINPUT(input);
               for(i = 0; input[i]; request[i] = input[i], ++i);
          }
     }
     else
     {
          WARN("Error", "Usage: /ctcp <nick> <request>");

          return;
     }

     if(irc_cmd_ctcp_request(hftirc->session[hftirc->selses], nick, request))
          WARN("Error", "Can't kick");

     return;
}

