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

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

#include "../hftirc.h"

IrcSession*
irc_session(IrcCallbacks *callbacks)
{
     IrcSession *s;

     s = calloc(1, sizeof(IrcSession));

     s->sock = -1;

     memcpy (&s->callbacks, callbacks, sizeof(IrcCallbacks));

     return s;
}

int
irc_connect(IrcSession *s, const char *server, unsigned short port, const char *password,
          const char *nick, const char *username,const char *realname)
{
     struct addrinfo a, *res;
     char sport[8];

     if (!server || !nick)
          return 1;

     s->username = (username) ? strdup(username) : NULL;
     s->password = (password) ? strdup(password) : NULL;
     s->realname = (realname) ? strdup(realname) : NULL;

     s->nick   = strdup(nick);
     s->server = strdup(server);

     memset(&a, 0, sizeof a);
     a.ai_family   = AF_INET;
     a.ai_socktype = SOCK_STREAM;

     sprintf(sport, "%d", port);
     if(getaddrinfo(server, sport, &a, &res))
          return 1;

     if((s->sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1)
          return 1;
     else
          connect(s->sock, res->ai_addr, res->ai_addrlen);

     /* Identify */
    irc_send_raw(s, "PASS %s", password);
    irc_send_raw(s, "NICK %s", nick);
    irc_send_raw(s, "USER %s %s %s :%s", username, username, username, realname);

    s->state = STATE_CONNECTED;

    return 0;
}

void
irc_disconnect(IrcSession *s)
{
     if(s->sock >= 0)
          close(s->sock);

     s->sock = -1;

     s->state = 0;

     return;
}

int
irc_process_select_descriptors(IrcSession *session, fd_set *inset, fd_set *outset)
{
     int length, offset;
     unsigned int amount;

     if(session->sock < 0 || session->state != STATE_CONNECTED)
          return 1;

     /* Read time */
     if(FD_ISSET(session->sock, inset))
     {
          amount = (sizeof(session->inbuf) - 1) - session->inoffset;

          if((length = recv(session->sock, session->inbuf + session->inoffset, amount, 0)) <= 0)
               return 1;

          session->inoffset += length;

          /* Process the incoming data */
          for(; ((offset = irc_findcrlf(session->inbuf, session->inoffset)) > 0)
                    ; session->inoffset -= offset)
          {
               irc_process_incoming_data (session, offset - 2);

               if(session->inoffset - offset > 0)
                    memmove(session->inbuf, session->inbuf + offset, session->inoffset - offset);
          }
     }

     /* Send time */
     if(FD_ISSET(session->sock, outset))
     {
          if((length = send(session->sock, session->outbuf, session->outoffset, 0)) <= 0)
               return 1;

          if(session->outoffset - length > 0)
               memmove (session->outbuf, session->outbuf + length, session->outoffset - length);

          session->outoffset -= length;
     }

     return 0;
}

int
irc_add_select_descriptors(IrcSession *session, fd_set *inset, fd_set *outset, int *maxfd)
{
     if(session->sock < 0 || session->state != STATE_CONNECTED)
          return 1;

     if(session->inoffset < (sizeof (session->inbuf) - 1))
          FD_SET(session->sock, inset);

     if(irc_findcrlf(session->outbuf, session->outoffset) > 0)
          FD_SET(session->sock, outset);

     if(*maxfd < session->sock)
          *maxfd = session->sock;

     return 0;
}

int
irc_send_raw(IrcSession *session, const char *format, ...)
{
     char buf[BUFSIZE];
     va_list va_alist;

     va_start(va_alist, format);
     vsnprintf(buf, sizeof(buf), format, va_alist);
     va_end(va_alist);

     if((strlen(buf) + 2) >= (sizeof(session->outbuf) - session->outoffset))
          return 1;

     strcpy(session->outbuf + session->outoffset, buf);

     session->outoffset += strlen(buf);
     session->outbuf[session->outoffset++] = '\r';
     session->outbuf[session->outoffset++] = '\n';

     return 0;
}

void
irc_process_incoming_data (IrcSession *session, int process_length)
{
     char *p, *s, buf[BUFSIZE], nbuf[6], nickbuf[256], ctcp_buf[128];

     ;const char *command = 0, *prefix = 0, *params[11];
     int code = 0, paramindex = 0;
     unsigned int msglen;

     if(process_length > sizeof(buf))
          return;

     memcpy(buf, session->inbuf, process_length);
     buf[process_length] = '\0';

     memset((char *)params, 0, sizeof(params));
     p = buf;

    /*
     * From RFC 1459:
     *  <message>  ::= [':' <prefix> <SPACE> ] <command> <params> <crlf>
     *  <prefix>   ::= <servername> | <nick> [ '!' <user> ] [ '@' <host> ]
     *  <command>  ::= <letter> { <letter> } | <number> <number> <number>
     *  <SPACE>    ::= ' ' { ' ' }
     *  <params>   ::= <SPACE> [ ':' <trailing> | <middle> <params> ]
     *  <middle>   ::= <Any *non-empty* sequence of octets not including SPACE
     *                 or NUL or CR or LF, the first of which may not be ':'>
     *  <trailing> ::= <Any, possibly *empty*, sequence of octets not including
     *                   NUL or CR or LF>
     */

     /* Parse <prefix> */
     if(buf[0] == ':')
     {
          for(; *p && *p != ' '; ++p);

          *p++ = '\0';

          /* Skip the leading colon */
          prefix = buf + 1;

     }

     /* Parse <command> */
     /* Numerical */
     if(isdigit(p[0]) && isdigit(p[1]) && isdigit(p[2]))
     {
          p[3] = '\0';
          code = atoi (p);
          p += 4;
     }
     /* Other */
     else
     {
          s = p;
          for(; *p && *p != ' '; ++p);
          *p++ = '\0';
          command = s;
     }

     /* Parse middle/params */
     while(*p && paramindex < 10)
     {
          /* beginning from ':', this is the last param */
          if(*p == ':')
          {
               params[paramindex++] = p + 1;
               break;
          }

          /* Just a param */
          for (s = p; *p && *p != ' '; ++p);

          params[paramindex++] = s;

          if(!*p)
               break;

          *p++ = '\0';
     }

     /* Handle PING/PONG */
     if(command && !strcmp(command, "PING") && params[0])
     {
          irc_send_raw(session, "PONG %s", params[0]);
          return;
     }

     /* Dump */
     if(code)
     {
          /* We use session->motd_received to check whether it is the first
           * RPL_ENDOFMOTD or ERR_NOMOTD after the connection. */
          if((code == 376 || code == 422) && !session->motd_received)
          {
               session->motd_received = 1;

               if(session->callbacks.connect)
                    (*session->callbacks.connect)(session, "CONNECT", prefix, params, paramindex);
          }

          if(session->callbacks.numeric)
          {
               sprintf(nbuf, "%d", code);
               (*session->callbacks.numeric)(session, nbuf, prefix, params, paramindex);
          }
     }
     else
     {
          if(!strcmp(command, "NICK"))
          {
               /*
                * If we're changed our nick, we should save it.
                */
               irc_target_get_nick(prefix, nickbuf, sizeof(nickbuf));

               if(!strcmp(nickbuf, session->nick) && paramindex > 0)
               {
                    free(session->nick);
                    session->nick = strdup(params[0]);
               }

               if(session->callbacks.nick)
                    (*session->callbacks.nick)(session, command, prefix, params, paramindex);
          }
          else if(!strcmp(command, "QUIT"))
          {
               if(session->callbacks.quit)
                    (*session->callbacks.quit)(session, command, prefix, params, paramindex);
          }
          else if(!strcmp(command, "JOIN"))
          {
               if(session->callbacks.join)
                    (*session->callbacks.join)(session, command, prefix, params, paramindex);
          }
          else if(!strcmp(command, "PART"))
          {
               if(session->callbacks.part)
                    (*session->callbacks.part)(session, command, prefix, params, paramindex);
          }
          else if(!strcmp (command, "MODE"))
          {
               if (paramindex > 0 && !strcmp(params[0], session->nick))
               {
                    params[0] = params[1];
                    paramindex = 1;

                    if(session->callbacks.umode)
                         (*session->callbacks.umode)(session, command, prefix, params, paramindex);
               }
               else
               {
                    if(session->callbacks.mode)
                         (*session->callbacks.mode)(session, command, prefix, params, paramindex);
               }
          }
          else if(!strcmp (command, "TOPIC"))
          {
               if(session->callbacks.topic)
                    (*session->callbacks.topic)(session, command, prefix, params, paramindex);
          }
          else if (!strcmp(command, "KICK"))
          {
               if(session->callbacks.kick)
                    (*session->callbacks.kick)(session, command, prefix, params, paramindex);
          }
          else if(!strcmp(command, "PRIVMSG"))
          {
               if(paramindex > 1)
               {
                    msglen = strlen(params[1]);

                    /* Check for CTCP request (a CTCP message starts from 0x01 and ends by 0x01) */
                    if(params[1][0] == 0x01 && params[1][msglen-1] == 0x01)
                    {
                         msglen -= 2;

                         if(msglen > sizeof(ctcp_buf) - 1)
                              msglen = sizeof(ctcp_buf) - 1;

                         memcpy(ctcp_buf, params[1] + 1, msglen);
                         ctcp_buf[msglen] = '\0';

                         if(strstr(ctcp_buf, "ACTION ") == ctcp_buf && session->callbacks.action)
                         {
                              params[1] = ctcp_buf + strlen("ACTION ");
                              paramindex = 2;

                              (*session->callbacks.action)(session, "ACTION", prefix, params, paramindex);
                         }
                         else
                         {
                              params[0] = ctcp_buf;
                              paramindex = 1;

                              if(session->callbacks.ctcp_req)
                                   (*session->callbacks.ctcp_req)(session, "CTCP", prefix, params, paramindex);
                         }
                    }
                    else if(!strcmp(params[0], session->nick))
                    {
                         if(session->callbacks.privmsg)
                              (*session->callbacks.privmsg)(session, command, prefix, params, paramindex);
                    }
                    else
                    {
                         if (session->callbacks.channel)
                              (*session->callbacks.channel)(session, command, prefix, params, paramindex);
                    }
               }
          }
          else if(!strcmp(command, "NOTICE"))
          {
               msglen = strlen(params[1]);

               /* Check for CTCP request (a CTCP message starts from 0x01 and ends by 0x01) */
               if(paramindex > 1 && params[1][0] == 0x01 && params[1][msglen-1] == 0x01)
               {
                    msglen -= 2;

                    if(msglen > sizeof(ctcp_buf) - 1)
                         msglen = sizeof(ctcp_buf) - 1;

                    memcpy(ctcp_buf, params[1] + 1, msglen);
                    ctcp_buf[msglen] = '\0';

                    params[0] = ctcp_buf;
                    paramindex = 1;

                    if(session->callbacks.ctcp_rep)
                         (*session->callbacks.ctcp_rep)(session, "CTCP", prefix, params, paramindex);
               }
               else
               {
                    if(session->callbacks.notice)
                         (*session->callbacks.notice)(session, command, prefix, params, paramindex);
               }
          }
          else if(!strcmp(command, "INVITE"))
          {
               if(session->callbacks.invite)
                    (*session->callbacks.invite)(session, command, prefix, params, paramindex);
          }
          else if(!strcmp (command, "KILL"))
          {
               ; /* ignore this event - not all servers generate this */
          }
          else
          {
               /* Unknow */
               if(session->callbacks.unknown)
                    (*session->callbacks.unknown)(session, command, prefix, params, paramindex);
          }
     }

     return;
}

int
irc_findcrlf(const char *buf, int length)
{
     int offset = 0;

     for(; offset < (length - 1); ++offset)
          if(buf[offset] == '\r' && buf[offset + 1] == '\n')
               return offset + 2;

     return 0;
}

void
irc_target_get_nick(const char *target, char *nick, size_t size)
{
     char *p;
     unsigned int len;

     if((p = strstr (target, "!")))
          len = p - target;
     else
          len = strlen(target);

     if(len > size - 1)
          len = size - 1;

     memcpy(nick, target, len);

     nick[len] = '\0';

     return;
}


