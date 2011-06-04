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
#include "ui.h"

static int say_without_cmd = 0;

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
     {
          say_without_cmd = 1;
          input_say(input);
     }

     return;
}

void
input_join(const char *input)
{
     DSINPUT(input);
     NOSERVRET();

     if(!input && !ISCHAN(hftirc->selcb->name[0]))
     {
               WARN("Error", "Usage: /join #<channel>");
               return;
     }

     /* Last arg -> password (TODO) */
     if(irc_send_raw(hftirc->selsession, "JOIN %s",
                    ((strlen(input)) ? input : hftirc->selcb->name)))
          WARN("Error", "Can't use JOIN command");

     return;
}

void
input_nick(const char *input)
{
     DSINPUT(input);
     NOSERVRET();

     if(!hftirc->selsession->motd_received)
          strcpy(hftirc->selsession->nick, input);

     if(irc_send_raw(hftirc->selsession, "NICK %s", input))
          WARN("Error", "Can't change nick or invalid nick");

     return;
}

void
input_quit(const char *input)
{
     IrcSession *is;

     DSINPUT(input);

     for(is = hftirc->sessionhead; is; is = is->next)
          irc_send_raw(is, "QUIT :%s", input);

     hftirc->running = -1;

     return;
}

void
input_names(const char *input)
{
     NOSERVRET();

     if(ISCHAN(hftirc->selcb->name[0]))
          if(irc_send_raw(hftirc->selsession, "NAMES %s",
                         hftirc->selcb->name))
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
          if(irc_send_raw(hftirc->selsession, "TOPIC %s :%s", hftirc->selcb->name, input))
               WARN("Error", "Can't change topic");
     }
     else
          ui_print_buf(hftirc->selcb, "  *** Topic of %s: %s",
                    hftirc->selcb->name,
                    hftirc->selcb->topic);

     return;
}

void
input_part(const char *input)
{
     DSINPUT(input);
     NOSERVRET();

     /* irc_cmd_part can't send a part message... */
     if(irc_send_raw(hftirc->selsession, "PART %s :%s",
                    hftirc->selcb->name, input))
          WARN("Error", "Can't use PART command");
     else
          ui_buf_close(hftirc->selcb);

     return;
}

void
input_me(const char *input)
{
     DSINPUT(input);
     NOSERVRET();

     if(irc_send_raw(hftirc->selsession, "PRIVMSG %s :\x01" "ACTION %s\x01",
                    hftirc->selcb->name, input))
          WARN("Error", "Can't send action message");
     else
          ui_print_buf(hftirc->selcb, " %c* %s%c %s", B, hftirc->selsession->nick, B, input);

     return;
}

void
input_msg(const char *input)
{
     int i, b = 0;
     char nick[NICKLEN] = { 0 };
     char msg[BUFSIZE] = { 0 };
     ChanBuf *cb;

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
          if(irc_send_raw(hftirc->selsession, "PRIVMSG %s :%s", nick, msg))
               WARN("Error", "Can't send MSG");
          else if((cb = find_buf(hftirc->selsession, nick)))
                ui_print_buf(cb, "<%s> %s", hftirc->selsession->nick, msg);
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
          WARN("Error", "Usage: /kick <nick> <reason>");

          return;
     }

     if(strlen(reason) > 0)
		irc_send_raw(hftirc->selsession, "KICK %s %s :%s",
                    hftirc->selcb->name, nick, reason);
	else
          irc_send_raw(hftirc->selsession, "KICK %s %s",
                     hftirc->selcb->name, nick);

     return;
}

void
input_whois(const char *input)
{
     DSINPUT(input);
     NOSERVRET();

     if(strlen(input) > 0)
     {
          if(irc_send_raw(hftirc->selsession, "WHOIS %s %s", input, input))
               WARN("Error", "Can't use WHOIS");
     }
     /* No input -> whois current private nick */
     else
     {
          if(ISCHAN(hftirc->selcb->name[0]))
               WARN("Error", "Usage: /whois <nick>");
          else if(irc_send_raw(hftirc->selsession, "WHOIS %s %s",
                         hftirc->selcb->name, hftirc->selcb->name))
               WARN("Error", "Can't use WHOIS");
     }

     return;
}

void
input_query(const char *input)
{
     ChanBuf *cb;
     NickStruct *ns;

     DSINPUT(input);
     NOSERVRET();

     if(strlen(input) > 0)
     {
          for(cb = hftirc->cbhead; cb; cb = cb->next)
               if(!strcmp(cb->name, input) && cb->session == hftirc->selsession)
               {
                    ui_buf_set(cb->id);
                    return;
               }

          cb = ui_buf_new(input, hftirc->selsession);
          ns = nickstruct_set((char *)input);
          nick_attach(cb, ns);
          ui_buf_set(cb->id);
          ui_print_buf(cb, "  *** Query with %s", input);
     }
     else
          WARN("Error", "Usage: /query <nick>");

     return;
}

void
input_close(const char *input)
{
     if(hftirc->selcb != hftirc->statuscb)
          return;

     if(ISCHAN(hftirc->selcb->name[0]))
          input_part(NULL);
     else
          ui_buf_close(hftirc->selcb);

     return;
}

void
input_raw(const char *input)
{
     DSINPUT(input);
     NOSERVRET();

     if(strlen(input) > 0)
     {
          if(irc_send_raw(hftirc->selsession, "%s", input))
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
     else if(irc_send_raw(hftirc->selsession, "MODE %s %s", hftirc->selsession->nick, input))
          WARN("Error", "Can't set user mode");

     return;
}

void
input_serv(const char *input)
{
     IrcSession *is;

     DSINPUT(input);

     if(hftirc->selcb != hftirc->statuscb)
          return;

     if(strlen(input) > 0)
     {
          for(is = hftirc->sessionhead; is; is = is->next)
               if(!strcasecmp(is->name, input))
                    hftirc->selsession = is;
     }
     else
          hftirc->selsession
               = (!hftirc->selsession->next ? hftirc->sessionhead : hftirc->selsession->next);

     refresh();

     return;
}

void
input_redraw(const char *input)
{
     endwin();
     ui_init();
     ui_buf_set(hftirc->selcb->id);

     return;
}

void
input_connect(const char *input)
{
     IrcSession *is;
     ServInfo defsi = { " ", " ", " ", 6667, "hftircuser", " ", "HFTIrcuser", "HFTIrcuser"};

     DSINPUT(input);

     if(strlen(input) > 0)
     {
          /* Check if serv is already used in hftirc */
          for(is = hftirc->sessionhead; is; is = is->next)
               if(!strcmp(input, is->server))
               {
                    WARN("Warning", "Already connected on this server");
                    return;
               }

          is = irc_session();

          if(irc_connect(is, input, input, defsi.port, defsi.password, defsi.nick, defsi.username, defsi.realname))
               ui_print_buf(hftirc->statuscb, "Error: Can't connect to %s", input);

          hftirc->selsession = is;
     }
     else
          WARN("Error", "Usage: /connect <adress>  or  /server <adress>");

     return;
}

void
input_disconnect(const char *input)
{
     IrcSession *is;

     DSINPUT(input);
     NOSERVRET();

     /* Session specified in input ? */
     if(strlen(input) > 0)
     {
          for(is = hftirc->sessionhead; is; is = is->next)
               if(!strcasecmp(is->name, input))
                    break;
     }
     else
          is = hftirc->selsession;

     irc_disconnect(is);

     ui_print_buf(0, "[%s] *** %c%s%c is now Disconnected", is->name, B, is->name, B);

     return;
}

void
input_away(const char *input)
{
     IrcSession *is;

     DSINPUT(input);
     NOSERVRET();

     if(strlen(input) > 0)
     {
          for(is = hftirc->sessionhead; is; is = is->next)
               if(is->connected || irc_send_raw(is, "AWAY :%s", input))
                    WARN("Error", "Can't send AWAY");

     }
     else
          for(is = hftirc->sessionhead; is; is = is->next)
               if(is->connected || irc_send_raw(is,  "AWAY :"))
                    WARN("Error", "Can't send AWAY");

     return;
}

/* /ctcp nick version */
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
          /* parse second arg in input */
          /* input may be = "nick version" */
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

     if(irc_send_raw(hftirc->selsession, "PRIVMSG %s :\x01%s\x01", nick, request))
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
          if((i = atoi(input)))
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
     ChanBuf *cb;

     ui_print_buf(hftirc->statuscb, "[Hftirc] %cBuffers list%c:", B, B);

     for(cb = hftirc->cbhead; cb; cb = cb->next)
          ui_print_buf(hftirc->statuscb, "[Hftirc] - %d: %s", cb->id, cb->name);

     return;
}

void
input_buffer_prev(const char *input)
{
     DSINPUT(input);
     NOSERVRET();

     ui_buf_set(hftirc->prevcb->id);

     return;
}


void
input_buffer_swap(const char *input)
{
    int i;

    if(strlen(input) > 0)
    {
        if((i = atoi(input)))
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
     NOSERVRET();

     if(!say_without_cmd)
          DSINPUT(input);
     else
          say_without_cmd = 0;

     if(strlen(input) > 0)
     {
          if(irc_send_raw(hftirc->selsession, "PRIVMSG %s :%s",
                         hftirc->selcb->name, input))
               WARN("Error", "Can't send message");
          else
               /* Write what we said on buffer, with cyan color */
               ui_print_buf(hftirc->selcb, "%s", colorstr(Cyan, "<%s> %s",
                              hftirc->selsession->nick, input));
     }
     else
          WARN("Error", "Usage: /say <message>");

     return;
}

void
input_reconnect(const char *input)
{
     DSINPUT(input);

     if(hftirc->selsession->connected)
          irc_disconnect(hftirc->selsession);

     if(irc_connect(hftirc->selsession,
                    hftirc->selsession->server,
                    hftirc->selsession->name,
                    hftirc->selsession->port,
                    hftirc->selsession->password,
                    hftirc->selsession->nick,
                    hftirc->selsession->username,
                    hftirc->selsession->realname))
          ui_print_buf(0, "Error: Can't connect to %s", hftirc->selsession->server);

     return;
}

void
input_nicklist_scroll(const char *input)
{
     DSINPUT(input);
     NOSERVRET();

     if(strlen(input) > 0)
          ui_nicklist_scroll(atoi(input));
     else
          WARN("Error", "Usage: /nicklist_scroll +/-<value>");

     return;
}

void
input_nicklist_toggle(const char *input)
{
     ui_nicklist_toggle();

     return;
}

void
input_color_theme(const char *input)
{
     DSINPUT(input);

     if(strlen(input) > 0)
          ui_set_color_theme(color_to_id((char*)input));
     else
          WARN("Error", "Usage: /color_theme <colorname>");

     return;
}

void
input_invite(const char *input)
{
     DSINPUT(input);

     /* invite command take 1 argument: the nick. */
     /* the concerned channel is the selected one */
     if(strlen(input) > 0 && ISCHAN(hftirc->selcb->name[0]))
     {
          if(irc_send_raw(hftirc->selsession, "INVITE %s %s", input, hftirc->selcb->name))
               WARN("Error", "Can't use invite command");
     }
     else /* command is not used correctly (no input or not a channel) */
          WARN("Error", "Usage: /invite <nickname>  selected buffer must be a channel");

     return;
}

void
input_mode(const char *input)
{
     int i;
     char mode[NICKLEN] = { 0 };
     char nick[NICKLEN] = { 0 };

     DSINPUT(input);
     NOSERVRET();

     if(strlen(input) > 0 && ISCHAN(hftirc->selcb->name[0]))
     {
          for(i = 0; input[i] != ' ' && input[i]; mode[i] = input[i], ++i);

          if(input[i] == ' ')
          {
               input += strlen(mode);
               DSINPUT(input);
               for(i = 0; input[i]; nick[i] = input[i], ++i);
          }
     }
     else
     {
          WARN("Error", "Usage: /mode <mode> <nick>");

          return;
     }

     if(strlen(nick) > 0)
          irc_send_raw(hftirc->selsession, "MODE %s %s :%s",
                    hftirc->selcb->name, mode, nick);
     else
          irc_send_raw(hftirc->selsession, "MODE %s %s",
                    hftirc->selcb->name, mode);

     return;
}

void
input_clear(const char *input)
{
     if(!hftirc->selcb->scrollpos)
     {
          ui_screen_clear();
     }

     return;
}

void
input_scrollclear(const char *input)
{
     if(!hftirc->selcb->scrollpos)
     {
          ui_screen_clear();
     }

     hftirc->selcb->bufpos = 0;

     return;
}

