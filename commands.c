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
#include <assert.h>
#include "efserv.h"

struct List *Servers = NULL, *Users = NULL, *Channels = NULL;
struct Server *first_server = NULL;

void
m_ping(char *sender, int parc, char **parv)
{
 if (parc < 2)
  parv[1] = sender ? sender : "";
 send_msg(":%s PONG %s %s", server_name, parv[1], server_name);
}

void
cleanup_channels(void)
{
 struct List *node, *nnode;
 struct Channel *ch;
 static time_t last_called = 0;
 if (timenow - last_called < 4)
  return;
 FORLISTDEL(node,nnode,Channels,struct Channel*,ch)
 {
  if (!HasSMODES(ch) && ch->nonops == NULL && ch->ops == NULL)
  {
   remove_from_hash(HASH_CHAN, ch->name);
   remove_from_list(&Channels, node);
   free(ch);
  }
 }
}

int
kick_excluded(struct Channel *ch, struct User *usr)
{
 if (IsBanChan(ch))
 {
  send_msg(":%s KICK %s %s :This channel has been closed by "
           NETNAME" administration.", sn, ch->name, usr->nick);
  return 1;
 }
 if (IsOperChan(ch) && !IsOper(usr))
 {
  send_msg(":%s MODE %s +b %s!*@*", sn, ch->name, usr->nick);
  send_msg(":%s KICK %s %s :You must be an IRC Operator to join "
           "this channel.", sn, ch->name, usr->nick);
  return 1;
 }
 if (IsAdminChan(ch) && !IsAdmin(usr))
 {
  send_msg(":%s MODE %s +b %s!*@*", sn, ch->name, usr->nick);
  send_msg(":%s KICK %s %s :You must be an IRC Admin to join "
           "this channel.", sn, ch->name, usr->nick);
  return 1;
 }
 return 0;
}

void
m_sjoin(char *sender, int parc, char **parv)
{
 struct Channel *ch;
 char *p;
 /* :sender SJOIN ts channel + :@x ... */
 if (parc < 5)
  return;
 cleanup_channels();
 if (!(ch = find_channel(parv[2])))
 {
  ch = malloc(sizeof(*ch));
  strncpy(ch->name, parv[2], CHANLEN-1)[CHANLEN-1] = 0;
  ch->flags = 0;
  ch->ops = NULL;
  ch->nonops = NULL;
  add_to_list(&Channels, ch);
  add_to_hash(HASH_CHAN, ch->name, ch);
 }
 for (p=strtok(parv[parc-1], " "); p; p=strtok(NULL, " "))
 {
  int isop = 0;
  struct User *usr;
  if (*p == '@')
  {
   p++;
   isop++;
  }
  if (*p == '+' || *p == '%')
   p++;
  if (!(usr = find_user(p)))
   continue;
  if (kick_excluded(ch, usr))
   continue;
  add_to_list(isop ? &ch->ops : &ch->nonops, usr);
 }
}

void
m_chmode(char *sender, int parc, char **parv)
{
 int arg = 3, dir = 0;
 char c;
 struct Channel *ch;
 struct User *usr, *usr2;
 struct List *node, *nnode;
 /* MODE #channel +o nick ... */
 /* Scan for +o or -o... */
 if (parc < 4)
  return;
 ch = find_channel(parv[1]);
 while ((c = *parv[2]++))
  switch (c)
  {
   case '-':
    dir = -1;
    break;
   case '+':
    dir = 0;
    break;
   case 'o':
    if (arg >= parc)
     continue;
    if ((usr = find_user(parv[arg])) == NULL)
     continue;
    FORLISTDEL(node,nnode,ch->ops,struct User*,usr2)
     if (usr2 == usr)
      remove_from_list(&ch->ops, node);
    FORLISTDEL(node,nnode,ch->nonops,struct User*,usr2)
     if (usr2 == usr)
      remove_from_list(&ch->nonops, node);
    add_to_list(dir ? &ch->nonops : &ch->ops, usr);
   case 'v':
   case 'h':
   case 'b':
   case 'e':
   case 'k':
   case 'l':
    arg++;
  }
}

void
process_smode(const char *name, const char *mode)
{
 struct Channel *ch;
 struct List *node, *nnode;
 struct User *usr;
 int dir = 0;
 const char *p;
 char c;
 if ((ch = find_channel(name)) == NULL)
 {
  ch = malloc(sizeof(*ch));
  ch->ops = NULL;
  ch->nonops = NULL;
  ch->flags = 0;
  strncpy(ch->name, name, CHANLEN-1)[CHANLEN-1] = 0;
  add_to_hash(HASH_CHAN, ch->name, ch);
  add_to_list(&Channels, ch);
 }
 for (p=mode; (c=*p); p++)
  switch (c)
  {
   case '+':
    dir = 0;
    break;
   case '-':
    dir = -1;
    break;
   case 'b':
    if (dir)
     ch->flags &= ~CHFLAG_BANNED;
    else
     ch->flags |= CHFLAG_BANNED;
    break;
   case 'o':
    if (dir)
     ch->flags &= ~CHFLAG_OPERONLY;
    else
     ch->flags |= CHFLAG_OPERONLY;
    break;
   case 'a':
    if (dir)
     ch->flags &= ~CHFLAG_ADMINONLY;
    else
     ch->flags |= CHFLAG_ADMINONLY;
    break;
  }
 FORLISTDEL(node,nnode,ch->ops,struct User*,usr)
  if (kick_excluded(ch, usr))
  {
   remove_from_list(&ch->ops, node);
   continue;
  }
 FORLISTDEL(node,nnode,ch->nonops,struct User*,usr)
  if (kick_excluded(ch, usr))
  {
   remove_from_list(&ch->nonops, node);
   continue;
  }
}

void
pm_smode(struct User *usr, char *str)
{
 /* SMODE #channel modes */
 char *channel, *modes;
 if ((channel = strtok(str, " ")) == NULL ||
     ((modes = strtok(NULL, " ")) == NULL)
     || *channel != '#')
 {
  send_msg(":%s NOTICE %s :Usage: SMODE #channel +/-modes", sn,
           usr->nick);
  return;
 }
 send_msg(":%s WALLOPS :SMODE on %s set %s by %s!%s@%s[%s]",
          sn, channel, modes, usr->nick, usr->user, usr->host,
          usr->server->name);
 process_smode(channel, modes);
 write_dynamic_config();
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
 else
 {
  ssvr = malloc(sizeof(*ssvr));
  strncpy(ssvr->name, svr, SERVLEN-1)[SERVLEN-1] = 0;
  ssvr->flags = 0;
  ssvr->node = add_to_list(&Servers, ssvr);
  add_to_hash(HASH_SERVER, ssvr->name, ssvr);
 }
 send_msg(":%s SERVER %s 2 :Juped: %s", server_name, svr, reason);
}

void
pm_help(struct User *usr, char *str)
{
 FILE *fhlp;
 char hlpb[2000];
 if ((fhlp = fopen("help.txt", "r")) == NULL)
  return;
 while (fgets(hlpb, 2000, fhlp))
 {
  int i;
  i = strlen(hlpb);
  if (i>0 && hlpb[i-1]=='\n')
   hlpb[i-1] = 0;
  send_msg(":%s NOTICE %s :%s", sn, usr->nick, hlpb);
 }
 fclose(fhlp);
}

void
pm_reop(struct User *usr, char *str)
{
 char *channel, *nick;
 int onchan = 0;
 struct Channel *ch;
 struct List *node, *nnode;
 struct User *usr2, *usr3;
 /* #channel nick */
 if (str==NULL || (channel = strtok(str, " ")) == NULL || *channel != '#'
     || (nick = strtok(NULL, " ")) == NULL)
 {
  send_msg(":%s NOTICE %s :Usage: REOP #channel nick", sn, usr->nick);
  return;
 }
 if (((ch = find_channel(channel)) == NULL) ||
     (ch->ops == NULL && ch->nonops == NULL))
 {
  send_msg(":%s NOTICE %s :That channel doesn't exist.", sn, usr->nick);
  return;
 }
 if ((usr2 = find_user(nick)) == NULL)
 {
  send_msg(":%s NOTICE %s :That user doesn't exist.", sn, usr->nick);
  return;
 }
 if (!IsServAdmin(usr) && ch->ops != NULL)
 {
  send_msg(":%s NOTICE %s :That channel already has ops.", sn, usr->nick);
  return;
 }
 FORLIST(node,ch->nonops,struct User*,usr3)
  if (usr3 == usr2)
   onchan = 1;
 FORLIST(node,ch->ops,struct User*,usr3)
  if (usr3 == usr2)
   onchan = 2;
 if (onchan == 0)
 {
  send_msg(":%s NOTICE %s :That user is not on the channel.", sn,
           usr->nick);
  return;
 }
 if (onchan == 2)
 {
  send_msg(":%s NOTICE %s :That user is already a chanop.", sn,
           usr->nick);
  return;
 }
 if (!IsServAdmin(usr))
  send_msg(":%s WALLOPS :Reop command used on %s by %s!%s@%s[%s]",
           sn, ch->name, usr->nick, usr->user, usr->host,
           usr->server->name);
 /* Now send a mode hack for them... */
 send_msg(":%s MODE %s +o %s", sn, ch->name, nick);
 FORLISTDEL(node,nnode,ch->nonops,struct User *, usr3)
  if (usr3 == usr2)
   remove_from_list(&ch->nonops, node);
 add_to_list(&ch->ops, usr2);
}

void
pm_admin(struct User *usr, char *str)
{
 char *user, *pass;
 /* ADMIN user passwd */
 if ((user = strtok(str, " ")) == NULL ||
     (pass = strtok(NULL, " ")) == NULL)
 {
  send_msg(":%s NOTICE %s :Usage: ADMIN adminname passwd", sn, usr->nick);
  return;
 }
 if (verify_admin(user, pass))
 {
  usr->flags |= UFLAG_SERVADMIN;
  send_msg(":%s NOTICE %s :You are now a services operator.", sn,
           usr->nick);
 }
 else
  send_msg(":%s NOTICE %s :Permission denied.", sn, usr->nick);
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
 {"HELP", pm_help},
 {"SMODE", pm_smode},
 {"REOP", pm_reop},
 {"ADMIN", pm_admin},
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
   assert(csvr->name[0] != '*');
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
 {"SJOIN", m_sjoin},
 {0, 0}
};
