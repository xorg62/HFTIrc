#include "hftirc.h"

void
irc_init(void)
{
     int i;
     irc_ctx_t ctx = { .id = 0 };

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
     hftirc->callbacks.event_invite      = irc_event_invite;
     hftirc->callbacks.event_umode       = irc_event_mode;
     hftirc->callbacks.event_ctcp_rep    = irc_dump_event;
     hftirc->callbacks.event_ctcp_action = irc_event_action;
     hftirc->callbacks.event_unknown     = irc_dump_event;
     hftirc->callbacks.event_numeric     = irc_event_numeric;

     hftirc->selses = 0;
     hftirc->session = calloc(hftirc->conf.nserv, sizeof(irc_session_t *));

     for(i = 0; i < hftirc->conf.nserv; ++i)
     {
          hftirc->session[i] = irc_create_session(&hftirc->callbacks);

          if(irc_connect(hftirc->session[i],
                         hftirc->conf.serv[i].adress,
                         hftirc->conf.serv[i].port,
                         hftirc->conf.serv[i].password,
                         hftirc->conf.serv[i].nick,
                         hftirc->conf.serv[i].username,
                         hftirc->conf.serv[i].realname))
               ui_print_buf(0, "Error: Can't connect to %s", hftirc->conf.serv[i].adress);

          ctx.id = (unsigned int)i;
          irc_set_ctx(hftirc->session[i], &ctx);
     }


     return;
}

void
irc_join(irc_session_t *session, const char *chan)
{
     int s;

     s = find_sessid(session);

     if(irc_cmd_join(session, chan, NULL))
     {
          WARN("Error", "Can't join this channel");
          return;
     }
     else
          ui_buf_new(chan, s);

     ui_buf_set(hftirc->nbuf - 1);

     return;
}

void
irc_nick(irc_session_t *session, const char *nick)
{
     int i;

     i = find_sessid(session);

     strcpy(hftirc->conf.serv[i].nick, nick);

     if(irc_cmd_nick(session, nick))
          WARN("Error", "Can't change nick or invalid nick");

     ui_print_buf(0, "  .:. Your nick is now %s on %s", nick, hftirc->conf.serv[find_sessid(session)].name);

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
          case 123456:
               for(i = 1; i < count; ++i)
               {
                    if(i)
                         strcat(buf, "|");

                    strcat(buf, params[i]);
               }

               ui_print_buf(0, "[%s] .:. %s", hftirc->conf.serv[find_sessid(session)].name, buf + 1);
               break;

          /* Whois */
          case 307:
          case 311:
          case 312:
          case 317:
          case 318:
          case 319:
               irc_event_whois(session, event, origin, params, count);
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
          case 412:
               ui_print_buf(0, "[%s] .:. %s", hftirc->conf.serv[find_sessid(session)].name, params[2]);
               break;
          case 482:
                ui_print_buf(hftirc->selbuf, "  .:. <%s> You're not channel operator", params[1]);
               break;
          case 470:
               ui_print_buf(0, "[%s] .:. %s %s %s", hftirc->conf.serv[find_sessid(session)].name, params[0], params[1], params[2]);
               break;

          /* Do nothing */
          case 376: /* End of MOTD, already managed by connect handle */
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

     if(strchr(origin, '!'))
          for(i = 0; origin[i] != '!'; nick[i] = origin[i], ++i);

     for(i = 0; i < hftirc->conf.nserv; ++i)
          if(session == hftirc->session[i])
               break;

     if(!strcmp(params[0], hftirc->conf.serv[i].nick))
               return;

     for(i = j = 0; i < hftirc->nbuf + 1; ++i)
          if(!strstr(nick, hftirc->cb[i].names)
                    || !strcmp(nick, hftirc->cb[i].name))
               c[j++] = i;

     for(i = 0; i < j; ++i)
          ui_print_buf(c[i], "  .:. %s is now %s", nick, params[0]);

     for(i = 0; i < hftirc->nbuf + 1; ++i)
          if(!strcmp(nick, hftirc->cb[i].name))
               strcpy(hftirc->cb[i].name, params[0]);

     return;
}

void
irc_event_mode(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int i;
     char nick[64] = { 0 };
     char nicks[BUFSIZE] = { 0 };

     /* User mode */
     if(count == 1)
     {
          ui_print_buf(0, "[%s] .:. User mode of %s : [%s]",
                    hftirc->conf.serv[find_sessid(session)].name, origin, params[0]);

          return;
     }

     if(strchr(origin, '!'))
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

     i = find_bufid(find_sessid(session), params[0]);

     ui_print_buf(i, "  .:. Mode %s [%s %s] set by %s", params[0], params[1], nicks + 1, nick);

     return;
}

void
irc_event_connect(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int c, i;

     c = find_sessid(session);

     irc_event_numeric(session, 123456, origin, params, count);

     for(i = 0; i < hftirc->conf.serv[c].nautojoin; ++i)
          irc_join(session, hftirc->conf.serv[c].autojoin[i]);

     return;
}

void
irc_event_join(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int i, j;
     char nick[64] = { 0 };

	irc_cmd_user_mode(session, "+i");

     i = find_bufid(find_sessid(session), params[0]);

     if(i == MAXBUF)
     {
          irc_dump_event(session, event, origin, params, count);

          return;
     }

     if(strchr(origin, '!'))
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

     i = find_bufid(find_sessid(session), params[0]);

     if(i == MAXBUF)
     {
          irc_dump_event(session, event, origin, params, count);

          return;
     }

     if(strchr(origin, '!'))
          for(j = 0; origin[j] != '!'; nick[j] = origin[j], ++j);

     ui_print_buf(i, "  <<<<- %s (%s) has left %s [%s]",
               nick, origin + strlen(nick) + 1, params[0], (params[1] ? params[1] : ""));

     return;
}

void
irc_event_quit(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int i, j, c[64], s;
     char nick[64] = { 0 };

     s = find_sessid(session);

     if(strchr(origin, '!'))
          for(j = 0; origin[j] != '!'; nick[j] = origin[j], ++j);

     for(i = j = 0; i < hftirc->nbuf + 1; ++i)
          if(hftirc->cb[i].sessid == s)
               if(strstr(hftirc->cb[i].names, nick)
                         || !strcmp(hftirc->cb[i].name, nick))
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

     i = find_bufid(find_sessid(session), params[0]);

     /* If the message is not from an old buffer, init a new one. */
     if(i > hftirc->nbuf)
     {
          ui_buf_new(params[0], i);
          i = hftirc->nbuf - 1;
     }

     if(strchr(origin, '!'))
          for(j = 0; origin[j] != '!'; nick[j] = origin[j], ++j);

     ui_print_buf(i, "<%s> %s", nick, params[1]);

     return;
}

void
irc_event_privmsg(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int i, j;
     char nick[64] = { 0 };

     if(strchr(origin, '!'))
          for(j = 0; origin[j] != '!'; nick[j] = origin[j], ++j);

     i = find_bufid(find_sessid(session), nick);

     /* If the message is not from an old buffer, init a new one. */
     if(i > hftirc->nbuf)
     {
          ui_buf_new(nick, find_sessid(session));
          strcpy(hftirc->cb[hftirc->nbuf - 1].names, nick);
          i = hftirc->nbuf - 1;
     }

     ui_print_buf(i, "<%s> %s", nick, params[1]);

     return;
}

void
irc_event_notice(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int j;
     char nick[64] = { 0 };

     if(strchr(origin, '!'))
          for(j = 0; origin[j] != '!'; nick[j] = origin[j], ++j);

     ui_print_buf(0, "[%s] .:. %s (%s)- %s",
               hftirc->conf.serv[find_sessid(session)].name,
               nick, origin + strlen(nick) + (strlen(nick) ? 1 : 0), params[1]);

     return;
}

void
irc_event_topic(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int i, j;
     char nick[64] = { 0 };

     i = find_bufid(find_sessid(session), params[((event[0] == '3') ? 1 : 0)]);

     if(!strcmp(event, "333"))
     {
          if(strchr(params[2], '!'))
               for(j = 0; params[2][j] != '!'; nick[j] = params[2][j], ++j);
          else
               strcpy(nick, params[2]);

          ui_print_buf(i, "  .:. Set by %s (%s)", nick, params[3]);
     }
     else if(!strcmp(event, "332"))
     {
          ui_print_buf(i, "  .:. Topic of %s: %s", params[1], params[2]);
          strcpy(hftirc->cb[i].topic, params[2]);
     }
     else
     {
          if(strchr(origin, '!'))
               for(j = 0; origin[j] != '!'; nick[j] = origin[j], ++j);

          ui_print_buf(i, "  .:. New topic of %s set by %s: %s", params[0], nick, params[1]);

          strcpy(hftirc->cb[i].topic, params[1]);
     }

     return;
}

void
irc_event_names(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int i;

     i = find_bufid(find_sessid(session), params[2]);

     ui_print_buf(i, "  .:. Users of %s:", params[2]);
     ui_print_buf(i, "-> [ %s]", params[3]);

     strcpy(hftirc->cb[i].names, params[3]);

     return;
}

void
irc_event_action(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int i;
     char nick[64] = { 0 };

     if(strchr(origin, '!'))
          for(i = 0; origin[i] != '!'; nick[i] = origin[i], ++i);

     i = find_bufid(find_sessid(session), params[0]);

     ui_print_buf(i, " * %s %s", nick, params[1]);

     return;
}

void
irc_event_kick(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int i;
     char ornick[64] = { 0 };

     if(strchr(origin, '!'))
          for(i = 0; origin[i] != '!'; ornick[i] = origin[i], ++i);

     i = find_bufid(find_sessid(session), params[0]);

     ui_print_buf(i, "  .:. %s kicked by %s from %s [%s]", params[1], ornick, params[0], params[2]);

     return;
}

void
irc_event_whois(irc_session_t *session, unsigned int event, const char *origin, const char **params, unsigned int count)
{
     switch(event)
     {
          case 307:
               ui_print_buf(0, "  .:.           %s: %s", params[1], params[2]);
               break;

          /* Whois user */
          case 311:
               ui_print_buf(0, "  .:. %s (%s@%s)", params[1], params[2], params[3]);
               ui_print_buf(0, "  .:. IRCNAME:  %s", params[5]);
               break;

          /* Whois server */
          case 312:
               ui_print_buf(0, "  .:. SERVER:   %s (%s)", params[2], params[3]);
               break;

          /* Whois idle */
          case 317:
               ui_print_buf(0, "  .:. IDLE:     seconds idle: %s signon time: %s", params[2], params[3]);
               break;

          /* End of whois */
          case 318:
               ui_print_buf(0, "  .:. %s", params[2]);
               break;

          /* Whois channel */
          case 319:
               ui_print_buf(0, "  .:. CHANNELS: %s", params[2]);
               break;

     }

     return;
}

void
irc_event_invite(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int i;
     char nick[64];

     if(strchr(origin, '!'))
          for(i = 0; origin[i] != '!'; nick[i] = origin[i], ++i);

     ui_print_buf(0, "[%s] .:. You've been invited by %s to %s",
               hftirc->conf.serv[find_sessid(session)].name, nick, params[1]);

     return;
}
