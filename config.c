#include "hftirc.h"
#include "confparse/confparse.h"

#define SSTRCPY(dest, src) if(src) strcpy((dest), (src))

void
config_server(char *src)
{
     int i, j, n = 0;
     char *tmp;
     opt_type *buf;
     ServInfo defsi = { "Hft", "irc.hft-community", "", 6667, "hftircuser", " ", "HFTIrcuser", "HFTIrcuser"};

     cfg_set_sauv(src);

     hftirc->conf.nserv = get_size_sec(src, "server");

     if(!hftirc->conf.nserv)
     {
          hftirc->conf.serv[0] = defsi;

          return;
     }

     if(hftirc->conf.nserv > NSERV)
     {
          ui_print_buf(0, "HFTIrc configuratin: Too much servs (limit: %d)", NSERV);
          hftirc->conf.nserv = NSERV;
     }

     for(i = 0; i < hftirc->conf.nserv; ++i)
     {
          tmp = get_nsec(src, "server", i);

          cfg_set_sauv(tmp);

          SSTRCPY(hftirc->conf.serv[i].adress,   get_opt(tmp, "irc.hft-community.org", "adress").str);
          SSTRCPY(hftirc->conf.serv[i].name,     get_opt(tmp, hftirc->conf.serv[i].adress, "name").str);
          SSTRCPY(hftirc->conf.serv[i].password, get_opt(tmp, "", "password").str);
          SSTRCPY(hftirc->conf.serv[i].nick,     get_opt(tmp, "hftircuser", "nickname").str);
          SSTRCPY(hftirc->conf.serv[i].username, get_opt(tmp, "", "username").str);
          SSTRCPY(hftirc->conf.serv[i].realname, get_opt(tmp, "", "realname").str);
          hftirc->conf.serv[i].port = get_opt(tmp, "6667", "port").num;

          buf = get_list_opt(tmp, "", "channel_autojoin", &n);

          if((hftirc->conf.serv[i].nautojoin = n) > 127)
               ui_print_buf(0, "HFTIrc configuration: section serv (%d), too many channel_autojoin (%d).", i, n);
          else
               for(j = 0; j < n; ++j)
                    SSTRCPY(hftirc->conf.serv[i].autojoin[j], buf[j].str);

          cfg_set_sauv(src);
     }

     return;
}

void
config_parse(char *file)
{
     char *buf;

     if(!(buf = file_to_str(file)))
     {
          ui_print_buf(0, "HFTIrc configuration: parsing configuration file (%s) failed.", hftirc->conf.path);
          buf = file_to_str(CONFPATH);
     }

     cfg_set_sauv(buf);

     config_server(buf);

     free(buf);

     return;
}
