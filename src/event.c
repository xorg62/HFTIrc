/*
 * hftirc2 by Martin Duquesnoy <xorg62@gmail.com>
 * For license, see COPYING
 */

#include "hftirc.h"
#include "ui.h"
#include "irc.h"
#include "event.h"
#include "util.h"

void
event_ping(struct session *s, int code, const char *prefix, const char **params, int paramindex)
{
     irc_send_raw(s, "PONG %s", params[0]);
}
