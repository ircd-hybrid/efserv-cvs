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

struct List *Servers = NULL, *Users = NULL;
struct Server *first_server = NULL;

void
m_ping(char *sender, int parc, char **parv)
{
 if (parc < 2)
  parv[1] = sender ? sender : "";
 send_msg(":%s PONG %s %s", server_name, parv[1], server_name);
}

void
pm_jupe(struct User *usr, char *str)
{
 char *svr, *reason;
 struct Server *ssvr;
 if (first_server == NULL)
  return;
 /* server Reason */
 if (!(svr = strtok(str, " ")) || !(reason = strtok(NULL, "")))
 {
  send_msg(":%s NOTICE %s :Usage: JUPE server reason", sn, usr->nick);
  return;
 }
 if (strchr(svr, '.') == NULL)
 {
  send_msg(":%s NOTICE %s :Invalid servername.", sn, usr->nick);
  return;
 }
 if (!strcasecmp(svr, server_name) || !strcasecmp(svr, first_server->name))
 {
  send_msg(
   ":%s NOTICE %s :Cannot jupe services or the server it connects to.",
   sn, usr->nick);
  return;
 }
 send_msg(":%s WALLOPS :Server %s being juped by %s!%s@%s[%s]: %s",
          sn, svr, usr->nick, usr->user, usr->host, usr->server->name,
          reason);
 if ((ssvr = find_server(svr)))
  send_msg(":%s SQUIT %s :Juped: %s", sn, svr, reason);
 send_msg(":%s SERVER %s 2 :Juped: %s", server_name, svr, reason);
}

void
pm_niy(struct User *usr, char *str)
{
 send_msg(":%s NOTICE %s :Not implemented yet.", sn, usr->nick);
}

struct
{
 char *name;
 void (*func)(struct User*, char*);
} OpCommands[] =
{
 {"JUPE", pm_jupe},
 {0, 0}
};

void
m_privmsg(char *sender, int parc, char **parv)
{
 struct User *usr;
 char *cmd, *msg;
 int i;
 /* :sender PRIVMSG recipient :Message */
 if (sender == NULL || parc < 2)
  return;
 if (parv[1][0] == '#')
  return;
 if (!(usr = find_user(sender)))
  return;
 if (!IsAdmin(usr))
 {
  send_msg(
   ":%s NOTICE %s :You must be an administrator(+a) to use services.",
           sn, sender);
  return;
 }
 if ((cmd = strtok(parv[2], " ")) == NULL)
  return;
 if ((msg = strtok(NULL, "")) == NULL)
  msg = "";
 for (i=0; OpCommands[i].name; i++)
  if (!strcasecmp(OpCommands[i].name, cmd))
  {
   OpCommands[i].func(usr, msg);
   return;
  }
 send_msg(":%s NOTICE %s :No such command.", sn, sender);  
}

unsigned long
parse_umode(const char *umode, unsigned long m)
{
 char c;
 int d = 0;
 while ((c = *umode++))
  switch (c)
  {
   case '+':
    d = 0;
    break;
   case '-':
    d = -1;
    break;
   case 'o':
    if (d)
     m &= ~ UFLAG_OPER;
    else
     m |=   UFLAG_OPER;
    break;
   case 'a':
    if (d)
     m &= ~ UFLAG_ADMIN;
    else
     m |=   UFLAG_ADMIN;
    break;
  }
 return m;
}

void
m_mode(char *sender, int parc, char **parv)
{
 /* :sender MODE nick :+modes */
 struct User *usr;
 if (parc < 3)
  return;
 if ((usr = find_user(parv[1])) == NULL)
  return;
 usr->flags = parse_umode(parv[2], usr->flags);
}

void
m_nick(char *sender, int parc, char **parv)
{
 if (sender == NULL)
  return;
 if (strchr(sender, '.'))
 {
  /* :server NICK nick hops TS +umode username hostname server :Real name*/
  struct User *usr;
  struct Server *svr;
  if (parc < 9)
   return;
  if (!(svr = find_server(parv[7])))
   return;
  usr = malloc(sizeof(*usr));
  strncpy(usr->nick, parv[1], NICKLEN-1)[NICKLEN-1] = 0;
  strncpy(usr->user, parv[5], USERLEN-1)[USERLEN-1] = 0;
  strncpy(usr->host, parv[6], USERLEN-1)[USERLEN-1] = 0;
  usr->server = svr;
  usr->flags = parse_umode(parv[4], 0);
  add_to_hash(HASH_USER, usr->nick, usr);
  usr->node = add_to_list(&Users, usr);
 } else
 {
  /* :nick NICK newnick */
  struct User *usr;
  if (parc < 2)
   return;
  if ((usr = find_user(sender)) == NULL)
   return;
  remove_from_hash(HASH_USER, usr->nick);
  strncpy(usr->nick, parv[1], NICKLEN-1)[NICKLEN-1] = 0;
  add_to_hash(HASH_USER, usr->nick, usr);
 }
}

void
m_server(char *sender, int parc, char **parv)
{
 struct Server *svr;
 /* :sender SERVER name hops :Realname 
  * We only care about the name, though.
  */
 if (parc < 2)
  return;
 if (strchr(parv[1], '.') == NULL)
  return;
 if (find_server(parv[1]))
  return;
 svr = malloc(sizeof(*svr));
 strncpy(svr->name, parv[1], SERVLEN-1)[SERVLEN-1] = 0;
 svr->flags = 0;
 svr->node = add_to_list(&Servers, svr);
 add_to_hash(HASH_SERVER, svr->name, svr);
 if (first_server == NULL)
 {
  first_server = svr;
  svr->uplink = NULL;
 }
 else
 {
  svr->uplink = find_server(parv[0]);
  if (svr->uplink == NULL)
   svr->uplink = first_server;
 }
}

void
destroy_user(struct User *usr)
{
 remove_from_hash(HASH_USER, usr->nick);
 remove_from_list(&Users, usr->node);
 free(usr);
}

void
destroy_server(struct Server *svr)
{
 struct List *node, *node2, *nnode, *DeadServers=NULL;
 struct Server *csvr, *usvr;
 struct User *usr;
 FORLISTDEL(node,nnode,Servers,struct Server*,csvr)
 {
  for (usvr=csvr; usvr; usvr=usvr->uplink)
   if (usvr == svr)
    break;
  if (usvr)
  {
   remove_from_list(&Servers, csvr->node);
   remove_from_hash(HASH_SERVER, csvr->name);
   csvr->node = add_to_list(&DeadServers, csvr);
  }
 }
 FORLISTDEL(node,nnode,Users,struct User*,usr)
  FORLIST(node2,DeadServers,struct Server*,csvr)
   if (usr->server == csvr)
    destroy_user(usr);
 FORLISTDEL(node,nnode,DeadServers,struct Server*,csvr)
 {
  free(node);
  free(csvr);
 }
}

void
m_squit(char *sender, int parc, char **parv)
{
 /* :doer SQUIT server :reason */
 struct Server *svr;
 if (parc < 2)
  return;
 if ((svr = find_server(parv[1])) == NULL)
  return;
 destroy_server(svr);
}

void
m_quit(char *sender, int parc, char **parv)
{
 /* :quitter QUIT :Reason. */
 struct User *usr;
 if ((usr = find_user(sender)) == NULL)
  return;
 destroy_user(usr);
}

void
m_kill(char *sender, int parc, char **parv)
{
 /* :doer KILL client :Reason */
 struct User *usr;
 if (parc < 2)
  return;
 if (!strcasecmp(parv[1], sn))
 {
  send_msg("NICK %s 1 1 +o services %s %s :* Services *", sn,
           server_name, server_name);
  return;
 }
 if ((usr = find_user(parv[1])) == NULL)
  return;
 destroy_user(usr);
}

struct Command Commands[] =
{
 {"PING", m_ping},
 {"SERVER", m_server},
 {"NICK", m_nick},
 {"SQUIT", m_squit},
 {"QUIT", m_squit},
 {"KILL", m_kill},
 {"MODE", m_mode},
 {"PRIVMSG", m_privmsg},
 {0, 0}
};
