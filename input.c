#include "hftirc.h"

const InputStruct input_struct[] =
{
     { "join", input_join },
     { "nick", input_nick },
     { "quit", input_quit },
     { "part", input_part },
     { "names", input_names },
     { "topic", input_topic },
     { "help", input_help },
};

void
input_manage(const char *input)
{
     int i;

     if(input[0] == '/')
     {
          ++input;
          for(i = 0; i < LEN(input_struct); ++i)
               if(!strncmp(input, input_struct[i].cmd, strlen(input_struct[i].cmd)))
                    input_struct[i].func(input + strlen(input_struct[i].cmd));
     }
     else
     {
          irc_cmd_msg(hftirc->session, hftirc->cb[hftirc->selbuf].name, input);
          ui_print_buf(hftirc->selbuf, "<%s> %s", hftirc->nick, input);
     }

     return;
}

void
input_join(const char *input)
{
     for(; input[0] == ' '; ++input);

     if(input[0] != '#')
     {
          WARN("Error", "Usage: /join #<channel>");

          return;
     }

     irc_join(input);

    return;
}

void
input_nick(const char *input)
{
     for(; input[0] == ' '; ++input);

     irc_nick(input);

     return;
}

void
input_quit(const char *input)
{
     hftirc->running = 0;

     irc_cmd_quit(hftirc->session, input);

     return;
}

void
input_names(const char *input)
{
     irc_cmd_names(hftirc->session, hftirc->cb[hftirc->selbuf].name);

     return;
}

void
input_help(const char *input)
{
     ui_print_buf(0, "Help: ...");

     return;
}

void
input_topic(const char *input)
{
     for(; input[0] == ' '; ++input);

     if(strlen(input) > 0)
          irc_cmd_topic(hftirc->session, hftirc->cb[hftirc->selbuf].name, input);
     else
          ui_print_buf(hftirc->selbuf, "  .:. Topic of %s: %s",
                    hftirc->cb[hftirc->selbuf].name,
                    hftirc->cb[hftirc->selbuf].topic);
     return;
}

void
input_part(const char *input)
{
     irc_cmd_part(hftirc->session, hftirc->cb[hftirc->selbuf].name);

     return;
}
