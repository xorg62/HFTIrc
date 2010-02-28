#include "hftirc.h"

void
update_date(void)
{
     hftirc->date.tm = localtime(&hftirc->date.lt);
     hftirc->date.lt = time(NULL);

     strftime(hftirc->date.str, sizeof(hftirc->date.str), "[%r]", hftirc->date.tm);

     return;
}

int
find_bufid(const char *str)
{
     int i;

     for(i = 0; i < hftirc->nbuf + 1; ++i)
          if(!strcmp(str, hftirc->cb[i].name))
               return i;

     return hftirc->nbuf + 1;
}



