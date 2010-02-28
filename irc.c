#include "hftirc.h"

void
irc_init(void)
{
     memset(&hftirc->callbacks, 0, sizeof(hftirc->callbacks));

     hftirc->callbacks.event_connect     = irc_event_connect;
     hftirc->callbacks.event_join        = irc_event_join;
     hftirc->callbacks.event_nick        = irc_event_nick;
     hftirc->callbacks.event_quit        = irc_event_quit;
     hftirc->callbacks.event_part        = irc_event_part;
     hftirc->callbacks.event_mode        = irc_event_mode;
     hftirc->callbacks.event_topic       = irc_event_topic;
     hftirc->callbacks.event_kick        = irc_event_kick;
     hftirc->callbacks.event_channel     = irc_event_channel;
     hftirc->callbacks.event_privmsg     = irc_event_privmsg;
     hftirc->callbacks.event_notice      = irc_event_notice;
     hftirc->callbacks.event_invite      = irc_dump_event;
     hftirc->callbacks.event_umode       = irc_dump_event;
     hftirc->callbacks.event_ctcp_rep    = irc_dump_event;
     hftirc->callbacks.event_ctcp_action = irc_event_action;
     hftirc->callbacks.event_unknown     = irc_dump_event;
     hftirc->callbacks.event_numeric     = irc_event_numeric;

     hftirc->session = irc_create_session(&hftirc->callbacks);

     strcpy(hftirc->nick, IRC_NICK);

     if(irc_connect(hftirc->session,
                    IRC_SERV,
                    IRC_PORT,
                    0,
                    IRC_NICK,
                    IRC_USERNAME,
                    IRC_REALNAME))
          WARN("Error", "Can't connect");

     return;
}

void
irc_join(const char *chan)
{
     if(irc_cmd_join(hftirc->session, chan, NULL))
     {
          WARN("Error", "Can't join this channel");
          return;
     }

     ++hftirc->nbuf;
     strcpy(hftirc->cb[hftirc->nbuf - 1].name, chan);

     ui_print_buf(0, "Join %s", chan);

     return;
}

void
irc_nick(const char *nick)
{
     strcpy(hftirc->nick, nick);

     if(irc_cmd_nick(hftirc->session, nick))
          WARN("Error", "Can't change nick or invalid nick");

     ui_print_buf(0, "  .:. Your nick is now %s", nick);
     ui_print_buf(hftirc->selbuf, "  .:. Your nick is now %s", nick);

     return;
}

void
irc_dump_event(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     char buf[BUFSIZE] = { 0 };
     unsigned int i;

     for(i = 0; i < count; ++i)
     {
          if(i)
               strcat(buf, "|");

          strcat(buf, params[i]);
     }

     ui_print_buf(0, "(%s)-(%s)-|%d|-%s", event, (origin ? origin : "NULL"), i, buf);

     return;
}

void
irc_event_numeric(irc_session_t *session, unsigned int event, const char *origin, const char **params, unsigned int count)
{
     char num[24];
     char buf[BUFSIZE] = { 0 };
     int i;
     sprintf(num, "%d", event);

     switch(event)
     {
          /* Basic message, just write it in status buffer */
          case 1:
          case 2:
          case 3:
          case 4:
          case 5:
          case 251:
          case 252:
          case 254:
          case 255:
          case 265:
          case 266:
          case 372:
          case 375:
          case 376:
          case 123456:
               for(i = 1; i < count; ++i)
               {
                    if(i)
                         strcat(buf, "|");

                    strcat(buf, params[i]);
               }

               ui_print_buf(0, "[%s] .:. %s", origin, buf + 1);
               break;

          /* Topic */
          case 332:
          case 333:
               irc_event_topic(session, num, origin, params, count);
               break;
          /* Names */
          case 353:
               irc_event_names(session, num, origin, params, count);
               break;
          case 366:
               break;
          /* Errors */
          case 401:
          case 404:
               ui_print_buf(0, "[%s] .:. %s", origin, params[2]);
               break;
          case 482:
                ui_print_buf(hftirc->selbuf, "  .:. <%s> You're not channel operator", params[1]);
               break;
          default:
               irc_dump_event(session, num, origin, params, count);
               break;
     }

     return;
}

void
irc_event_nick(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int i, j, c[64];
     char nick[64] = { 0 };

     for(i = 0; origin[i] != '!'; nick[i] = origin[i], ++i);

     if(!strcmp(params[0], hftirc->nick))
               return;

     for(i = j = 0; i < hftirc->nbuf + 1; ++i)
          if(!strstr(nick, hftirc->cb[i].names))
               c[j++] = i;

     for(i = 0; i < j; ++i)
          ui_print_buf(c[i], "  .:. %s is now %s", nick, params[0]);

     return;
}

void
irc_event_mode(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int i;
     char nick[64] = { 0 };
     char nicks[BUFSIZE] = { 0 };

     for(i = 0; origin[i] != '!'; nick[i] = origin[i], ++i);

     for(i = 2; i < count; ++i)
     {
          if(i)
               strcat(nicks, "|");

          strcat(nicks, params[i]);
     }

     for(i = 0; nicks[i]; ++i)
          if(nicks[i] == '|')
               nicks[i] = ' ';

     i = find_bufid(params[0]);

     ui_print_buf(i, "  .:. Mode %s [%s %s] set by %s", params[0], params[1], nicks + 1, nick);

     return;
}

void
irc_event_connect(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     irc_event_numeric(session, 123456, origin, params, count);

     irc_join(IRC_CHAN);

     return;
}

void
irc_event_join(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int i, j;
     char nick[64] = { 0 };

	irc_cmd_user_mode(session, "+i");

     i = find_bufid(params[0]);

     if(i == MAXBUF)
     {
          irc_dump_event(session, event, origin, params, count);

          return;
     }

     for(j = 0; origin[j] != '!'; nick[j] = origin[j], ++j);

     ui_print_buf(i, "  ->>>> %s (%s) has joined %s", nick, origin + strlen(nick) + 1, params[0]);

     return;
}


void
irc_event_part(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int i, j;
     char nick[64] = { 0 };

	irc_cmd_user_mode(session, "+i");

     i = find_bufid(params[0]);

     if(i == MAXBUF)
     {
          irc_dump_event(session, event, origin, params, count);

          return;
     }

     for(j = 0; origin[j] != '!'; nick[j] = origin[j], ++j);

     ui_print_buf(i, "  <<<<- %s (%s) has left %s [%s]",
               nick, origin + strlen(nick) + 1, params[0], (params[1] ? params[1] : ""));

     return;
}

void
irc_event_quit(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int i, j, c[64];
     char nick[64] = { 0 };

	irc_cmd_user_mode(session, "+i");

     for(j = 0; origin[j] != '!'; nick[j] = origin[j], ++j);

     for(i = j = 0; i < hftirc->nbuf + 1; ++i)
          if(!strstr(nick, hftirc->cb[i].names))
               c[j++] = i;

     if(i == MAXBUF)
     {
          irc_dump_event(session, event, origin, params, count);

          return;
     }

     for(i = 0; i < j; ++i)
          ui_print_buf(c[i], "  <<<<- %s (%s) has quit [%s]", nick, origin + strlen(nick) + 1, params[0]);

     return;
}


void
irc_event_channel(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int i, j;
     char nick[64] = { 0 };

     i = find_bufid(params[0]);

     /* If the message is not from an old buffer, init a new one. */
     if(i > hftirc->nbuf)
     {
          ++hftirc->nbuf;
          strcpy(hftirc->cb[hftirc->nbuf - 1].name, params[0]);
          i = hftirc->nbuf - 1;
     }

     for(j = 0; origin[j] != '!'; nick[j] = origin[j], ++j);

     ui_print_buf(i, "<%s> %s", nick, params[1]);

     return;
}

void
irc_event_privmsg(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int i, j;
     char nick[64] = { 0 };

     for(j = 0; origin[j] != '!'; nick[j] = origin[j], ++j);

     i = find_bufid(nick);

     /* If the message is not from an old buffer, init a new one. */
     if(i > hftirc->nbuf)
     {
          ++hftirc->nbuf;
          strcpy(hftirc->cb[hftirc->nbuf - 1].name, nick);
          i = hftirc->nbuf - 1;
     }

     ui_print_buf(i, "<%s> %s", nick, params[1]);

     return;
}

void
irc_event_notice(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     ui_print_buf(0, "<%s> %s", origin, params[1]);

     return;
}

void
irc_event_topic(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int i;

     i = find_bufid(params[1]);

     if(!strcmp(event, "333"))
          ui_print_buf(i, "  .:. Set by %s (%s)", params[2], params[3]);
     else
     {
          ui_print_buf(i, "  .:. Topic of %s: %s", params[1], params[2]);
          strcpy(hftirc->cb[i].topic, params[2]);
     }

     return;
}

void
irc_event_names(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int i;

     i = find_bufid(params[2]);

     ui_print_buf(i, "  .:. Users of %s:", params[2]);
     ui_print_buf(i, "-> %s", params[3]);

     strcpy(hftirc->cb[i].names, params[3]);

     return;
}

void
irc_event_action(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int i;
     char nick[64] = { 0 };

     for(i = 0; origin[i] != '!'; nick[i] = origin[i], ++i);

     i = find_bufid(params[0]);

     ui_print_buf(i, " * %s %s", nick, params[1]);

     return;
}

void
irc_event_kick(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int i;
     char ornick[64] = { 0 };

     for(i = 0; origin[i] != '!'; ornick[i] = origin[i], ++i);

     i = find_bufid(params[0]);

     ui_print_buf(i, "  .:. %s kicked by %s from %s [%s]", params[1], ornick, params[0], params[2]);

     return;
}
