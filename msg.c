/*
 *  msg.c: Outside interaction with the services.
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
 * $Id: msg.c,v 1.8 2001/11/05 04:27:40 wcampbel Exp $
 */
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "define.h"
#include "struct.h"
#include "utils.h"
#include "funcs.h"
#include "config.h"

void
pm_monitor(struct User *usr, char *str)
{
 /* MONITOR [+|-]*/
 int on = 0;
 if (str && *str == '-')
  on = -1;
 if (on == 0)
 {
  if (usr->monnode != NULL)
  {
   send_msg(":%s NOTICE %s :You are already a monitor.", sn, usr->nick);
   return;
  }
  usr->monnode = add_to_list(&Monitors, usr);
  send_msg(":%s NOTICE %s :You are now a monitor.", sn, usr->nick);
 } else
 {
  if (usr->monnode == NULL)
  {
   send_msg(":%s NOTICE %s :You are not a monitor.", sn, usr->nick);
   return;
  }
  remove_from_list(&Monitors, usr->monnode);
  usr->monnode = NULL;
  send_msg(":%s NOTICE %s :You are no longer a monitor.", sn, usr->nick);
 }
}

#ifdef USE_SMODE
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
 if (*modes == '=')
 {
  struct Channel *ch;
  char smbuff[10], *p = smbuff;
  if ((ch = find_channel(channel)) == NULL)
  {
   send_msg(":%s NOTICE %s :No such channel(No SMODES).", sn, usr->nick);
  }
  if (IsBanChan(ch))
   *p++ = 'b';
  if (IsOperChan(ch))
   *p++ = 'o';
  if (IsAdminChan(ch))
   *p++ = 'a';
  *p++ = 0;
  send_msg(":%s NOTICE %s :SMODE for %s is %s", sn, usr->nick, channel,
           smbuff);
  return;
 }
 process_smode(channel, modes);
 write_dynamic_config();
}
#endif

void
pm_jupe(struct User *usr, char *str)
{
 char *svr, *reason;
 struct VoteServer *vs;
 struct JupeVote *jv;
 struct Jupe *jp;
 struct Server *ssvr;
 struct List *node;
 struct JExempt *je;
 if (first_server == NULL)
  return;
 if ((vs = find_server_vote(usr->server->name)) == NULL)
 {
  send_msg(":%s NOTICE %s :Your server has not been given access yet, "
           "or the access has been revoked due to abuse.", sn, usr->nick);
  log("[Jupe] %s!%s@%s attempted to JUPE from unauthorised server %s.\n",
      usr->nick, usr->user, usr->host, usr->server->name);
  return;
 }
 /* server [Reason|+|-] */
 if (!(svr = strtok(str, " ")) || !(reason = strtok(NULL, "")))
 {
  send_msg(":%s NOTICE %s :Usage: JUPE server reason", sn, usr->nick);
  return;
 }
 if (strchr(svr, '.') == NULL)
 {
  send_msg(":%s NOTICE %s :Invalid servername.", sn, usr->nick);
  log("[Jupe] %s!%s@%s[%s]{%s} attempted to JUPE invalid servername %s.\n",
      usr->nick, usr->user, usr->host, usr->server->name,
      usr->sa ? usr->sa->name : "oper", svr);
  return;
 }
 if (!strcasecmp(svr, server_name) || !strcasecmp(svr, first_server->name))
 {
  log("[Jupe] %s!%s@%s[%s]{%s} attempted to perform jupe on "
      "illegal(services or active hub) server %s.\n",
      usr->nick, usr->user, usr->host, usr->server->name,
      usr->sa ? usr->sa->name : "oper", svr);
  send_msg(
   ":%s NOTICE %s :Cannot jupe services or the server it connects to.",
   sn, usr->nick);
  return;
 }
 /* See if they are exempt... */
 FORLIST(node,JupeExempts,struct JExempt*,je)
  if (match(je->name, svr) && (je->flags & JEFLAG_MANUAL))
  {
   send_msg(":%s NOTICE %s :Sorry, that server is exempt from jupes.",
            sn, usr->nick);
   return;
  }
 /* Now we have to look up the jupe... */
 ssvr = find_server(svr);
 if ((ssvr == NULL || ssvr->jupe == NULL) &&
     (*reason == '+' || *reason == '-'))
 {
  /* Can't vote for/against a non-existant jupe... */
  send_msg(":%s NOTICE %s :That jupe does not exist.", sn, usr->nick);
  return;
 } else if ((ssvr != NULL && ssvr->jupe != NULL) &&
            !(*reason == '+' || *reason == '-'))
 {
  send_msg(":%s NOTICE %s :Jupe already exists; use +/- to vote on it",
           sn, usr->nick);
  return;
 }
 if (ssvr == NULL || ssvr->jupe == NULL)
 {
  if (ssvr == NULL)
  {
   ssvr = malloc(sizeof(*ssvr));
   strncpy(ssvr->name, svr, SERVLEN-1)[SERVLEN-1] = 0;
   ssvr->flags = 0;
   ssvr->uplink = NULL;
   ssvr->node = add_to_list(&Servers, ssvr);
   add_to_hash(HASH_SERVER, ssvr->name, ssvr);
  }
  ssvr->jupe = jp = malloc(sizeof(*ssvr->jupe));
  strncpy(jp->reason, reason, 254)[254] = 0;
  jp->score = IsServAdmin(usr) ? 5 : 3;
  jp->last_active = timenow;
  jp->flags = 0;
  jp->jupevotes = NULL;
  jv = malloc(sizeof(*jv));
  jv->vs = vs;
  vs->refcount++;
  jv->vsa = usr->sa;
  jv->score = IsServAdmin(usr) ? 5 : 3;
  if (usr->sa)
   usr->sa->refcount++;
  add_to_list(&jp->jupevotes, jv);
  log("[Jupe] %s!%s@%s[%s]{%s} called for votes on juping %s for %s\n",
      usr->nick, usr->user, usr->host, usr->server->name,
      usr->sa ? usr->sa->name : "oper", svr, reason);
  send_msg(":%s WALLOPS :%s %s!%s@%s[%s]{%s} is calling for votes to "
           "jupe %s for: %s", sn, IsServAdmin(usr)?"Admin":"Oper",
           usr->nick, usr->user, usr->host, usr->server->name,
           usr->sa ? usr->sa->name : "oper", svr, reason);
  /* No action will be taken yet... */
  return;
 }
 /* Now find the current status of their vote... */
 jp = ssvr->jupe;
 FORLIST(node,jp->jupevotes,struct JupeVote*,jv)
 {
  if ((jv->vs == vs) || (jv->vsa && jv->vsa == usr->sa))
   break;
 }
 if (node == NULL)
 {
  /* Easy, it is their first vote... */
  jv = malloc(sizeof(*jv));
  jp->score += (IsServAdmin(usr)?5:3) * (*reason=='-' ? -1 : 1);
  jv->score = (IsServAdmin(usr)?5:3) * (*reason=='-' ? -1 : 1);
  jv->vs = vs;
  vs->refcount++;
  jv->vsa = usr->sa;
  if (usr->sa != NULL)
   usr->sa->refcount++;
  add_to_list(&jp->jupevotes, jv);
  log("[Jupe] %s!%s@%s[%s]{%s} voted %s juping %s.\n",
      usr->nick, usr->user, usr->host, usr->server->name,
      usr->sa ? usr->sa->name : "oper", *reason=='+' ? "for":"against",
      svr);
  send_msg(":%s WALLOPS :%s %s!%s@%s[%s]{%s} is voting %s %s jupe.",
           sn, IsServAdmin(usr)?"Admin":"Oper",
           usr->nick, usr->user, usr->host, usr->server->name,
           usr->sa ? usr->sa->name : "oper",
           *reason=='+'?"for":"against", svr);
  if (IsJuped(ssvr) && jp->score <= 0)
  {
   ssvr->flags &= ~(SERVFLAG_JUPED | SERVFLAG_ACTIVE);
   ssvr->jupe->last_active = timenow;
   send_msg(":%s SQUIT %s :Jupe has been deactivated.", sn, svr);
   log("[Jupe] Jupe for %s has been deactivated.\n", svr);
   send_msg(":%s WALLOPS :Jupe for %s deactivated.", sn, svr);
  }
  else if (!IsJuped(ssvr) && jp->score >= 15)
  {
   ssvr->flags |= SERVFLAG_JUPED | SERVFLAG_ACTIVE;
   /* Regardless of previous activity always send SQUIT to be sure... */
   send_msg(":%s SQUIT %s :Juped: %s", sn, svr, jp->reason);
   send_msg(":%s SERVER %s 2 :Juped: %s", sn, svr, jp->reason);
   send_msg(":%s WALLOPS :Jupe for %s activated.", sn, svr);
   log("[Jupe] Jupe for %s has been activated.\n", svr);
   destroy_server_links(ssvr);
  }
  return;
 }
 /* Okay, some admin/server is trying to vote twice/change vote... */
 if ((jv->score < 0 && *reason == '-') ||
     (jv->score > 0 && *reason == '+'))
 {
  /* They are either abusive or stupid :)... */
  if (usr->sa != NULL && usr->sa == jv->vsa)
  {
   /* Almost definitely abusive... */
   log("[Jupe] Admin %s!%s@%s[%s]{%s} voted more than once %s "
       "juping %s.\n",
       usr->nick, usr->user, usr->host, usr->server->name,
       usr->sa->name, *reason=='+' ? "for":"against",
       svr);
   send_msg(":%s WALLOPS :Admin %s!%s@%s[%s]{%s} has attempted to vote "
            "at least twice %s the juping of %s.",
            sn, usr->nick, usr->user, usr->host, usr->server->name,
            usr->sa->name, jv->score > 0 ? "for":"against", svr);
  }
  else
  {
   send_msg(":%s WALLOPS :%s %s!%s@%s[%s]{%s} has attempted to vote "
            "%s the juping of %s for a server-group which has already "
            "voted.",
            sn, IsServAdmin(usr)?"Admin":"Oper", usr->nick, usr->user,
            usr->host, usr->server->name, usr->sa ? usr->sa->name : "oper",
            jv->score > 0 ? "for":"against", svr);
   log("[Jupe] %s!%s@%s[%s]{%s} attempted to vote on behalf of a server"
       " which has already voted %s juping %s.\n",
       usr->nick, usr->user, usr->host, usr->server->name,
       usr->sa ? usr->sa->name:"oper", *reason=='+' ? "for":"against",
       svr);
  }      
  return;
 }
 /* Opers cannot reverse admin votes... */
 if ((jv->score >= 5 || jv->score <= -5) && !IsServAdmin(usr))
 {
  log("[Jupe] %s!%s@%s[%s] attempted to reverse server vote(%s) regarding"
      " the juping of %s, which was placed by an admin.\n",
      usr->nick, usr->user, usr->host, usr->server->name,
      (jv->score>0) ? "yes":"no", svr);
  send_msg(":%s NOTICE %s :Only admins can reverse admin votes.", sn,
           usr->nick);
  return;
 }
 /* Now reverse the previous vote... */
 jp->score -= jv->score;
 jv->score = (IsServAdmin(usr) ? 5 : 3) * (*reason=='-'?-1:1);
 jp->score += jv->score;
 /* Notify the admins... */
 if (usr->sa != NULL && jv->vsa == usr->sa)
 {
  send_msg(":%s WALLOPS :Admin %s!%s@%s[%s]{%s} is changing their vote "
           "regarding the juping of %s to %s.", sn, usr->nick,
           usr->user, usr->host, usr->server->name, usr->sa->name,
           svr, jv->score > 0 ? "yes":"no");
  log("[Jupe] %s!%s@%s[%s]{%s} has changed their (admin) vote "
      "regarding the juping of %s to %s.\n", 
      usr->nick, usr->user, usr->host, usr->server->name,
      usr->sa ? usr->sa->name:"oper", svr,(jv->score>0) ? "yes":"no");
 }
 else
 {
  send_msg(":%s WALLOPS :%s %s!%s@%s[%s]{%s} is changing the server "
           "group vote regarding the juping of %s to %s.", sn,
           IsServAdmin(usr)?"Admin":"Oper", usr->nick, usr->user,
           usr->host, usr->server->name, usr->sa ? usr->sa->name : "oper",
           svr, jv->score > 0 ? "yes" : "no");
  log("[Jupe] %s!%s@%s[%s]{%s} has changed the server vote "
      "regarding the juping of %s to %s.\n", 
      usr->nick, usr->user, usr->host, usr->server->name,
      usr->sa ? usr->sa->name:"oper", svr,(jv->score>0) ? "yes":"no");
 }    
 /* I think this is the best way, and if the services is correctly
  * configured it should not be able to be abused... */
 vs->refcount++;
 deref_voteserver(jv->vs);
 jv->vs = vs;
 if (usr->sa)
 {
  usr->sa->refcount++;
  deref_admin(jv->vsa);
 }
 jv->vsa = usr->sa;
 /* Now check the score... */
 if (IsJuped(ssvr) && jp->score <= 0)
 {
  ssvr->flags &= ~(SERVFLAG_JUPED | SERVFLAG_ACTIVE);
  ssvr->jupe->last_active = timenow;
  send_msg(":%s SQUIT %s :Jupe has been deactivated.", sn, svr);
  send_msg(":%s WALLOPS :Jupe for %s deactivated.", sn, svr);
  log("[Jupe] Jupe for %s has been activated.\n", svr);
 }
 else if (!IsJuped(ssvr) && jp->score >= 15)
 {
  ssvr->flags |= SERVFLAG_JUPED | SERVFLAG_ACTIVE;
  /* Regardless of previous activity always send SQUIT to be sure... */
  send_msg(":%s SQUIT %s :Juped: %s", sn, svr, jp->reason);
  send_msg(":%s SERVER %s 2 :Juped: %s", server_name, svr, jp->reason);
  send_msg(":%s WALLOPS :Jupe for %s activated.", sn, svr);
  log("[Jupe] Jupe for %s has been deactivated.\n", svr);
  destroy_server_links(ssvr);
 }
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

#ifdef USE_REOP
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
#endif

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
 if (check_admin(usr, user, pass))
 {
  send_msg(":%s NOTICE %s :You are now a services operator.", sn,
           usr->nick);
 }
 else
  send_msg(":%s NOTICE %s :ADMIN command failed.", sn, usr->nick);
}

void
pm_sunjupe(struct User *usr, char *str)
{
 char *server;
 struct Server *svr;
 struct Jupe *jp;
 struct JupeVote *jv;
 struct List *node, *nnode;
 if (!CanSUnjupe(usr))
 {
  send_msg(":%s NOTICE %s :Permission denied.", sn, usr->nick);
  return;
 }
 if ((server = strtok(str, " \t")) == NULL)
 {
  send_msg(":%s NOTICE %s :Usage: SUNJUPE server.name", sn, usr->nick);
  return;
 }
 if ((svr = find_server(server)) == NULL || !IsJuped(svr))
 {
  send_msg(":%s NOTICE %s :That server isn't juped.", sn, usr->nick);
  return;
 }
 log("[SUNJUPE] Admin %s!%s@%s[%s]{%s} is super-unjuping %s.\n",
     usr->nick, usr->user, usr->host, usr->server->name, usr->sa->name,
     svr->name);
 send_msg(":%s WALLOPS :Admin %s!%s@%s[%s]{%s} is super-unjuping %s.",
     sn, usr->nick, usr->user, usr->host, usr->server->name,
     usr->sa->name, svr->name);
 send_msg(":%s SQUIT %s :Super-unjuped by administrator.",
          sn, svr->name);
 /* Actually destroy the jupe, so it can be re-juped... */
 jp = svr->jupe;
 destroy_server(svr);
 FORLISTDEL(node,nnode,jp->jupevotes,struct JupeVote*,jv)
 {
  if (jv->vsa)
   deref_admin(jv->vsa);
  deref_voteserver(jv->vs);
  free(node);
  free(jv);
 }
 free(jp);
}

#ifdef USE_CYCLE
void
pm_cycle(struct User *usr, char *str)
{
 char *channel;
 struct Channel *ch;
 struct User *usr1;
 struct List *node, *nnode;
 /* CYCLE #channel */
 if ((channel = strtok(str, " \t")) == NULL || *channel != '#')
 {
  send_msg(":%s NOTICE %s :Usage: CYCLE #channel", sn, usr->nick);
  return;
 }
 if ((ch = find_channel(channel)) == NULL)
 {
  send_msg(":%s NOTICE %s :That channel does not exist.", sn, usr->nick);
  return;
 }
 FORLIST(node,ch->ops,struct User*,usr1)
  if (usr1 == usr)
   break;
 if (usr1 != usr)
 {
  send_msg(":%s NOTICE %s :You are not an op on that channel.", sn,
           usr->nick);
  return;
 }
#if 0
 if (ch->cycops != NULL || (timenow - ch->cycled) < 60*60)
 {
  send_msg(":%s NOTICE %s :You cannot cycle your channel more than "
           "once an hour. Try again in %ld minutes.", sn, usr->nick,
           60 - (timenow-ch->cycled)/60);
  return;
 }
#endif
 send_msg(":%s MODE %s +i", sn, ch->name);
 FORLISTDEL(node,nnode,ch->nonops,struct User*,usr1)
 {
  send_msg(":%s KICK %s :%s", sn, ch->name, usr1->nick);
  free(node);
 }
 FORLISTDEL(node,nnode,ch->ops,struct User*,usr1)
  send_msg(":%s KICK %s :%s", sn, ch->name, usr1->nick);
 ch->cycops = ch->ops;
 ch->ops = NULL;
 ch->cycled = timenow;
 ch->nonops = NULL;
}
#endif

struct
{
 char *name;
 void (*func)(struct User*, char*);
 int alevel;
} OpCommands[] =
{
 {"JUPE", pm_jupe, ALEVEL_OPER},
 {"HELP", pm_help, ALEVEL_OPER},
#ifdef USE_SMODE
 {"SMODE", pm_smode, ALEVEL_OPER},
#endif
#ifdef USE_REOP
 {"REOP", pm_reop, ALEVEL_OPER},
#endif
#ifdef USE_CYCLE
 {"CYCLE", pm_cycle, ALEVEL_ANY},
#endif
 {"ADMIN", pm_admin, ALEVEL_OPER},
 {"MONITOR", pm_monitor, ALEVEL_OPER},
 {"SUNJUPE", pm_sunjupe, ALEVEL_SERVADMIN},
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
 if ((cmd = strtok(parv[2], " ")) == NULL)
  return;
 if ((msg = strtok(NULL, "")) == NULL)
  msg = "";
 for (i=0; OpCommands[i].name; i++)
  if (!strcasecmp(OpCommands[i].name, cmd))
  {
   if ((OpCommands[i].alevel == ALEVEL_ADMIN && !IsAdmin(usr)) ||
       (OpCommands[i].alevel == ALEVEL_OPER && !IsOper(usr)) ||
       (OpCommands[i].alevel == ALEVEL_SERVADMIN && !IsServAdmin(usr))
       )
   {
    send_msg(":%s NOTICE %s :Permission denied.", sn, usr->nick);
    return;
   }
   OpCommands[i].func(usr, msg);
   return;
  }
 send_msg(":%s NOTICE %s :No such command.", sn, sender);  
}
