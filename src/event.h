/*
 * hftirc2 by Martin Duquesnoy <xorg62@gmail.com>
 * For license, see COPYING
 */

#ifndef EVENT_H
#define EVENT_H

#include "hftirc.h"

void event_dump(struct session*, int, const char*, const char**, int);
void event_ping(struct session*, int, const char*, const char**, int);
void event_join(struct session*, int, const char*, const char**, int);

#endif /* EVENT_H */
