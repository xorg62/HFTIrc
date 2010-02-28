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
#include <pthread.h>

#include <libircclient.h>
#include <libirc_events.h>

#include "config.h"

/* Macro */
#define HFTIRC_VERSION    "0.01"
#define BUFSIZE           4096
#define MAXBUF            64
#define BUFLINES          256
#define HFTIRC_KEY_ENTER  10
#define MAINWIN_LINES     LINES - 1

#define LEN(x) (sizeof(x)/sizeof(x[0]))
#define WARN(t,s) ui_print_buf(0, "%s: %s", t, s)

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

/* Global struct */
typedef struct
{
     int ft, running;
     int nbuf, selbuf;
     char nick[64];
     irc_session_t *session;
     irc_callbacks_t callbacks;
     ChanBuf cb[MAXBUF];
     Ui *ui;
     DateStruct date;
} HFTIrc;

/* Prototypes */

/* ui.c */
void ui_init(void);
void ui_update_statuswin(void);
void ui_update_infowin(void);
void ui_print_buf(int id, char *format, ...);
void ui_draw_buf(int id);
void ui_buf_set(int buf);
void ui_get_input(void);

/* irc.c */
void irc_init(void);
void irc_join(const char *chan);
void irc_nick(const char *nick);

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
void input_kick(const char *input);

/* util.c */
void update_date(void);
int find_bufid(const char *str);


/* main.c */
void signal_handler(int signal);

/* Variables */
HFTIrc *hftirc;

#endif /* HFTIRC_H */
