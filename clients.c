/*
 *  clients.c: Server and client monitoring.
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
 * $Id: clients.c,v 1.1 2001/05/26 01:41:02 a1kmm Exp $
 */
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "efserv.h"

void cleanup_hosts(void);
void m_chmode(char *sender, int parc, char **parv);
void add_cloner(char*,char*);
void remove_cloner(char*,char*);

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
 if (parv[1][0] == '#')
 {
  m_chmode(sender, parc, parv);
  return;
 }
 if ((usr = find_user(parv[1])) == NULL)
  return;
 usr->flags = parse_umode(parv[2], usr->flags);
}

void
m_nick(char *sender, int parc, char **parv)
{
 cleanup_hosts();
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
  usr->monnode = NULL;
  add_cloner(usr->user, usr->host);
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
 struct List *node, *node2, *nnode2;
 struct Channel *ch;
 struct User *usr2;
 FORLIST(node,Channels,struct Channel*,ch)
 {
  FORLISTDEL(node2,nnode2,ch->nonops,struct User *,usr2)
   if (usr2 == usr)
    remove_from_list(&ch->nonops, node2);
  FORLISTDEL(node2,nnode2,ch->ops,struct User *,usr2)
   if (usr2 == usr)
    remove_from_list(&ch->ops, node2);
 }
 remove_cloner(usr->user, usr->host);
 if (usr->monnode)
  remove_from_list(&Monitors, usr->monnode);
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
#if 1
   /* Yucky bad debug code... */
   assert(csvr->name[0] != '$');
   csvr->name[0] = '$';
   csvr->name[1] = 0;
#endif
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

void
m_ping(char *sender, int parc, char **parv)
{
 if (parc < 2)
  parv[1] = sender ? sender : "";
 send_msg(":%s PONG %s %s", server_name, parv[1], server_name);
}
