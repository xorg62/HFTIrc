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

#include "input.h"

void
input_manage(char *input)
{
     int i;

     if(input[0] == '/')
     {
          /* Erase / */
          ++input;

          /* Erase spaces at the end */
          for(; *(input + strlen(input) - 1) == ' '; *(input + strlen(input) - 1) = '\0');

          for(i = 0; i < LEN(input_struct); ++i)
               if(!strncmp(input, input_struct[i].cmd, strlen(input_struct[i].cmd)))
                    input_struct[i].func(input + strlen(input_struct[i].cmd));
     }
     else
          input_say(input);

     return;
}

void
input_join(const char *input)
{
     DSINPUT(input);
     NOSERVRET();

     if(!input && !ISCHAN(hftirc->cb[hftirc->selbuf].name[0]))
     {
               WARN("Error", "Usage: /join #<channel>");
               return;
     }

     /* Last arg -> password (TODO) */
     if(irc_send_raw(hftirc->session[hftirc->selses], "JOIN %s",
                    ((strlen(input)) ? input : hftirc->cb[hftirc->selbuf].name)))
          WARN("Error", "Can't use JOIN command");

     return;
}

void
input_nick(const char *input)
{
     DSINPUT(input);
     NOSERVRET();

     if(!hftirc->session[hftirc->selses]->motd_received)
     {
          strcpy(hftirc->session[hftirc->selses]->nick, input);
          strcpy(hftirc->conf.serv[hftirc->selses].nick, input);
     }

     if(irc_send_raw(hftirc->session[hftirc->selses], "NICK %s", input))
          WARN("Error", "Can't change nick or invalid nick");

     return;
}

void
input_quit(const char *input)
{
     DSINPUT(input);

     irc_send_raw(hftirc->session[hftirc->selses], "QUIT :%s", input);

     hftirc->running = -1;

     return;
}

void
input_names(const char *input)
{
     NOSERVRET();

     if(ISCHAN(hftirc->cb[hftirc->selbuf].name[0]))
          if(irc_send_raw(hftirc->session[hftirc->selses], "NAMES %s",
                         hftirc->cb[hftirc->selbuf].name))
               WARN("Error", "Can't get names list");

     return;
}

void
input_help(const char *input)
{
     int i;

     ui_print_buf(0, "[Hftirc] *** %cCommands list%c:", B, B);

     for(i = 0; i < LEN(input_struct); ++i)
          ui_print_buf(0, "[Hftirc] - %s", input_struct[i].cmd);

     return;
}

void
input_topic(const char *input)
{
     DSINPUT(input);
     NOSERVRET();

     if(strlen(input) > 0)
     {
          if(irc_send_raw(hftirc->session[hftirc->selses], "TOPIC %s :%s", hftirc->cb[hftirc->selbuf].name, input))
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

     if(irc_send_raw(hftirc->session[hftirc->selses], "PRIVMSG %s :\x01" "ACTION %s\x01",
                    hftirc->cb[hftirc->selbuf].name, input))
          WARN("Error", "Can't send action message");
     else
          ui_print_buf(hftirc->selbuf, " %c* %s%c %s", B, hftirc->session[hftirc->selses]->nick, B, input);

     return;
}

void
input_msg(const char *input)
{
     int i, b = 0;
     char nick[NICKLEN] = { 0 };
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
     {
          if(irc_send_raw(hftirc->session[hftirc->selses], "PRIVMSG %s :%s", nick, msg))
               WARN("Error", "Can't send MSG");
          else if((i = find_bufid(hftirc->selses, nick)))
                ui_print_buf(i, "<%s> %s", hftirc->session[hftirc->selses]->nick, msg);
     }

     return;
}

void
input_kick(const char *input)
{
     int i;
     char nick[NICKLEN] = { 0 };
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

     if(strlen(reason) > 0)
		irc_send_raw(hftirc->session[hftirc->selses], "KICK %s %s :%s",
                    hftirc->cb[hftirc->selbuf].name, nick, reason);
	else
          irc_send_raw(hftirc->session[hftirc->selses], "KICK %s %s",
                     hftirc->cb[hftirc->selbuf].name, nick);

     return;
}

void
input_whois(const char *input)
{
     DSINPUT(input);
     NOSERVRET();

     if(strlen(input) > 0)
     {
          if(irc_send_raw(hftirc->session[hftirc->selses], "WHOIS %s %s", input, input))
               WARN("Error", "Can't use WHOIS");
     }
     /* No input -> whois current private nick */
     else
     {
          if(ISCHAN(hftirc->cb[hftirc->selbuf].name[0]))
               WARN("Error", "Usage: /whois <nick>");
          else if(irc_send_raw(hftirc->session[hftirc->selses], "WHOIS %s %s",
                         hftirc->cb[hftirc->selbuf].name, hftirc->cb[hftirc->selbuf].name))
               WARN("Error", "Can't use WHOIS");
     }

     return;
}

void
input_query(const char *input)
{
     int i;
     NickStruct *ns;

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
          ns = nickstruct_set((char *)input);
          nick_attach(hftirc->nbuf - 1,  ns);
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

     if(ISCHAN(hftirc->cb[hftirc->selbuf].name[0]))
               input_part(NULL);

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
     else if(irc_send_raw(hftirc->session[hftirc->selses], "MODE %s %s",
                    hftirc->session[hftirc->selses]->nick, input))
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
                    if(hftirc->session[i]->connected)
                    {
                         WARN("Warning", "Already connected on this server");
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

          hftirc->session[i] = irc_session();

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
     char nick[NICKLEN] = { 0 };
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

     if(irc_send_raw(hftirc->session[hftirc->selses], "PRIVMSG %s :\x01%s\x01", nick, request))
          WARN("Error", "Can't send ctcp");

     return;
}

void
input_buffer(const char *input)
{
     int i;

     DSINPUT(input);
     NOSERVRET();

     if(strlen(input) > 0)
     {
          if(sscanf(input, "%d", &i) == 1)
          {
               ui_buf_set(i);
               return;
          }
     }
     else
          WARN("Error", "Usage: /buffer <num>");

     return;
}

void
input_buffer_list(const char *input)
{
     int i;

     ui_print_buf(0, "[Hftirc] %cBuffers list%c:", B, B);

     for(i = 0; i < hftirc->nbuf; ++i)
          ui_print_buf(0, "[Hftirc] - %d: %s (%s)",
                    i, hftirc->cb[i].name, hftirc->conf.serv[hftirc->cb[i].sessid].name);

     return;
}

void
input_buffer_swap(const char *input)
{
    int i;

    if(strlen(input) > 0)
    {
        if(sscanf(input, "%d", &i) == 1)
        {
            ui_buf_swap(i);
            return;
        }
     }
     else
        WARN("Error", "Usage: /buffer_swap <num>");
     
     return;
}

void
input_say(const char *input)
{
     DSINPUT(input);
     NOSERVRET();

     if(strlen(input) > 0)
     {
          if(irc_send_raw(hftirc->session[hftirc->selses], "PRIVMSG %s :%s",
                         hftirc->cb[hftirc->selbuf].name, input))
               WARN("Error", "Can't send message");
          else
               ui_print_buf(hftirc->selbuf, "<%s> %s", hftirc->session[hftirc->selses]->nick, input);
     }
     else
          WARN("Error", "Usage: /say <message>");

     return;
}

void
input_reconnect(const char *input)
{
     int i;

     DSINPUT(input);

     i = hftirc->selses;

     if(hftirc->session[i]->connected)
          irc_disconnect(hftirc->session[i]);

     if(irc_connect(hftirc->session[i],
                    hftirc->conf.serv[i].adress,
                    hftirc->conf.serv[i].port,
                    hftirc->conf.serv[i].password,
                    hftirc->conf.serv[i].nick,
                    hftirc->conf.serv[i].username,
                    hftirc->conf.serv[i].realname))
          ui_print_buf(0, "Error: Can't connect to %s", hftirc->conf.serv[i].adress);

     return;
}

void
input_roster_scroll(const char *input)
{
     DSINPUT(input);
     NOSERVRET();

     if(strlen(input) > 0)
          ui_roster_scroll(atoi(input));
     else
          WARN("Error", "Usage: /roster_scroll +/-<value>");

     return;
}

void
input_roster_toggle(const char *input)
{
     ui_roster_toggle();

     return;
}
