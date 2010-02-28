#include "hftirc.h"

const InputStruct input_struct[] =
{
     { "join",  input_join },
     { "nick",  input_nick },
     { "quit",  input_quit },
     { "part",  input_part },
     { "kick",  input_kick },
     { "names", input_names },
     { "topic", input_topic },
     { "query", input_query },
     { "me",    input_me },
     { "whois", input_whois },
     { "help",  input_help },
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
          if(irc_cmd_msg(hftirc->session, hftirc->cb[hftirc->selbuf].name, input))
               WARN("Error", "Can't send message");
          else
               ui_print_buf(hftirc->selbuf, "<%s> %s", hftirc->nick, input);
     }

     return;
}

void
input_join(const char *input)
{
     DSINPUT(input);

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

     irc_nick(input);

     return;
}

void
input_quit(const char *input)
{
     DSINPUT(input);

     hftirc->running = 0;

     irc_cmd_quit(hftirc->session, input);

     return;
}

void
input_names(const char *input)
{
     if(irc_cmd_names(hftirc->session, hftirc->cb[hftirc->selbuf].name))
          WARN("Error", "Can't get names list");

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
     DSINPUT(input);

     if(strlen(input) > 0)
     {
          if(irc_cmd_topic(hftirc->session, hftirc->cb[hftirc->selbuf].name, input))
               WARN("Error", "Can't change topic");
     }
     else
          ui_print_buf(hftirc->selbuf, "  .:. Topic of %s: %s",
                    hftirc->cb[hftirc->selbuf].name,
                    hftirc->cb[hftirc->selbuf].topic);
     return;
}

void
input_part(const char *input)
{
     if(irc_cmd_part(hftirc->session, hftirc->cb[hftirc->selbuf].name))
          WARN("Error", "While using PART command");

     return;
}

void
input_me(const char *input)
{
     DSINPUT(input);

     if(irc_cmd_me(hftirc->session, hftirc->cb[hftirc->selbuf].name, input))
          WARN("Error", "Can't send action message");
     else
          ui_print_buf(hftirc->selbuf, " * %s %s", hftirc->nick, input);

     return;
}

void
input_kick(const char *input)
{
     int i;
     char nick[64] = { 0 };
     char reason[BUFSIZE] = { 0 };

     DSINPUT(input);


     if(strlen(input) > 0)
     {
          for(i = 0; input[i] != ' ' && input[i]; nick[i] = input[i], ++i);

          if(input[i] == ' ')
          {
               input += strlen(nick);
               DSINPUT(input);
               for(i = 0; input[i]; reason[i] = input[i], ++i);
          }
     }
     else
     {
          WARN("Error", "Usage: /kick <nick> <reason(optional)>");

          return;
     }

     if(irc_cmd_kick(hftirc->session, nick, hftirc->cb[hftirc->selbuf].name, reason))
          WARN("Error", "Can't kick");

     return;
}

void
input_whois(const char *input)
{
     DSINPUT(input);

     if(strlen(input) > 0)
     {
          if(irc_cmd_whois(hftirc->session, input))
               WARN("Error", "Can't use WHOIS");
     }
     else
          WARN("Error", "Usage: /whois <nick>");

     return;
}

void
input_query(const char *input)
{
     DSINPUT(input);

     if(strlen(input) > 0)
     {
          ++hftirc->nbuf;
          strcpy(hftirc->cb[hftirc->nbuf - 1].name, input);
          ui_buf_set(hftirc->nbuf - 1);
          ui_print_buf(hftirc->nbuf - 1, "  .:. Query with %s", input);
     }
     else
          WARN("Error", "Usage: /query <nick>");

     return;
}

