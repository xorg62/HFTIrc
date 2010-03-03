#ifndef HFTIRC_H
#define HFTIRC_H

/* Libs */
#include <ncurses.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <locale.h>

#include <libircclient.h>
#include <libirc_events.h>

#include "config.h"

/* Macro */
#define HFTIRC_VERSION    "0.01"
#define BUFSIZE           4096
#define MAXBUF            64
#define BUFLINES          512
#define HFTIRC_KEY_ENTER  10
#define MAINWIN_LINES     LINES - 1
#define CONFPATH          "hftirc.conf"

#define CTRL(c) ((c) & 037)
#define LEN(x) (sizeof(x)/sizeof(x[0]))
#define WARN(t,s) ui_print_buf(0, "%s: %s", t, s)
#define DSINPUT(i) for(; i[0] == ' '; ++i)
#define PRINTATTR(w, attr, s)  wattron(w, attr); waddstr(w, s); wattroff(w, attr);

/* Structures */
typedef struct
{
     /* Ncurses windows */
     WINDOW *mainwin;
     WINDOW *inputwin;
     WINDOW *statuswin;

     /* Input buffer struct */
     struct
     {
          wchar_t buffer[BUFSIZE];
          int pos;
          int cpos;
          int split;
          int spting;
     } ib;
} Ui;

/* Channel buffer */
typedef struct
{
     /* For ui use */
     char buffer[BUFLINES][BUFSIZE];
     int bufpos;

     /* For irc info */
     unsigned int sessid;
     char name[128];
     char names[BUFSIZE];
     char topic[BUFSIZE];
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
     char name[256];
     char adress[256];
     char password[128];
     int port;
     char nick[128];
     char username[256];
     char realname[256];
     char autojoin[128][128];
     int nautojoin;
} ServInfo;

/* Config struct */
typedef struct
{
     char path[512];
     char datef[256];
     int nserv;
     ServInfo *serv;

} ConfStruct;

/* Control char to char attribute */
typedef struct
{
     char c;
     unsigned int a;
} CcharToAttr;

/* Global struct */
typedef struct
{
     int ft, running;
     int nbuf, selbuf, selses;
     irc_session_t **session;
     irc_callbacks_t callbacks;
     ConfStruct conf;
     ChanBuf cb[MAXBUF];
     Ui *ui;
     DateStruct date;
} HFTIrc;

/* Ctx struct */
typedef struct
{
     unsigned int id;
} irc_ctx_t;

/* Prototypes */

/* config.c */
void config_parse(char *file);

/* ui.c */
void ui_init(void);
void ui_update_statuswin(void);
void ui_update_infowin(void);
void ui_print(WINDOW *w, char *str);

void ui_print_buf(int id, char *format, ...);
void ui_draw_buf(int id);
void ui_buf_new(const char *name, unsigned int id);
void ui_buf_close(int buf);
void ui_buf_set(int buf);
void ui_get_input(void);

/* irc.c */
void irc_init(void);
void irc_join(irc_session_t *session, const char *chan);
void irc_nick(irc_session_t *session, const char *nick);

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
void input_serv(const char *input);

/* util.c */
void update_date(void);
int find_bufid(unsigned id, const char *str);
int find_sessid(irc_session_t *session);

/* main.c */
void signal_handler(int signal);

/* Variables */
HFTIrc *hftirc;

#endif /* HFTIRC_H */
