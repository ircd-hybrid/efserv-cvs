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
 * $Id: channels.c,v 1.2 2001/05/27 10:16:27 a1kmm Exp $
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "efserv.h"

void
cleanup_channels(void)
{
 static time_t last_cleanup = 0;
 struct List *node, *nnode;
 struct Channel *ch;
 if (timenow - last_cleanup < 5)
  return;
 last_cleanup = timenow;
 FORLISTDEL(node,nnode,Channels,struct Channel*,ch)
  if (!HasSMODES(ch) && ch->ops == NULL && ch->nonops == NULL)
   remove_from_list(&Channels, node);
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
