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
#define _XOPEN_SOURCE_EXTENDED 1
#if !defined (__NetBSD__)
    #include <ncurses.h>
#else
    #include <ncurses/ncurses.h>
#endif

#include <wchar.h>
#include <wctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>
#include <locale.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

/* Local headers */
#include "config.h"

/* Macro */
#define HFTIRC_VERSION   "(devel version)"
#define BUFSIZE          (2048)
#define MAXBUF           (64)
#define BUFLINES         (512)
#define NICKLEN          (24)
#define CHANLEN          (24)
#define HOSTLEN          (128)
#define HISTOLEN         (256)
#define COLORMAX         (16)
#define COLOR_THEME_DEF  (COLOR_BLUE)

#define MAINWIN_LINES  (LINES - 2)
#define DATELEN        (strlen(hftirc->date.str))
#define DEF_CONF        ".config/hftirc/hftirc.conf"

#define C(c)         ((c) & 037)
#define ISCHAN(c)    ((c == '#' || c ==  '&'))
#define LEN(x)       (sizeof(x) / sizeof(x[0]))
#define WARN(t, s)   ui_print_buf(hftirc->statuscb, "%s: %s", t, s)
#define DSINPUT(i)   for(; i && i[0] == ' '; ++i)

#define PRINTATTR(w, attr, s) { \
     wattron(w, attr);          \
     waddstr(w, s);             \
     wattroff(w, attr);         \
}

#define NOSERVRET(r) do {                         \
if(!hftirc->conf.nserv || !hftirc->selsession     \
          || !hftirc->selsession->connected)      \
     {                                            \
          WARN("Error", "You're not connected");  \
          return r;                               \
     }                                            \
} while(0 /*CONSTCOND*/);

/* List macros */
#define HFTLIST_ATTACH(head, e) do { \
     if(head)                        \
          head->prev = e;            \
     e->next = head;                 \
     head = e;                       \
} while(0 /*CONSTCOND*/);
#define HFTLIST_ATTACH_END(head, type, e) do {                \
     type *last;                                              \
     for(last = head; last && last->next; last = last->next); \
     if(last) {                                               \
          last->next = e;                                     \
          cb->prev = last;                                    \
     } else                                                   \
          head = e;                                           \
     e->next = NULL;                                          \
} while(0 /*CONSTCOND*/);
#define HFTLIST_DETACH(head, type, e) do { \
     if(head == e)                         \
         head = e->next;                   \
     else if(e->prev)                      \
         e->prev->next = e->next;          \
     if(e->next)                           \
         e->next->prev = e->prev;          \
     e->next = e->prev = NULL;             \
} while(0 /*CONSTCOND*/);

/* Key and const for ui */
#define B                 (C('b'))
#define U                 (C('_'))
#define V                 (C('v'))
#define MIRC_COLOR        (C('c'))
#define HFTIRC_COLOR      (C('s'))
#define HFTIRC_END_COLOR  (15)
#define HFTIRC_KEY_ENTER  (10)
#define HFTIRC_KEY_ALTBP  (27)
#define HFTIRC_KEY_DELALL (C('u'))

/* Flags definition for Update need */
#define UNoMask        (0)
#define UTopicMask     (1 << 1) /* Need topic bar update */
#define UNickSortMask  (1 << 2) /* Need nick list sort   */
#define UNickListMask  (1 << 3) /* Need nicklist update  */

/* Typedef */
typedef enum { False, True } Bool;

/* Internal lib */
#include "parse/parse.h"

/* Structures */
typedef struct IrcSession IrcSession;
struct IrcSession
{
     int sock;
     short port;
     char *server;
     char *name;
     char *nick;
     char *username;
     char *realname;
     char *password;
     char *mode;
     char inbuf[BUFSIZE];
     int motd_received, connected;
     unsigned int inoffset;

     IrcSession *next, *prev;
};

typedef struct
{
     /* Ncurses windows */
     WINDOW *mainwin;
     WINDOW *inputwin;
     WINDOW *statuswin;
     WINDOW *topicwin;
     WINDOW *nicklistwin;

     int bg, c, nicklist;
     int tcolor;
     /* Input buffer struct */
     struct
     {
          wchar_t buffer[BUFSIZE];
          wchar_t histo[HISTOLEN][BUFSIZE];
          unsigned int nhisto, histpos, hits, found;
          wchar_t prev;
          int pos;
          int cpos;
          int split;
          int spting;
     } ib;
} Ui;

/* Nick linked list */
typedef struct NickStruct NickStruct;
struct NickStruct
{
     char nick[NICKLEN];
     char rang;

     NickStruct *next, *prev;
};

/* Channel buffer */
typedef struct ChanBuf ChanBuf;
struct ChanBuf
{
     /* For ui use */
     int id;
     char *buffer[BUFLINES];
     int bufpos, scrollpos, naming;
     int nicklistscroll, lastposbold;

     /* For irc info */
     IrcSession *session;
     char name[HOSTLEN], *names;
     NickStruct *nickhead;
     int nnick;
     char topic[BUFSIZE];
     int act;
     unsigned int umask;

     ChanBuf *next, *prev;
};

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
     int nicklist;
     int lastlinepos;
     int tcolor;
     int nickcolor;
     ServInfo *serv;
} ConfStruct;

/* Global struct */
typedef struct
{
     int ft, nbuf, running;
     ConfStruct conf;
     IrcSession *selsession, *sessionhead;
     ChanBuf *prevcb, *statuscb, *selcb, *cbhead;
     Ui *ui;
     DateStruct date;
} HFTIrc;


/* Prototypes */

/* config.c */
void config_parse(void);

/* ui.c */
void ui_init(void);
void ui_init_color(void);
int ui_color(int fg, int bg);
void ui_update_statuswin(void);
void ui_update_topicwin(void);
void ui_update_infowin(void);
void ui_update_nicklistwin(void);
void ui_print(WINDOW *w, char *str, int n);
void ui_print_buf(ChanBuf *cb, char *format, ...);
void ui_draw_buf(ChanBuf *cb);
ChanBuf *ui_buf_new(const char *name, IrcSession *session);
void ui_buf_close(ChanBuf *cb);
void ui_buf_set(int buf);
void ui_buf_swap(int buf);
void ui_scroll_up(ChanBuf *cb);
void ui_scroll_down(ChanBuf *cb);
void ui_nicklist_toggle(void);
void ui_nicklist_scroll(int v);
void ui_refresh_curpos(void);
void ui_set_color_theme(int col);
void ui_get_input(void);

/* event.c */
void dump_event(IrcSession *session, const char *event, const char *origin, const char **params, unsigned int count);
void event_numeric(IrcSession *session, unsigned int event, const char *origin, const char **params, unsigned int count);
void event_nick(IrcSession *session, const char *event, const char *origin, const char **params, unsigned int count);
void event_mode(IrcSession *session, const char *event, const char *origin, const char **params, unsigned int count);
void event_connect(IrcSession *session, const char *event, const char *origin, const char **params, unsigned int count);
void event_join(IrcSession *session, const char *event, const char *origin, const char **params, unsigned int count);
void event_part(IrcSession *session, const char *event, const char *origin, const char **params, unsigned int count);
void event_quit(IrcSession *session, const char *event, const char *origin, const char **params, unsigned int count);
void event_channel(IrcSession * session, const char * event, const char * origin, const char ** params, unsigned int count);
void event_privmsg(IrcSession *session, const char *event, const char *origin, const char **params, unsigned int count);
void event_notice(IrcSession *session, const char *event, const char *origin, const char **params, unsigned int count);
void event_topic(IrcSession *session, const char *event, const char *origin, const char **params, unsigned int count);
void event_names(IrcSession *session, const char *event, const char *origin, const char **params, unsigned int count);
void event_action(IrcSession *session, const char *event, const char *origin, const char **params, unsigned int count);
void event_kick(IrcSession *session, const char *event, const char *origin, const char **params, unsigned int count);
void event_whois(IrcSession *session, unsigned int event, const char *origin, const char **params, unsigned int count);
void event_invite(IrcSession *session, const char *event, const char *origin, const char **params, unsigned int count);
void event_ctcp(IrcSession *session, const char *event, const char *origin, const char **params, unsigned int count);

/* irc.c */
void irc_init(void);
void irc_join(IrcSession *s, const char *chan);

int irc_run_process(IrcSession *s, fd_set *inset);
void irc_disconnect(IrcSession *s);
int irc_connect(IrcSession *s,
          const char *server,
          const char *servername,
          unsigned short port,
          const char *password,
          const char *nick,
          const char *username,
          const char *realname);

int irc_send_raw(IrcSession *s, const char *format, ...);
void irc_parse_in(char *buf,
          const char *prefix,
          const char *command,
          const char **params,
          int *code,
          int *paramindex);

IrcSession* irc_session(void);
void irc_manage_event(IrcSession *session, int process_length);

/* input.c */
void input_manage(char *input);
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
void input_buffer(const char *input);
void input_buffer_list(const char *input);
void input_buffer_swap(const char *input);
void input_buffer_prev(const char *input);
void input_say(const char *input);
void input_reconnect(const char *input);
void input_nicklist_scroll(const char *input);
void input_nicklist_toggle(const char *input);
void input_color_theme(const char *input);

/* util.c */
void update_date(void);
ChanBuf *find_buf(IrcSession *s, const char *str);
ChanBuf *find_buf_wid(int id);
void msg_sessbuf(IrcSession *session, char *str);
int color_to_id(char *name);
char *colorstr(int color, char *str, ...);
char *nick_color(char *nick);
int hftirc_waddwch(WINDOW *w, unsigned int mask, wchar_t wch);
wchar_t *complete_nick(ChanBuf *cb, unsigned int hits, wchar_t *start, int *beg);
wchar_t *complete_input(ChanBuf *cb, unsigned int hits, wchar_t *start);

/* nick.c  */
void nick_attach(ChanBuf *cb, NickStruct *nick);
void nick_detach(ChanBuf *cb, NickStruct *nick);
NickStruct* nickstruct_set(char *nick);
void nick_sort_abc(ChanBuf *cb);

/* main.c */
void signal_handler(int signal);

/* Variables */
HFTIrc *hftirc;

#endif /* HFTIRC_H */
