/*
 * hftirc2 by Martin Duquesnoy <xorg62@gmail.com>
 * For license, see COPYING
 */

#include "hftirc.h"
#include "util.h"

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
     char *ret = NULL;

     if(str == NULL || (ret = strdup(str)) == NULL)
          warnx("strdup(%s)", str);

     return ret;
}
