/*
 *  commands.c: The command table for efserv.
 *  This is part of efserv, a services.int implementation.
 *  efserv is Copyright(C) 2001 by Andrew Miller, and others.
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *    MA  02111-1307  USA.
 * $Id: commands.c,v 1.4 2001/05/26 01:41:03 a1kmm Exp $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "efserv.h"

void m_ping(char*, int, char**);
void m_server(char*, int, char**);
void m_nick(char*, int, char**);
void m_squit(char*, int, char**);
void m_quit(char*, int, char**);
void m_kill(char*, int, char**);
void m_mode(char*, int, char**);
void m_privmsg(char*, int, char**);
void m_sjoin(char*, int, char**);
void m_admin(char*, int, char**);
void m_version(char*, int, char**);

struct Command Commands[] =
{
 {"PING", m_ping},
 {"SERVER", m_server},
 {"NICK", m_nick},
 {"SQUIT", m_squit},
 {"QUIT", m_quit},
 {"KILL", m_kill},
 {"MODE", m_mode},
 {"PRIVMSG", m_privmsg},
 {"SJOIN", m_sjoin},
 {0, 0}
};

void
hash_commands(void)
{
 struct Command *cmd;
 for (cmd=Commands; cmd; cmd++)
 {
  if (cmd->name == NULL)
   return;
  add_to_hash(HASH_COMMAND, cmd->name, cmd);
 }
}
