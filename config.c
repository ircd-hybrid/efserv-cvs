/*
 *  config.c: The efserv configuration file.
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
 * $Id: config.c,v 1.10 2001/07/30 06:51:03 a1kmm Exp $
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "define.h"
#include "struct.h"
#include "utils.h"
#include "funcs.h"

char *values[200][2];
int keyc;
FILE *yyin;
extern struct List *serv_admins;
extern int lineno;

void yyparse(void);

int
check_admin(struct User *usr, const char *name, const char *pass)
{
 struct List *node;
 struct ServAdmin *sa;
 FORLIST(node,serv_admins,struct ServAdmin*,sa)
  if (!strcasecmp(sa->name, name))
  {
   if (!strcasecmp(sa->pass, pass))
   {
    struct List *node;
    struct AdminHost *ah;
    /* Now we have to go through and check against their server and
     * their host... */
    FORLIST(node,sa->hosts,struct AdminHost*,ah)
     if (match(ah->user, usr->user) &&
         match(ah->server, usr->server->name) &&
         match(ah->host, usr->host)
        )
      {
       usr->flags |= UFLAG_SERVADMIN;
       usr->caps = sa->caps;
       if (usr->sa)
        deref_admin(usr->sa);
       usr->sa = sa;
       usr->sa->refcount++;
       return -1;
      }
    return 0;
   }
   else
    return 0;
  }
 return 0;
}

struct VoteServer*
find_server_vote(const char *name)
{
 struct List *node, *node2;
 struct VoteServer *vs;
 char *tname;
 FORLIST(node,VoteServers,struct VoteServer*,vs)
  FORLIST(node2,vs->names,char*,tname)
   if (match(tname,name))
    return vs;
 return NULL;
}

void
yyerror(char *msg)
{
 log("[Config] Lex error, line %d: %s\n", lineno, msg);
 return;
}

void
check_complete(void)
{
 if (server_name == NULL || sn == NULL)
  fatal_error("General block is missing or incomplete.\n");
}

void
read_config_file(const char *file)
{
 FILE *fle;
 if ((fle = fopen(file, "r")) == NULL)
 {
  fatal_error("Could not read from %s.\n", file);
  return;
 }
 yyin = fle;
 yyparse();
 fclose(fle);
}

void
read_all_config(void)
{
 read_config_file(ETCPATH "efserv.conf");
 check_complete();
}

void
deref_admin(struct ServAdmin *ad)
{
 struct List *node, *nnode;
 struct AdminHost *ah;
 ad->refcount--;
 if (ad->refcount > 0)
  return;
 if (ad->name)
  free(ad->name);
 if (ad->pass)
  free(ad->pass);
 if (ad->hosts)
  free(ad->hosts);
 FORLISTDEL(node,nnode,ad->hosts,struct AdminHost*,ah)
 {
  free(ah->host);
  free(ah->server);
  free(ah->user);
  free(ah);
  free(node);
 }
 free(ad);
}

void
deref_voteserver(struct VoteServer *vs)
{
 char *name;
 struct List *node, *nnode;
 vs->refcount--;
 if (vs->refcount > 0)
  return;
 FORLISTDEL(node,nnode,vs->names,char*,name)
 {
  free(node);
  free(name);
 }
 free(vs);
}

void
pick_a_hub(void)
{
 struct List *node;
 struct Hub *hub;
 int hub_count = 0, hub_no;
 FORLIST(node,Hubs,struct Hub*,hub)
  hub_count++;
 if (hub_count == 0)
  fatal_error("No hubs are defined, can't connect.\n");
 hub_no = rand() % hub_count;
 FORLIST(node,Hubs,struct Hub*,hub)
  if (hub_no-- == 0) 
   break;
 server_host = hub->host;
 port = hub->port;
 server_pass = hub->pass;
}

void
do_rehash(void)
{
 struct List *node, *nnode;
 struct ServAdmin *sa;
 struct VoteServer *vs;
 struct JExempt *je;
 struct Hub *hub;
 char *kw;
 FORLISTDEL(node,nnode,serv_admins,struct ServAdmin*,sa)
 {
  free(node);
  if (sa->refcount == 0)
   deref_admin(sa);
 }
 FORLISTDEL(node,nnode,VoteServers,struct VoteServer*,vs)
 {
  free(node);
  if (vs->refcount == 0)
   deref_voteserver(vs);
 }
 FORLISTDEL(node,nnode,Hubs,struct Hub*,hub)
 {
  free(hub->host);
  free(hub->pass);
  free(hub);
  free(node);
 }
 FORLISTDEL(node,nnode,HKeywords,char*,kw)
 {
  free(kw);
  free(node);
 }
 FORLISTDEL(node,nnode,JupeExempts,struct JExempt*,je)
 {
  free(je->name);
  free(je);
 }
 VoteServers = NULL;
 serv_admins = NULL;
 Hubs = NULL;
 HKeywords = NULL;
 JupeExempts = NULL;
 read_all_config();
}
