/*
 * hftirc2 by Martin Duquesnoy <xorg62@gmail.com>
 * For license, see COPYING
 */

#ifndef UTIL_H
#define UTIL_H

#define REMOVE_SPACE(s) \
     while(*(s) == ' ') \
          ++(s)

void *xcalloc(size_t nmemb, size_t size);
int xasprintf(char **strp, const char *fmt, ...);
char *xstrdup(const char *str);
void hftirc_waddwch(WINDOW *w, unsigned int mask, wchar_t wch);

#endif
