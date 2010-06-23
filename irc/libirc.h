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

#ifndef LIBIRC_H
#define LIBIRC_H

#include <arpa/inet.h>

#define STATE_CONNECTING 1
#define STATE_CONNECTED  2

typedef struct IrcSession IrcSession;
typedef struct IrcCallbacks IrcCallbacks;

/* Event callback functions type */
typedef void (*IrcEventCallback)(IrcSession *s, const char *event, const char *origin, const char **params, unsigned int count);

struct IrcCallbacks
{
     IrcEventCallback connect;
     IrcEventCallback quit;
     IrcEventCallback join;
     IrcEventCallback nick;
     IrcEventCallback part;
     IrcEventCallback topic;
     IrcEventCallback privmsg;
     IrcEventCallback channel;
     IrcEventCallback kick;
     IrcEventCallback mode;
     IrcEventCallback umode;
     IrcEventCallback notice;
     IrcEventCallback invite;
     IrcEventCallback action;
     IrcEventCallback numeric;
     IrcEventCallback unknown;
     IrcEventCallback ctcp_rep;
     IrcEventCallback ctcp_req;
};

struct IrcSession
{
     int sock;
     short port;
     char *server;
     char *nick;
     char *username;
     char *realname;
     char *password;

	char	inbuf[BUFSIZE];
	char outbuf[BUFSIZE];

     int motd_received, state;
	unsigned int inoffset;
	unsigned int outoffset;

     struct in_addr	local_addr;

     IrcCallbacks callbacks;
};

/* Internal util */
void irc_process_incoming_data(IrcSession *session, int process_length);
void irc_target_get_nick(const char *target, char *nick, size_t size);
int irc_findcrlf(const char *buf, int length);

/* Irc lib functions  */
int irc_process_select_descriptors(IrcSession *session, fd_set *inset, fd_set *outset);
int irc_add_select_descriptors(IrcSession *session, fd_set *inset, fd_set *outset, int *maxfd);
int irc_connect(IrcSession *s, const char *server, unsigned short port, const char *password,
          const char *nick, const char *username,	const char *realname);
void irc_disconnect(IrcSession *s);
int irc_send_raw(IrcSession *session, const char *format, ...);
IrcSession* irc_session(IrcCallbacks *callbacks);

#endif /* LIBIRC_H */
