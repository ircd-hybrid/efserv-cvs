/*
 *  channels.c: Channel related functions.
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
 * $Id: channels.c,v 1.4 2001/05/30 04:10:14 a1kmm Exp $
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "efserv.h"

void
check_channel_status(struct Channel *ch)
{
 struct List *node, *nnode, *node2, *BestOps = NULL;
 int count = 0, i;
 struct ChanopUser *cou, *cou2;
 struct User *usr;
 if (server_count < minimum_servers)
  return;
 if (ch->ops == NULL)
 {
  if (ch->exops == NULL)
   return;
  FORLIST(node,ch->exops,struct ChanopUser*,cou)
  {
   i = 0;
   FORLIST(node2,BestOps,struct ChanopUser*,cou2)
    if (++i > 5 || cou2->slices <= cou->slices)
     break;
   if (i>5 || (node2 == NULL && count >= 5))
    continue;
   add_to_list_before(&BestOps, node2, cou);
   count++;
  }
  if (count > 5)
   count = 5;
  FORLISTDEL(node,nnode,ch->nonops,struct User*,usr)
  {
   char *md5 = getmd5(usr);
   i = 0;
   FORLIST(node2,BestOps,struct ChanopUser*,cou)
   {
    if (++i > count)
     continue;
    if (!memcmp(md5, cou->uhost_md5, 16))
    {
     remove_from_list(&ch->nonops, node);
     add_to_list(&ch->ops, usr);
     send_msg(":%s MODE %s +o %s", sn, ch->name, usr->nick);
     log("[AutoReop] Auto-reopped %s on channel %s\n", usr->nick,
         ch->name);
    }
   }
  }
  FORLISTDEL(node,nnode,BestOps,struct ChanopUser *,cou)
   free(node);
  return;
 }
#ifdef MINIMUM_OPS
 if (ch->exops == NULL)
 {
  count = 0;
  FORLIST(node,ch->ops,struct User*,usr)
   count++;
  if (count < MINIMUM_OPS)
   return;
 }
#endif
 /* Now go through all the ops... */
 FORLIST(node,ch->ops,struct User*,usr)
 {
  char *md5 = getmd5(usr);
  FORLIST(node2,ch->exops,struct ChanopUser*,cou)
   if (!memcmp(md5, cou->uhost_md5, 16))
   {
    cou->last_opped = timenow;
    cou->slices++;
    break;
   }
  if (node2 == NULL)
  {
   cou = malloc(sizeof(*cou));
   memcpy(cou->uhost_md5, md5, 16);
   cou->last_opped = timenow;
   cou->slices = 1;
   add_to_list(&ch->exops, cou);
  }
 }
 /* Now we simply have to go through and delete the expired
  * ops... */
 FORLISTDEL(node,nnode,ch->exops,struct ChanopUser*,cou)
  if ((timenow-cou->last_opped) > 7*24*60*60)
   remove_from_list(&ch->exops, node);
}

void
cleanup_channels(void)
{
 int doing_reop = 0;
 static time_t last_reop = 0;
 struct List *node, *nnode;
 struct Channel *ch;
 if (timenow - last_reop > CHAN_SLICE_LENGTH)
 {
  doing_reop = -1;
  last_reop = timenow;
 }
 FORLISTDEL(node,nnode,Channels,struct Channel*,ch)
 {
  /* Check if it is empty... */
  if (ch->ops == NULL && ch->nonops == NULL)
  {
   if (timenow-ch->last_notempty > 60*60*2)
    remove_from_list(&Channels, node);
   continue;
  } else
   ch->last_notempty = timenow;
  if (doing_reop)
   check_channel_status(ch);
 }
 if (doing_reop)
  save_channel_opdb();
}

#ifdef USE_SMODES
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
#endif

void
m_sjoin(char *sender, int parc, char **parv)
{
 struct Channel *ch;
 time_t newts;
 char *p;
 /* :sender SJOIN ts channel + :@x ... */
 if (parc < 5)
  return;
 cleanup_channels();
 newts = strtoul(parv[1], NULL, 10); 
 if (!(ch = find_channel(parv[2])))
 {
  ch = malloc(sizeof(*ch));
  strncpy(ch->name, parv[2], CHANLEN-1)[CHANLEN-1] = 0;
  ch->flags = 0;
  ch->ts = newts;
  ch->ops = NULL;
  ch->nonops = NULL;
  ch->exops = NULL;
  ch->last_notempty = timenow;
  add_to_list(&Channels, ch);
  add_to_hash(HASH_CHAN, ch->name, ch);
 }
 if ((parv[parc-1][0] == '@' || parv[parc-1][1] == '@') && newts < ch->ts)
  /* We now have to remove all the ops... */
  move_list(&ch->nonops, &ch->ops);
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
#ifdef USE_SMODES
  if (kick_excluded(ch, usr))
   continue;
#endif
  add_to_list(isop ? &ch->ops : &ch->nonops, usr);
 }
}

void
m_part(char *sender, int parc, char **parv)
{
 struct Channel *ch;
 struct User *usr, *usr2;
 struct List *node, *nnode;
 /* :sender PART #channel */
 if (parc < 2)
  return;
 if ((usr = find_user(sender)) == NULL)
  return;
 if ((ch = find_channel(parv[1])) == NULL)
  return;
 FORLISTDEL(node,nnode,ch->ops,struct User*,usr2)
  if (usr2 == usr)
   remove_from_list(&ch->ops, node);
 FORLISTDEL(node,nnode,ch->nonops,struct User*,usr2)
  if (usr2 == usr)
   remove_from_list(&ch->nonops, node);
}

void
m_join(char *sender, int parc, char **parv)
{
 /* :sender JOIN 0 */
 struct List *node, *node2, *nnode2;
 struct Channel *ch;
 struct User *usr, *usr2;
 if (parc < 2)
  return;
 if ((usr = find_user(sender)) == NULL)
  return;
 if (parv[1][0] != '0' || parv[1][1] != 0)
  return;
 FORLIST(node,Channels,struct Channel*,ch)
 {
  FORLISTDEL(node2,nnode2,ch->nonops,struct User *,usr2)
   if (usr2 == usr)
    remove_from_list(&ch->nonops, node2);
  FORLISTDEL(node2,nnode2,ch->ops,struct User *,usr2)
   if (usr2 == usr)
    remove_from_list(&ch->ops, node2);
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

#ifdef USE_SMODE
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
#endif
