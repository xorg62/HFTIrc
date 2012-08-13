/*
 * hftirc2 by Martin Duquesnoy <xorg62@gmail.com>
 * For license, see COPYING
 */

#ifndef IRC_H
#define IRC_H

#include "hftirc.h"

ssize_t irc_send_raw(struct session *s, const char *format, ...);
int irc_connect(struct session *s);
void irc_disconnect(struct session *s);

#endif /* IRC_H */
