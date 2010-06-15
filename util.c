#include "hftirc.h"

void
update_date(void)
{
     hftirc->date.tm = localtime(&hftirc->date.lt);
     hftirc->date.lt = time(NULL);

     strftime(hftirc->date.str, sizeof(hftirc->date.str), "%m-%d %H:%M:%S", hftirc->date.tm);

     return;
}

int
find_bufid(unsigned int id, const char *str)
{
     int i;

     if(!str)
          return 0;

     for(i = 0; i < hftirc->nbuf; ++i)
          if(hftirc->cb[i].name != NULL
            && strlen(hftirc->cb[i].name) > 1)
               if(!strcmp(str, hftirc->cb[i].name)
                 && hftirc->cb[i].sessid == id)
                    return i;

     return 0;
}

int
find_sessid(irc_session_t *session)
{
     int i;

     if(!session)
          return 0;

     for(i = 0; i < hftirc->conf.nserv; ++i)
          if(session == hftirc->session[i])
               break;

     return i;
}


