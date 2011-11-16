/*
 * copyright (c) 2010 Martin Duquesnoy <xorg62@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef UI_H
#define UI_H

#include "hftirc.h"

/* Will be configurable */
#define ROSTERSIZE    20

/* Test control-bind */
#define IS_CTRLK(c)  (c > 0 && c < 32)

/* Colors lists */
#define COLOR_THEME_DEFAULT  COLOR_BLUE
#define COLOR_SW      (ui_color(COLOR_BLACK, hftirc.ui.tcolor))
#define COLOR_SW2     (ui_color(COLOR_WHITE, hftirc.ui.tcolor))
#define COLOR_ROSTER  (ui_color(hftirc.ui.tcolor, hftirc.ui.bg))
#define COLOR_ACT     (ui_color(COLOR_WHITE,  hftirc.ui.tcolor))
#define COLOR_HLACT   (ui_color(COLOR_RED, hftirc.ui.tcolor) | A_BOLD)
#define COLOR_LASTPOS (ui_color(COLOR_BLUE, hftirc.ui.bg | A_BOLD ))

/* HFTIrc colors:
 * Based on ANSI colors list
 * See: http://en.wikipedia.org/wiki/ANSI_escape_code#Colors
 *
 * 0  black    8   grey
 * 1  red      9   lightred
 * 2  green    10  lightgreen
 * 3  yellow   11  lightyellow
 * 4  blue     12  lightblue
 * 5  magenta  13  lightmagenta
 * 6  cyan     14  lightcyan
 * 7  white    15  lightwhite
 *
 */
typedef enum
{
     Black,      Red,          Green,     Yellow,     Blue,       Magenta,
     Cyan,       White,        Grey,      LightRed,   LightGreen, LightYellow,
     LightBlue,  LightMagenta, LightCyan, LightWhite, LastCol
} HFTIrcColor;

static const struct { int c, m; char *name; } hftirccol[LastCol] =
{
     { COLOR_BLACK,   A_NORMAL, "black" },        /* 0 */
     { COLOR_RED,     A_NORMAL, "red" },          /* 1 */
     { COLOR_GREEN,   A_NORMAL, "green" },        /* 2 */
     { COLOR_YELLOW,  A_NORMAL, "yellow" },       /* 3 */
     { COLOR_BLUE,    A_NORMAL, "blue" },         /* 4 */
     { COLOR_MAGENTA, A_NORMAL, "magenta" },      /* 5 */
     { COLOR_CYAN,    A_NORMAL, "cyan" },         /* 6 */
     { COLOR_WHITE,   A_NORMAL, "white" },        /* 7 */
     { COLOR_BLACK,   A_BOLD,   "grey" },         /* 8 */
     { COLOR_RED,     A_BOLD,   "lightred" },     /* 9 */
     { COLOR_GREEN,   A_BOLD,   "lightgreen" },   /* 10 */
     { COLOR_YELLOW,  A_BOLD,   "lightyellow" },  /* 11 */
     { COLOR_BLUE,    A_BOLD,   "lightblue" },    /* 12 */
     { COLOR_MAGENTA, A_BOLD,   "lightmagenta" }, /* 13 */
     { COLOR_CYAN,    A_BOLD,   "lightcyan" },    /* 14 */
     { COLOR_WHITE,   A_BOLD,   "lightwhite" }    /* 15 */
};

/* mIRC colors:
 * From http://www.mirc.net/newbie/colors.php
 *
 *  0  white	 8   yellow
 *  1  black	 9   lightgreen
 *  2  blue	 10  cyan
 *  3  green    11  lightcyan
 *  4  lightred 12  lightblue
 *  5  brown    13  pink
 *  6  purple   14  grey
 *  7  orange   15  lightg
 *
 */
static const struct { int c, m; } mirccol[] =
{
     { COLOR_WHITE,   A_BOLD   },
     { COLOR_BLACK,   A_NORMAL },
     { COLOR_BLUE,    A_NORMAL },
     { COLOR_GREEN,   A_NORMAL },
     { COLOR_RED,     A_BOLD   },
     { COLOR_RED,     A_NORMAL },
     { COLOR_MAGENTA, A_NORMAL },
     { COLOR_YELLOW,  A_NORMAL },
     { COLOR_YELLOW,  A_BOLD   },
     { COLOR_GREEN,   A_BOLD   },
     { COLOR_CYAN,    A_NORMAL },
     { COLOR_CYAN,    A_BOLD   },
     { COLOR_BLUE,    A_BOLD   },
     { COLOR_MAGENTA, A_BOLD   },
     { COLOR_BLACK,   A_BOLD   },
     { COLOR_WHITE,   A_NORMAL },
};

#endif /* UI_H */
