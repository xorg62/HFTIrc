/*
 * hftirc2 by Martin Duquesnoy <xorg62@gmail.com>
 * For license, see COPYING
 */

#ifndef EVENT_H
#define EVENT_H

#include "hftirc.h"

void event_ping(struct session *, int, const char*, const char**, int);

#endif /* EVENT_H */
