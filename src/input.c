/*
 * hftirc2 by Martin Duquesnoy <xorg62@gmail.com>
 * For license, see COPYING
 */

#include "hftirc.h"
#include "input.h"
#include "util.h"
#include "ui.h"

static void
input_say(const char *input)
{
     ui_print_buf(H.bufsel, "%s", input);
}

static void
input_quit(const char *input)
{
     H.flags ^= HFTIRC_RUNNING;
}

/*
 * Input management: Index array + associativ loop
 */
static const struct
{
     char *cmd;
     int len;
     void (*func)(const char *arg);
} input_list[5] =
{
     { "quit", 4, input_quit },
     { "say",  3, input_say },
     { "/",    1, input_say },
     { NULL, 0, NULL },
};

void
input_manage(char *input)
{
     int i = 0;

     if(*input == '/')
     {
          ++input;

          REMOVE_SPACE(input);

          for(; input_list[i].cmd; ++i)
               if(!strncmp(input, input_list[i].cmd, input_list[i].len))
               {
                    input += input_list[i].len;
                    REMOVE_SPACE(input);
                    input_list[i].func(input);
                    break;
               }
     }
     else
          input_say(input);
}

