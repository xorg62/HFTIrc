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

#include "hftirc.h"
#include "config.h"

#define SSTRCPY(dest, src) if(src) strcpy((dest), (src))

static void
config_misc(void)
{
     struct conf_sec *misc;

     misc = fetch_section_first(NULL, "misc");

     hftirc->conf.bell   = fetch_opt_first(misc, "false", "bell").boolp;
     hftirc->conf.nicklist = fetch_opt_first(misc, "false", "nicklist_enable").boolp;
     hftirc->conf.lastlinepos = fetch_opt_first(misc, "false", "lastline_position").boolp;

     free(misc);

     return;
}

static void
config_ui(void)
{
     struct conf_sec *ui, *colors;

     ui = fetch_section_first(NULL, "ui");

     hftirc->conf.nickcolor = fetch_opt_first(ui, "false", "nick_color_enable").boolp;

     /* Colors section */
     colors = fetch_section_first(ui, "colors");
     hftirc->conf.tcolor = color_to_id(fetch_opt_first(colors, "blue", "color_theme").str);

     free(ui);
     free(colors);

     return;
}

static void
config_server(void)
{
     int i, j, n = 0;
     struct conf_sec **serv;
     struct opt_type *opt;
     ServInfo defsi = { "Hft", "irc.freenode.net", "", 6667, "hftircuser", " ", "HFTIrcuser", "HFTIrcuser"};

     if(!(serv = fetch_section(fetch_section_first(NULL, "servers"), "server"))
               || !(hftirc->conf.nserv = fetch_section_count(serv)))
     {
          hftirc->conf.serv = malloc(sizeof(ServInfo));
          hftirc->conf.serv[0] = defsi;
          hftirc->conf.nserv = 1;

          return;
     }

     hftirc->conf.serv = malloc(sizeof(ServInfo) * hftirc->conf.nserv);

     for(i = 0; i < hftirc->conf.nserv; ++i)
     {

          SSTRCPY(hftirc->conf.serv[i].adress,   fetch_opt_first(serv[i], "irc.hft-community.org", "adress").str);
          SSTRCPY(hftirc->conf.serv[i].name,     fetch_opt_first(serv[i], hftirc->conf.serv[i].adress, "name").str);
          SSTRCPY(hftirc->conf.serv[i].password, fetch_opt_first(serv[i], "", "password").str);
          SSTRCPY(hftirc->conf.serv[i].nick,     fetch_opt_first(serv[i], "hftircuser", "nickname").str);
          SSTRCPY(hftirc->conf.serv[i].username, fetch_opt_first(serv[i], "hftircuser", "username").str);
          SSTRCPY(hftirc->conf.serv[i].realname, fetch_opt_first(serv[i], "hftircuser", "realname").str);
          hftirc->conf.serv[i].port = fetch_opt_first(serv[i], "6667", "port").num;
          hftirc->conf.serv[i].ipv6 = fetch_opt_first(serv[i], "false", "ipv6").boolp;

          opt = fetch_opt(serv[i], "", "channel_autojoin");

          if((n = fetch_opt_count(opt)))
          {
               if((hftirc->conf.serv[i].nautojoin = n) > 127)
                    ui_print_buf(0, "HFTIrc configuration: section serv (%d), too many channel_autojoin (%d).", i, n);
               else
                    for(j = 0; j < n; ++j)
                         SSTRCPY(hftirc->conf.serv[i].autojoin[j], opt[j].str);
          }

          free(opt);
     }

     free(serv);

     return;
}

void
config_parse(void)
{
     if(get_conf(hftirc->conf.path) == -1)
     {
          ui_print_buf(0, "parsing configuration file (%s) failed.", hftirc->conf.path);
          sprintf(hftirc->conf.path, "%s/hftirc/hftirc.conf", XDG_CONFIG_DIR);
          get_conf(hftirc->conf.path);
     }

     config_misc();
     config_ui();
     config_server();

     return;
}
