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

#ifndef HFTIRC_H
#define HFTIRC_H

/* Libs */
#include <ncurses.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>
#include <locale.h>
#include <sys/utsname.h>
#include <libircclient.h>
#include <libirc_events.h>

/* Macro */
#define HFTIRC_VERSION    "0.01"
#define BUFSIZE           8192
#define MAXBUF            64
#define BUFLINES          512
#define NICKLEN           24
#define CHANLEN           24
#define HOSTLEN           128
#define NSERV             32
#define HFTIRC_KEY_ENTER  10
#define HISTOLEN          256
#define MAINWIN_LINES     LINES - 2
#define DATELEN           (strlen(hftirc->date.str))
#define CONFPATH          "hftirc.conf"
#define B                 C('B')
#define U                 C('_')

#define C(c) ((c) & 037)
#define LEN(x) (sizeof(x)/sizeof(x[0]))
#define WARN(t,s) ui_print_buf(0, "%s: %s", t, s)
#define DSINPUT(i) for(; i && i[0] == ' '; ++i)
#define PRINTATTR(w, attr, s)  wattron(w, attr); waddstr(w, s); wattroff(w, attr);
#define NOSERVRET(r) if(!hftirc->conf.nserv)                     \
                     {                                           \
                          WARN("Error", "You're not connected"); \
                          return r;                              \
                     }


/* Enum */
typedef enum { False, True } Bool;

/* Internal lib */
#include "parse/parse.h"

/* Structures */
typedef struct
{
     /* Ncurses windows */
     WINDOW *mainwin;
     WINDOW *inputwin;
     WINDOW *statuswin;
     WINDOW *topicwin;

     int bg, c;
     /* Input buffer struct */
     struct
     {
          wchar_t buffer[BUFSIZE];
          wchar_t histo[HISTOLEN][BUFSIZE];
          unsigned int nhisto, histpos;
          int pos;
          int cpos;
          int split;
          int spting;
     } ib;
} Ui;

/* Nick chained list */
typedef struct NickStruct NickStruct;
struct NickStruct
{
     char nick[NICKLEN];
     char rang;
     NickStruct *prev;
     NickStruct *next;
};

/* Channel buffer */
typedef struct
{
     /* For ui use */
     char *buffer[BUFLINES];
     int bufpos, scrollpos, naming;

     /* For irc info */
     unsigned int sessid;
     char name[HOSTLEN], *names;
     NickStruct *nickhead;
     char topic[BUFSIZE];
     int act;
} ChanBuf;

/* Date struct */
typedef struct
{
     struct tm *tm;
     time_t lt;
     char str[256];
} DateStruct;

/* Input struct */
typedef struct
{
     char *cmd;
     void (*func)(const char *arg);
} InputStruct;

/* Server information struct */
typedef struct
{
     char name[HOSTLEN];
     char adress[HOSTLEN];
     char password[128];
     int port;
     char nick[NICKLEN];
     char mode[NICKLEN];
     char username[256];
     char realname[256];
     char autojoin[128][CHANLEN];
     int nautojoin;
     Bool ipv6;
} ServInfo;

/* Config struct */
typedef struct
{
     char path[512];
     int nserv;
     int bell;
     ServInfo serv[NSERV];

} ConfStruct;

/* Global struct */
typedef struct
{
     int ft, running;
     int nbuf, selbuf, selses;
     irc_session_t *session[NSERV];
     irc_callbacks_t callbacks;
     ConfStruct conf;
     ChanBuf *cb;
     Ui *ui;
     DateStruct date;
} HFTIrc;


/* Prototypes */

/* config.c */
void config_parse(char *file);

/* ui.c */
void ui_init(void);
void ui_init_color(void);
int ui_color(int fg, int bg);
void ui_update_statuswin(void);
void ui_update_topicwin(void);
void ui_update_infowin(void);
void ui_print(WINDOW *w, char *str);
void ui_print_buf(int id, char *format, ...);
void ui_draw_buf(int id);
void ui_buf_new(const char *name, unsigned int id);
void ui_buf_close(int buf);
void ui_buf_set(int buf);
void ui_scroll_up(int buf);
void ui_scroll_down(int buf);
void ui_get_input(void);

/* irc.c */
void irc_init(void);
void irc_join(irc_session_t *session, const char *chan);

void irc_dump_event(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
void irc_event_numeric(irc_session_t *session, unsigned int event, const char *origin, const char **params, unsigned int count);
void irc_event_nick(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
void irc_event_mode(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
void irc_event_connect(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
void irc_event_join(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
void irc_event_part(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
void irc_event_quit(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
void irc_event_channel(irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count);
void irc_event_privmsg(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
void irc_event_notice(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
void irc_event_topic(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
void irc_event_names(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
void irc_event_action(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
void irc_event_kick(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
void irc_event_whois(irc_session_t *session, unsigned int event, const char *origin, const char **params, unsigned int count);
void irc_event_invite(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
void irc_event_ctcp(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);


/* input.c */
void input_manage(const char *input);
void input_join(const char *input);
void input_nick(const char *input);
void input_quit(const char *input);
void input_help(const char *input);
void input_names(const char *input);
void input_topic(const char *input);
void input_part(const char *input);
void input_me(const char *input);
void input_msg(const char *input);
void input_kick(const char *input);
void input_whois(const char *input);
void input_query(const char *input);
void input_close(const char *input);
void input_raw(const char *input);
void input_umode(const char *input);
void input_serv(const char *input);
void input_redraw(const char *input);
void input_connect(const char *input);
void input_disconnect(const char *input);
void input_away(const char *input);
void input_ctcp(const char *input);


/* util.c */
void update_date(void);
int find_bufid(unsigned id, const char *str);
int find_sessid(irc_session_t *session);
void nick_attach(int buf, NickStruct *nick);
void nick_detach(int buf, NickStruct *nick);
NickStruct* nickstruct_set(char *nick);

/* main.c */
void signal_handler(int signal);

/* Variables */
HFTIrc *hftirc;

#endif /* HFTIRC_H */
