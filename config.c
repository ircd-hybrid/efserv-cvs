/*
 *  config.c: The efserv configuration file.
 *  This is part of efserv, a services.int implementation.
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "efserv.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

char *values[200][2];
int keyc;
char *server_name=NULL, *server_pass=NULL, *server_host=NULL;
/* Interface nick... */
char *sn = NULL;
int port;

void
value_parse(void)
{
 char *key, *value;
 keyc = 0;
 while ((key = strtok(NULL, " \t\r\n")))
  if ((value = strchr(key, '=')))
  {
   *value++ = 0;
   values[keyc][0] = key;
   values[keyc++][1] = value;
  }
}

char*
find_value(const char *key)
{
 int i;
 for (i=0; i<keyc; i++)
  if (!strcasecmp(values[i][0], key))
   return values[i][1];
 return NULL;
}

void
cf_server(void)
{
 char *port_str;
 value_parse();
 if (server_name)
  free(server_name);
 if (server_pass)
  free(server_pass);
 if (server_host)
  free(server_host);
 if (sn)
  free(sn);
 if (!(server_name = find_value("NAME")) ||
     !(server_pass = find_value("PASS")) ||
     !(server_host = find_value("HOST")) ||
     !(port_str = find_value("PORT")) ||
     !(sn = find_value("NICK")))
  fatal_error("SERVER tag needs NAME, PASS, PORT, NICK and HOST values.\n");
 server_name = strdup(server_name);
 server_pass = strdup(server_pass);
 server_host = strdup(server_host);
 sn = strdup(sn);
 if ((port = strtoul(port_str, NULL, 10)) < 1 || port > 0xFFFF)
  fatal_error("PORT in SERVER tag needs to be a valid integer.\n");
}

void
check_complete(void)
{
 if (server_name == NULL)
  fatal_error("No SERVER line given in config file.\n");
}

void
read_config_file(void)
{
 FILE *fle;
 char string[2000];
 fle = fopen("efserv.conf", "r");
 if (!fle)
  fatal_error("Could not open the config file.\n");
 while (fgets(string, 2000, fle))
 {
  char *keyword = strtok(string, " \t\r\n");
  if (!keyword || *keyword == '#')
   continue;
  if (!strcasecmp(keyword, "SERVER"))
   cf_server();
 }
 check_complete();
}
