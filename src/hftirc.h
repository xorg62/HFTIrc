/*
 * hftirc2 by Martin Duquesnoy <xorg62@gmail.com>
 * For license, see COPYING
 */

#ifndef HFTIRC_H
#define HFTIRC_H

#define _XOPEN_SOURCE_EXTENDED 1
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <locale.h>
#include <err.h>
#include <errno.h>
#include <sys/queue.h>
#include <wchar.h>
#include <wctype.h>

#if !defined (__NetBSD__)
    #include <ncurses.h>
#else
    #include <ncurses/ncurses.h>
#endif

#define BUFSIZE      (4096)
#define HISTOLEN     (64)
#define BUFHISTOLEN  (512)
#define MAX_PATH_LEN (8192)
#define TIMEOUT_IRC  (30)
#define NICKLEN      (24)

typedef unsigned long Flags;

struct session_info
{
     short port;
     char *server;
     char *name;
     char *nick;
     char *username;
     char *realname;
     char *password;
     char **autojoin;
     int nautojoin;
     SLIST_ENTRY(session_info) next;
};

struct session
{
     struct session_info *info;
     struct buffer *buf_event;
     int sock;
     time_t last_response;
     char *mode;
#define SESSION_CONNECTED 0x01
#define SESSION_MOTD      0x02
     Flags flags;

     /* Socket parse utility */
     char inbuf[BUFSIZE];
     unsigned int inoffset;

     SLIST_ENTRY(session) next;
};

struct nick
{
     char *nick;
     char rank;
     SLIST_ENTRY(nick) next;
};

struct buffer_line
{
     char *line;
     int id;
     TAILQ_ENTRY(buffer_line) next;
};

struct buffer
{
     struct session *session;
     struct buffer_line *scrollb;
     int id;
#define ACT_NO     0
#define ACT_NORMAL 1
#define ACT_HL     2
     int act, nline, nscroll;
     char *name;
     char *topic;
     TAILQ_HEAD(blsub, buffer_line) lines;
     SLIST_HEAD(, nick) nicks;
     TAILQ_ENTRY(buffer) next;
};

struct ui
{
     /* Ncurses windows */
     WINDOW *mainwin;
     WINDOW *inputwin;
     WINDOW *statuswin;
     WINDOW *topicwin;
     WINDOW *nicklistwin;
     int bg, c, tcolor;

     /* Input buffer struct */
     struct inputbuffer
     {
          wchar_t buffer[BUFSIZE];
          wchar_t histo[HISTOLEN][BUFSIZE];
          unsigned int nhisto, histpos, hits, found;
          wchar_t prev;
          int pos, cpos, split, spting;
     } ib;
};

struct input
{
     void (*func)(const char *cmd);
     char *cmd;
};

struct hftirc
{
     struct ui ui;
     struct buffer *bufsel, *prevbufsel;
     struct session *sessionsel;
     char *confpath, dateformat[256];
#define HFTIRC_RUNNING     0x01
#define HFTIRC_FIRST_TIME  0x02
#define HFTIRC_BELL        0x04
#define HFTIRC_NICKLIST    0x08
#define HFTIRC_LASTLINEPOS 0x10
#define HFTIRC_NICKCOLOR   0x20
     Flags flags;
#define IGNORE_JOIN   0x01
#define IGNORE_QUIT   0x02
#define IGNORE_MODE   0x04
#define IGNORE_CTCP   0x08
#define IGNORE_NOTICE 0x10
#define IGNORE_PART   0x20
#define IGNORE_NICK   0x40
     Flags ignore_flags;

     /* Linked list heads */
     struct
     {
          SLIST_HEAD(, session_info) session_info;
          SLIST_HEAD(, session) session;
          TAILQ_HEAD(bsub, buffer) buffer;
     } h;
} H;

#endif
