/*
 * hftirc2 by Martin Duquesnoy <xorg62@gmail.com>
 * For license, see COPYING
 */

#ifndef UTIL_H
#define UTIL_H

#include "ui.h"

#define LEN(x) (sizeof(x) / sizeof(*x))
#define REMOVE_SPACE(s)                         \
     while(*(s) && *(s) == ' ')                 \
          ++(s)
#define ISACHAN(s) (s[0] == '#' || s[0] == '&')

void* xmalloc(size_t nmemb, size_t size);
void *xcalloc(size_t nmemb, size_t size);
int xasprintf(char **strp, const char *fmt, ...);
char *xstrdup(const char *str);
void hftirc_waddwch(WINDOW *w, unsigned int mask, wchar_t wch);

static const struct { char name[10]; int id; } colordef[] =
{
     { "black",   COLOR_BLACK },
     { "red",     COLOR_RED },
     { "green",   COLOR_GREEN },
     { "yellow",  COLOR_YELLOW },
     { "blue",    COLOR_BLUE },
     { "magenta", COLOR_MAGENTA },
     { "cyan",    COLOR_CYAN },
     { "white",   COLOR_WHITE },
     { "", -1 }
};

static inline int
color_to_id(char *name)
{
     int i = 0;

     while(colordef[i++].id > -1)
          if(!strcasecmp(colordef[i].name, name))
               return colordef[i].id;

     return COLOR_THEME_DEFAULT;
}

#endif
