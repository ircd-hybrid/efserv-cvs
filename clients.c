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
 * $Id: clients.c,v 1.2 2001/05/27 10:16:27 a1kmm Exp $
 */
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "efserv.h"

void cleanup_hosts(void);
void m_chmode(char *sender, int parc, char **parv);
void add_cloner(char*,char*);
void remove_cloner(char*,char*);
void destroy_server(struct Server *svr);


void
cleanup_jupes(void)
{
 static time_t last_cleanup = 0;
 struct List *node, *nnode;
 struct Server *svr;
 if (timenow-last_cleanup < 10)
  return;
 last_cleanup = timenow;
 FORLISTDEL(node,nnode,Servers,struct Server*,svr)
  if (!IsJuped(svr) && svr->jupe &&
      (timenow-svr->jupe->last_active) > JUPE_EXPIRE_TIME)
  {
   struct Jupe *jp;
   struct JupeVote *jv;
   struct List *jnode, *jnnode;
   jp = svr->jupe;
   send_msg(":%s WALLOPS :Call for vote to jupe for server %s expired("
            "NOT PASSED).", sn, svr->name);
   destroy_server(svr);
   FORLISTDEL(jnode,jnnode,jp->jupevotes,struct JupeVote*,jv)
   {
    deref_admin(jv->vsa);
    deref_voteserver(jv->vs);
    free(node);
    free(jv);
   }
   free(jp);
  }
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
 cleanup_jupes();
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
  assert(usr->server);
  add_to_hash(HASH_USER, usr->nick, usr);
  usr->node = add_to_list(&Users, usr);
  usr->monnode = NULL;
  usr->sa = NULL;
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
 svr->flags = SERVFLAG_ACTIVE;
 svr->node = add_to_list(&Servers, svr);
 svr->jupe = NULL;
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
 if (usr->sa)
  deref_admin(usr->sa);
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
destroy_server_links(struct Server *svr)
{
 struct List *node, *node2, *nnode, *DeadServers=NULL;
 struct Server *csvr, *usvr;
 struct User *usr;
 FORLISTDEL(node,nnode,Servers,struct Server*,csvr)
 {
  for (usvr=csvr->uplink; usvr; usvr=usvr->uplink)
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
 if (IsJuped(svr))
 {
  /* Just to be sure not to create a race condition, send a SQUIT
   * first... */
  send_msg(":%s SQUIT %s :Clearing juped server...", sn, svr->name);
  send_msg(":%s SERVER %s 1 :Juped: %s", sn, svr->name,
           svr->jupe->reason);
  return;
 }
 if (svr->jupe)
 {
  svr->flags &= SERVFLAG_ACTIVE;
  return;
 }
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

void
m_version(char *sender, int parc, char **parv)
{
 struct User *usr;
 if ((usr = find_user(sender)) == NULL)
  return;
 if (IsOper(usr))
  send_msg(":%s NOTICE %s :This is efserv "VERSION, sn, usr->nick);
 else
  send_msg(":%s NOTICE %s :This is the efserv services.", sn, usr->nick);
}

void
m_admin(char *sender, int parc, char **parv)
{
 send_msg(":%s NOTICE %s :Please direct efserv queries through your "
          "local server administrator.", sn, sender);
}

void
m_motd(char *sender, int parc, char **parv)
{
 send_msg(":%s NOTICE %s :Please contact your local server administrator"
          " if you would like your channel re-opped.", sn, sender);
}
