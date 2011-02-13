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

IrcSession*
irc_session(void)
{
     IrcSession *s;

     s = calloc(1, sizeof(IrcSession));

     s->sock = -1;
     s->connected = 0;

     HFTLIST_ATTACH(hftirc->sessionhead, s);

     return s;
}

int
irc_connect(IrcSession *s,
          const char *server,
          const char *servername,
          unsigned short port,
          const char *password,
          const char *nick,
          const char *username,
          const char *realname)
{
     struct hostent *hp;
     struct sockaddr_in a;

     if(!server || !nick)
          return 1;

     s->username = (username) ? strdup(username) : NULL;
     s->password = (password) ? strdup(password) : NULL;
     s->realname = (realname) ? strdup(realname) : NULL;

     s->nick   = strdup(nick);
     s->server = strdup(server);
     s->name   = strdup(servername);

     if(!(hp = gethostbyname(server)))
          return 1;

     a.sin_family = AF_INET;
     a.sin_port   = htons(s->port ? s->port : 6667);

     memcpy(&a.sin_addr, hp->h_addr_list[0], (size_t) hp->h_length);

     /* Create socket */
     if((s->sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
          return 1;

     /* connect to the server */
     if(connect(s->sock, (const struct sockaddr*) &a, sizeof(a)) < 0)
          return 1;

     /* Identify */
    irc_send_raw(s, "PASS %s", password);
    irc_send_raw(s, "NICK %s", nick);
    irc_send_raw(s, "USER %s localhost %s :%s", username, server, realname);

    s->connected = 1;

    return 0;
}

void
irc_disconnect(IrcSession *s)
{
     if(s->sock >= 0)
          close(s->sock);

     s->sock = -1;
     s->connected = 0;

     /* Signal disconnection on each channel */
     msg_sessbuf(s, "  *** Server disconnected by client");

     return;
}

int
irc_run_process(IrcSession *s, fd_set *inset)
{
     int i, length, offset;
     unsigned int amount;

     if(s->sock < 0 || !s->connected)
     {
          /* Signal disconnection on each channel */
          msg_sessbuf(s, "  *** Server disconnected");

          return 1;
     }

     /* Read incoming socket */
     if(FD_ISSET(s->sock, inset))
     {
          amount = (sizeof(s->inbuf) - 1) - s->inoffset;

          if((length = recv(s->sock, s->inbuf + s->inoffset, amount, 0)) <= 0)
               return 1;

          s->inoffset += length;

          /* Parse incoming data */
          do
          {
               /* Find CR-LF sequence separators */
               for(i = offset = 0; i < ((int)(s->inoffset) - 1); ++i)
                    if(s->inbuf[i] == '\r' && s->inbuf[i + 1] == '\n')
                    {
                         offset = i + 2;
                         break;
                    }

               irc_manage_event(s, offset - 2);

               if(s->inoffset - offset > 0)
                    memmove(s->inbuf, s->inbuf + offset, s->inoffset - offset);

               s->inoffset -= offset;
          } while(offset > 0);
     }

     return 0;
}

int
irc_send_raw(IrcSession *s, const char *format, ...)
{
     char buf[BUFSIZE];
     va_list va_alist;

     va_start(va_alist, format);
     vsnprintf(buf, sizeof(buf), format, va_alist);
     va_end(va_alist);

     if(!s->sock)
          return 1;

     strcat(buf, "\r\n");

     send(s->sock, buf, strlen(buf), 0);

     return 0;
}

void
irc_parse_in(char *buf,
          const char *prefix,
          const char *command,
          const char **params,
          int *code,
          int *paramindex)
{
     char *p = buf, *s = NULL;

     /* Parse prefix */
     if(buf[0] == ':')
     {
          for(; *p && *p != ' '; ++p);
          *p++ = '\0';
          strcpy((char *)prefix, buf + 1);
     }

     /* Parse command */
     if(isdigit((int)p[0]) && isdigit((int)p[1]) && isdigit((int)p[2]))
     {
          p[3] = '\0';
          *code = atoi (p);
          p += 4;
     }
     else
     {
          for(s = p; *p && *p != ' '; ++p);
          *p++ = '\0';
          strcpy((char *)command, s);
     }

     /* Parse params */
     for(;*p && *paramindex < 10; *p++ = '\0')
     {
          if(*p == ':')
          {
               params[(*paramindex)++] = p + 1;
               break;
          }

          for(s = p; *p && *p != ' '; ++p);

          params[(*paramindex)++] = s;

          if(!*p)
               break;
     }

     return;
}

void
irc_manage_event(IrcSession *session, int process_length)
{
     char buf[BUFSIZE], ctcp_buf[128];
     const char command[BUFSIZE] = { 0 };
     const char prefix[BUFSIZE] =  { 0 };
     const char *params[11];
     int code = 0, paramindex = 0;
     unsigned int msglen;

     if(process_length > sizeof(buf))
          return;

     memcpy(buf, session->inbuf, process_length);
     buf[process_length] = '\0';

     memset((char *)params, 0, sizeof(params));

     /* Parse socket */
     irc_parse_in(buf, prefix, command, params, &code, &paramindex);

     /* Handle auto PING/PONG */
     if(strlen(command) && !strcmp(command, "PING") && params[0])
     {
          irc_send_raw(session, "PONG %s", params[0]);
          return;
     }

     /* Numerical */
     if(code)
     {
          if((code == 376 || code == 422) && !session->motd_received)
          {
               session->motd_received = 1;
               event_connect(session, "CONNECT", prefix, params, paramindex);
          }

          event_numeric(session, code, prefix, params, paramindex);
     }
     else
     {
          /* Quit */
          if(!strcmp(command, "QUIT"))
               event_quit(session, command, prefix, params, paramindex);

          /* Join */
          else if(!strcmp(command, "JOIN"))
               event_join(session, command, prefix, params, paramindex);

          /* Part */
          else if(!strcmp(command, "PART"))
               event_part(session, command, prefix, params, paramindex);

          /* Invite */
          else if(!strcmp(command, "INVITE"))
               event_invite(session, command, prefix, params, paramindex);

          /* Topic */
          else if(!strcmp (command, "TOPIC"))
               event_topic(session, command, prefix, params, paramindex);

          /* Kick */
          else if (!strcmp(command, "KICK"))
               event_kick(session, command, prefix, params, paramindex);

          /* Nick */
          else if(!strcmp(command, "NICK"))
          {
               if(!strncmp(prefix, session->nick, sizeof(session->nick)) && paramindex > 0)
               {
                    free(session->nick);
                    session->nick = strdup(params[0]);
               }

               event_nick(session, command, prefix, params, paramindex);
          }

          /* Mode / User mode */
          else if(!strcmp(command, "MODE"))
          {
               /* User mode case */
               if(paramindex > 0 && !strcmp(params[0], session->nick))
               {
                    params[0] = params[1];
                    paramindex = 1;
               }

               event_mode(session, command, prefix, params, paramindex);
          }

          /* Privmsg */
          else if(!strcmp(command, "PRIVMSG"))
          {
               if(paramindex > 1)
               {
                    msglen = strlen(params[1]);

                    /* CTCP request */
                    if(params[1][0] == 0x01 && params[1][msglen - 1] == 0x01)
                    {
                         msglen -= 2;

                         if(msglen > sizeof(ctcp_buf) - 1)
                              msglen = sizeof(ctcp_buf) - 1;

                         memcpy(ctcp_buf, params[1] + 1, msglen);
                         ctcp_buf[msglen] = '\0';

                         if(strstr(ctcp_buf, "ACTION ") == ctcp_buf)
                         {
                              params[1] = ctcp_buf + strlen("ACTION ");
                              paramindex = 2;
                              event_action(session, "ACTION", prefix, params, paramindex);
                         }
                         else
                         {
                              params[0] = ctcp_buf;
                              paramindex = 1;
                              event_ctcp(session, "CTCP", prefix, params, paramindex);
                         }
                    }
                    /* Private message */
                    else if(!strcmp(params[0], session->nick))
                         event_privmsg(session, command, prefix, params, paramindex);
                    /* Channel message */
                    else
                         event_channel(session, command, prefix, params, paramindex);
               }

          }

          /* Notice */
          else if(!strcmp(command, "NOTICE"))
          {
               msglen = strlen(params[1]);

               /* CTCP request */
               if(paramindex > 1 && params[1][0] == 0x01 && params[1][msglen - 1] == 0x01)
               {
                    msglen -= 2;

                    if(msglen > sizeof(ctcp_buf) - 1)
                         msglen = sizeof(ctcp_buf) - 1;

                    memcpy(ctcp_buf, params[1] + 1, msglen);
                    ctcp_buf[msglen] = '\0';

                    params[0] = ctcp_buf;
                    paramindex = 1;

                    dump_event(session, "CTCP", prefix, params, paramindex);
               }
               /* Normal notice */
               else
                    event_notice(session, command, prefix, params, paramindex);
          }

          /* Unknown */
          else
               dump_event(session, command, prefix, params, paramindex);
     }

     return;
}

void
irc_init(void)
{
     int i;
     IrcSession *is;

     /* Connection to conf servers */
     for(is = hftirc->sessionhead; is && i < hftirc->conf.nserv; is = is->next, ++i)
     {
          if(!is)
               is = irc_session();

          hftirc->selsession = is;

          if(irc_connect(is,
                         hftirc->conf.serv[i].adress,
                         hftirc->conf.serv[i].name,
                         hftirc->conf.serv[i].port,
                         hftirc->conf.serv[i].password,
                         hftirc->conf.serv[i].nick,
                         hftirc->conf.serv[i].username,
                         hftirc->conf.serv[i].realname))
               ui_print_buf(0, "Error: Can't connect to %s", hftirc->conf.serv[i].adress);
     }

     return;
}

void
irc_join(IrcSession *s, const char *chan)
{
     ui_buf_new(chan, s);

     ui_buf_set(hftirc->nbuf - 1);

     return;
}



