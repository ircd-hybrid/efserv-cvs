/*
 *  commands.c: The command table for efserv.
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "efserv.h"

void
m_ping(char *sender, int parc, char **parv)
{
 printf("Got a ping.\n");
}

void
m_notice(char *sender, int parc, char **parv)
{
 if (parc < 3)
  return;
 printf("NOTICE: %s\n", parv[2]);
}

void
m_privmsg(char *sender, int parc, char **parv)
{
 
}

void
m_nick(char *sender, int parc, char **parv)
{
 if (sender==NULL || strchr(sender, '.'))
 {
  /* :server NICK hops TS +umode username hostname server :Real name*/
  struct User *usr;
  struct Server *svr;
  if (parc < 8)
   return;
  usr = malloc(sizeof(*usr));
 } else
 {
 }
}

void
m_server(char *sender, int parc, char **parv)
{
}

struct Command Commands[] =
{
 {"PING", m_ping},
 {"NOTICE", m_notice},
 {0, 0}
};
