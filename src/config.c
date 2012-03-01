/*
 * hftirc2 by Martin Duquesnoy <xorg62@gmail.com>
 * For license, see COPYING
 */

#include "hftirc.h"
#include "config.h"
#include "parse.h"
#include "ui.h"
#include "util.h"

#define SSTRCPY(dest, src) if(src) strcpy((dest), (src))

static const struct { Flags f; char name[16]; } ignorebli[] =
{
     { IGNORE_JOIN,   "join"        },
     { IGNORE_QUIT,   "quit"        },
     { IGNORE_MODE,   "mode"        },
     { IGNORE_CTCP,   "ctcp"        },
     { IGNORE_NOTICE, "notice"      },
     { IGNORE_PART,   "part"        },
     { IGNORE_NICK,   "nick_change" },
     { 0, "" }
};

static void
config_misc(void)
{
     struct conf_sec *misc;

     misc = fetch_section_first(NULL, "misc");

     SSTRCPY(H.dateformat, fetch_opt_first(misc, "%m-%d %H:%M:%S", "date_format").str);

     if(fetch_opt_first(misc, "false", "bell").boolean)
          H.flags |= HFTIRC_BELL;

     if(fetch_opt_first(misc, "false", "nicklist_enable").boolean)
          H.flags |= HFTIRC_NICKLIST;

     if(fetch_opt_first(misc, "false", "lastline_position").boolean)
          H.flags |= HFTIRC_LASTLINEPOS;
}

static void
config_ignore(void)
{
     int i = -1;
     struct conf_sec *ignore;

     H.ignore_flags = 0;

     if((ignore = fetch_section_first(NULL, "ignore")))
     {
          while(ignorebli[i++].f)
               if(fetch_opt_first(ignore, "false", (char *)ignorebli[i].name).boolean)
                    H.ignore_flags |= ignorebli[i].f;
     }
}

static void
config_ui(void)
{
     struct conf_sec *ui, *colors;

     ui = fetch_section_first(NULL, "ui");

     if(fetch_opt_first(ui, "false", "nick_color_enable").boolean)
          H.flags |= HFTIRC_NICKCOLOR;

     /* Colors section */
     colors = fetch_section_first(ui, "colors");
     H.ui.tcolor = color_to_id(fetch_opt_first(colors, "blue", "color_theme").str);
}

static void
config_server(void)
{
     int i, j, nserv;
     struct conf_sec **serv;
     struct opt_type *opt;
     struct session_info *in;
     struct session_info defsi =
          {
               .name = "Hft",
               .server = "irc.hftc.fr",
               .password = "",
               .port = 6667,
               .nick = "hftircuser",
               .username = "HFTIrcuser",
               .realname = "HFTIrcuser",
               .autojoin = NULL,
               .nautojoin = 0
          };

     if(!(serv = fetch_section(fetch_section_first(NULL, "servers"), "server"))
        || !(nserv = fetch_section_count(serv)))
     {
          SLIST_INSERT_HEAD(&H.h.session_info, &defsi, next);
          return;
     }

     for(i = 0; i < nserv; ++i)
     {
          in = xcalloc(1, sizeof(struct session_info));

          in->server   = xstrdup(fetch_opt_first(serv[i], "irc.hftc.fr", "adress").str);
          in->name     = xstrdup(fetch_opt_first(serv[i], in->server, "adress").str);
          in->password = xstrdup(fetch_opt_first(serv[i], "", "password").str);
          in->nick     = xstrdup(fetch_opt_first(serv[i], "hftircuser", "nickname").str);
          in->username = xstrdup(fetch_opt_first(serv[i], "hftircuser", "username").str);
          in->realname = xstrdup(fetch_opt_first(serv[i], "hftircuser", "realname").str);
          in->port     = fetch_opt_first(serv[i], "6667", "port").num;

          opt = fetch_opt(serv[i], "", "channel_autojoin");

          if((in->nautojoin = fetch_opt_count(opt)))
          {
               in->autojoin = xcalloc(in->nautojoin + 1, sizeof(char*));
               for(j = 0; j < in->nautojoin; ++j)
                    in->autojoin[j] = xstrdup(opt[j].str);
          }

          SLIST_INSERT_HEAD(&H.h.session_info, in, next);
     }
}

void
config_init(void)
{
     if(get_conf(H.confpath) == -1)
     {
          warnx("parsing configuration file (%s) failed.", H.confpath);
          sprintf(H.confpath, "%s/hftirc/hftirc.conf", /*XDG_CONFIG_DIR*/"/etc/xdg");

          if(get_conf(H.confpath) == -1)
               errx(1, "parsing default configuration file (%s) failed.", H.confpath);
     }

     config_misc();
     config_ui();
     config_ignore();
     config_server();

     free_conf();
}

