/*
 * hftirc2 by Martin Duquesnoy <xorg62@gmail.com>
 * For license, see COPYING
 */

#include <stdint.h>

#include "hftirc.h"
#include "util.h"

void*
xmalloc(size_t nmemb, size_t size)
{
     void *ret;

     if(SIZE_MAX / nmemb < size)
          err(EXIT_FAILURE, "xmalloc(%zu, %zu), "
               "size_t overflow detected", nmemb, size);

     if((ret = malloc(nmemb * size)) == NULL)
          err(EXIT_FAILURE, "malloc(%zu)", nmemb * size);

     return ret;
}

void*
xcalloc(size_t nmemb, size_t size)
{
     void *ret;

     if((ret = calloc(nmemb, size)) == NULL)
          err(EXIT_FAILURE, "calloc(%zu * %zu)", nmemb, size);

     return ret;
}

int
xasprintf(char **strp, const char *fmt, ...)
{
     int ret;
     va_list args;

     va_start(args, fmt);
     ret = vasprintf(strp, fmt, args);
     va_end(args);

     if (ret == -1)
          err(EXIT_FAILURE, "asprintf(%s)", fmt);

     return ret;
}

char *
xstrdup(const char *str)
{
     char *ret;

     if(str)
          return strdup(str);

     return NULL;
}

void
hftirc_waddwch(WINDOW *w, unsigned int mask, wchar_t wch)
{
     cchar_t cch;
     wchar_t wstr[2] = { wch };

     wattron(w, mask);

     if(setcchar(&cch, wstr, A_NORMAL, 0, NULL) == OK)
          (void)wadd_wch(w, &cch);

     wattroff(w, mask);
}
