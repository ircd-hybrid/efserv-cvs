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
 * $Id: channels.c,v 1.15 2001/12/10 07:47:19 a1kmm Exp $
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "config.h"
#include "define.h"
#include "struct.h"
#include "utils.h"
#include "funcs.h"

void
clearops_channel(struct Channel *ch)
{
  struct List *node, *nnode;
  struct User *usr;
  char *b, *p, buf[4], pbuf[2];
  FORLISTDEL(node, nnode, ch->ops, struct User *, usr)
  {
    send_msg(":%s MODE %s -o %s", sn, ch->name, usr->nick);
    free(node);
  }
  ch->ops = NULL;
  FORLISTDEL(node, nnode, ch->bans, char *, b)
  {
    send_msg(":%s MODE %s -b %s", sn, ch->name, b);
    free(b);
    free(node);
  }
  ch->bans = NULL;
  b = buf;
  p = pbuf;
  if (ch->modes & CHMODE_INVITE)
    *b++ = 'i';
  if (ch->modes & CHMODE_LIMIT)
    *b++ = 'l';
  if (ch->modes & CHMODE_KEY)
  {
    *b++ = 'k';
    *p++ = '*';
  }
  *b++ = 0;
  *p++ = 0;
  if (ch->modes & (CHMODE_INVITE | CHMODE_LIMIT | CHMODE_KEY))
  {
    send_msg(":%s MODE %s -%s %s", sn, ch->name, buf, pbuf);
  }
}

void
check_channel_status(struct Channel *ch)
{
  struct List *node, *nnode, *node2, *BestOps = NULL;
  int count = 0, i;
  struct ChanopUser *cou, *cou2;
  struct User *usr;
  char *hk;

  if (server_count < minimum_servers)
    return;
  BestOps = NULL;
  if ((timenow - ch->ts) > CHAN_RECOVER_TIME)
    ch->first_ts = ch->ts;
  else if (ch->ops != NULL && ch->ts > ch->first_ts && ch->exops != NULL)
  {
    int tflags = 0, mf;
    FORLIST(node, ch->exops, struct ChanopUser *, cou)
    {
      i = 0;
      FORLIST(node2, BestOps, struct ChanopUser *, cou2)
        if (++i > 5 || cou2->slices <= cou->slices)
          break;
      if (i > 5 || (node2 == NULL && count >= 5))
        continue;
      add_to_list_before(&BestOps, node2, cou);
      count++;
    }

    if (count > 5)
      count = 5;
    FORLIST(node, ch->ops, struct User *, usr)
    {
      char *md5 = getmd5(usr);
      mf = 0;
      i = 0;
      FORLIST(node2, BestOps, struct ChanopUser *, cou)
      {
        if (i++ > count)
          break;
        if (!memcmp(md5, cou->uhost_md5, 16))
          mf = 1;
      }
      if (mf)
        tflags |= 1;
      else
        tflags |= 2;
    }
    switch (tflags)
    {
      case 1:                  /* Only exops, reset the oldest ts... */
        ch->first_ts = ch->ts;
        break;
      case 2:                  /* Only non-exops, massdeop. */
        clearops_channel(ch);
        break;
        /* case 3: A mixture. Do nothing and see what happens. */
    }
    FORLISTDEL(node, nnode, BestOps, struct ChanopUser *, cou)
      free(node);
    return;
  }

  if (ch->ops == NULL)
  {
    if (ch->exops == NULL)
      return;
    BestOps = NULL;
    FORLIST(node, ch->exops, struct ChanopUser *, cou)
    {
      i = 0;
      FORLIST(node2, BestOps, struct ChanopUser *, cou2)
        if (++i > 5 || cou2->slices <= cou->slices)
          break;
      if (i > 5 || (node2 == NULL && count >= 5))
        continue;
      add_to_list_before(&BestOps, node2, cou);
      count++;
    }

    if (count > 5)
      count = 5;

    FORLISTDEL(node, nnode, ch->nonops, struct User *, usr)
    {
      char *md5 = getmd5(usr);
      i = 0;

      FORLIST(node2, BestOps, struct ChanopUser *, cou)
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

    FORLISTDEL(node, nnode, BestOps, struct ChanopUser *, cou)
      free(node);
    return;
  }
#ifdef MINIMUM_OPS
  if (ch->exops == NULL)
  {
    count = 0;
    FORLIST(node, ch->ops, struct User *, usr)
      count++;
    if (count < MINIMUM_OPS)
      return;
  }
#endif
  /* Now go through all the ops... */
  FORLIST(node, ch->ops, struct User *, usr)
  {
    char *md5;
    FORLIST(node2, HKeywords, char *, hk)
      if (match(hk, usr->host))
        continue;
    if (usr->host)
      md5 = getmd5(usr);
    FORLIST(node2, ch->exops, struct ChanopUser *, cou)
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
  if (ch->ops != NULL && ch->exops != NULL)
  {
    int unopped_exops = 0, opped_exops = 0;
    char *md5;
    BestOps = NULL;
    FORLIST(node, ch->exops, struct ChanopUser *, cou)
    {
      i = 0;
      FORLIST(node2, BestOps, struct ChanopUser *, cou2)
        if (++i > 5 || cou2->slices <= cou->slices)
          break;
      if (i > 5 || (node2 == NULL && count >= 5))
        continue;
      add_to_list_before(&BestOps, node2, cou);
      count++;
    }
    if (count > 5)
      count = 5;
    FORLIST(node, ch->ops, struct User *, usr)
    {
      i = 0;
      if (usr->host)
        md5 = getmd5(usr);
      FORLIST(node2, BestOps, struct ChanopUser *, cou)
        if (i++ < 5 && !memcmp(md5, cou->uhost_md5, 16))
          opped_exops++;
    }
    FORLIST(node, ch->nonops, struct User *, usr)
    {
      if (usr->host)
        md5 = getmd5(usr);
      i = 0;
      FORLIST(node2, BestOps, struct ChanopUser *, cou)
      {
        if (i++ < 5 && !memcmp(md5, cou->uhost_md5, 16))
          unopped_exops++;
      }
    }
    FORLISTDEL(node, nnode, BestOps, struct ChanopUser *, cou)
      free(node);
    if (unopped_exops > 3)
    {
      clearops_channel(ch);
      check_channel_status(ch);
    }
  }
  /* Now we simply have to go through and delete the expired
   * ops... 
   */
  FORLISTDEL(node, nnode, ch->exops, struct ChanopUser *, cou)
    if ((timenow - cou->last_opped) > EXOP_EXPIRE_TIME)
      remove_from_list(&ch->exops, node);
}

void
cleanup_channels(void)
{
  int doing_reop = 0;
  static time_t last_reop = 0;
  struct List *node, *nnode, *node2, *nnode2;
  struct User *usr;
  struct Channel *ch;
  char *chp;

  if (timenow - last_reop > CHAN_SLICE_LENGTH)
  {
    doing_reop = -1;
    last_reop = timenow;
  }

  FORLISTDEL(node, nnode, Channels, struct Channel *, ch)
  {
    /* Check the cyops list... */
#ifdef USE_CYCLE
    if (ch->cycops != NULL && (timenow - ch->cycled) > CYCLE_REJOIN_TIME)
    {
      FORLISTDEL(node2, nnode2, ch->cycops, struct User *, usr) free(node2);
      ch->cycops = NULL;
    }
#endif

    /* Check if it is empty... */
    if (ch->ops == NULL && ch->nonops == NULL)
    {
      if (timenow - ch->last_notempty > 60 * 60 * 2)
      {
        remove_from_hash(HASH_CHAN, ch->name);
        remove_from_list(&Channels, node);
        FORLISTDEL(node2, nnode2, ch->cycops, struct User *, usr)
        {
          free(node2->data);
          free(node2);
        }
        FORLISTDEL(node2, nnode2, ch->bans, char *, chp)
        {
          free(chp);
          free(node2);
        }
        free(ch);
      }
      continue;
    }
    else
    {
      ch->last_notempty = timenow;
    }

    if (doing_reop)
      check_channel_status(ch);
  }
  if (doing_reop)
    save_channel_opdb();
}

#ifdef USE_SMODE
int
kick_excluded(struct Channel *ch, struct User *usr)
{
  if (IsBanChan(ch))
  {
    send_msg(":%s KICK %s %s :This channel has been closed by "
             NETNAME " administration.", sn, ch->name, usr->nick);
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
  struct Server *svr;
  time_t newts;
  int hack = 0, new = 0;
  char *p;

  /* :sender SJOIN ts channel + :@x ... */
  if (parc < 5)
    return;

  cleanup_channels();
  newts = strtoul(parv[1], NULL, 10);
  if (!(ch = find_channel(parv[2])))
  {
    ch = malloc(sizeof(*ch));
    strncpy(ch->name, parv[2], CHANLEN - 1)[CHANLEN - 1] = 0;
    ch->first_ts = newts;
    ch->flags = 0;
    ch->ts = newts;
    if (ch->first_ts > newts)
      ch->first_ts = newts;
    ch->ops = NULL;
    ch->nonops = NULL;
    ch->exops = NULL;
    ch->bans = NULL;
    ch->last_notempty = timenow;
    ch->modes = 0;
#ifdef USE_CYCLE
    ch->cycled = 0;
    ch->cycops = NULL;
#endif
    add_to_list(&Channels, ch);
    add_to_hash(HASH_CHAN, ch->name, ch);
  }
  for (p = parv[4]; *p; p++)
    switch (*p)
    {
      case 'i':
        ch->modes |= CHMODE_INVITE;
        break;
      case 'l':
        ch->modes |= CHMODE_LIMIT;
        break;
      case 'k':
        ch->modes |= CHMODE_KEY;
        break;
    }

  ch->ts = newts;

  if (ch->ops == NULL && ch->nonops == NULL)
    new = -1;
  if ((parv[parc - 1][0] == '@' || parv[parc - 1][1] == '@')
      && newts < ch->ts)
    /* We now have to remove all the ops... */
    move_list(&ch->nonops, &ch->ops);
  for (p = strtok(parv[parc - 1], " "); p; p = strtok(NULL, " "))
  {
    int isop = 0;
    struct User *usr;
#ifdef USE_CYCLE
    struct User *usr1;
    struct List *node;
#endif
    if (*p == '@')
    {
      p++;
      isop++;
      hack++;
    }
    if (*p == '+' || *p == '%')
      p++;
    if (!(usr = find_user(p)))
      continue;

#ifdef USE_SMODE
    if (kick_excluded(ch, usr))
      continue;
#endif

#ifdef USE_CYCLE
    FORLIST(node, ch->cycops, struct User *, usr1)
      if (usr1 == usr)
        break;
    if (ch->cycops != NULL && isop != 0 && usr1 != usr)
    {
      isop = 0;
      send_msg(":%s MODE %s -o %s", sn, ch->name, usr->nick);
    }
    else if (ch->cycops != NULL && isop == 0 && usr1 == usr)
    {
      isop++;
      send_msg(":%s MODE %s +o %s", sn, ch->name, usr->nick);
    }
#endif
    add_to_list(isop ? &ch->ops : &ch->nonops, usr);
  }
#ifdef USE_AUTOJUPE
  if ((svr = find_server(sender)) != NULL)
  {
    if (new == 0 && hack && (timenow - svr->introduced) > MAX_SJOIN_DELAY)
    {
      place_autojupe(svr, "[Auto] Attempt to SJOIN giving ops after burst; "
                     "Probably compromised server.");
    }
#if 0
    else if (newts < 800000000)
      place_autojupe(svr, "[Auto] Attempt to SJOIN with invalid TS; "
                     "Probably compromised server.");
#endif
  }
#endif
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
  FORLISTDEL(node, nnode, ch->ops, struct User *, usr2)
    if (usr2 == usr)
      remove_from_list(&ch->ops, node);
  FORLISTDEL(node, nnode, ch->nonops, struct User *, usr2)
    if (usr2 == usr)
      remove_from_list(&ch->nonops, node);
  FORLISTDEL(node, nnode, ch->nonops, struct User *, usr2)
    if (usr2 == usr)
      remove_from_list(&ch->nonops, node);
  if (ch->nonops == NULL && ch->ops == NULL)
  {
    char *b;
    ch->modes = 0;
    FORLISTDEL(node, nnode, ch->bans, char *, b)
    {
      free(b);
      free(node);
    }
    ch->bans = NULL;
  }
}

void
m_kick(char *sender, int parc, char **parv)
{
  /* :sender KICK #channel user :reason */
  if (parc < 3)
    return;
  m_part(parv[2], parc, parv);
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
  FORLIST(node, Channels, struct Channel *, ch)
  {
    FORLISTDEL(node2, nnode2, ch->nonops, struct User *, usr2)
      if (usr2 == usr)
        remove_from_list(&ch->nonops, node2);
    FORLISTDEL(node2, nnode2, ch->ops, struct User *, usr2)
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
  struct Server *svr;
  int hack = 0;

  if ((ch = find_channel(parv[1])) == NULL)
    return;

#ifdef USE_CYCLE
  if (ch->cycops != NULL && (usr = find_user(sender)) != NULL)
  {
    char *p;
    usr2 = NULL;
    FORLIST(node, ch->cycops, struct User *, usr2)
      if (usr2 == usr)
         break;
    if (usr2 != usr)
    {
      if (parv[2][0] == 0)
        return;
      /* Unset it now! It could block the re-joiners... */
      if (parv[2][0] != '+')
        parv[2][0] = '-';
      for (p = parv[2]; *p != 0; p++)
        if (*p == '+')
          *p = '-';
      parc -= 2;
      /* Ugly kludge which will hopefully be reliable... */
      while (parc--)
        parv[2][strlen(parv[2])] = ' ';
      send_msg(":%s MODE %s %s", sn, parv[1], parv[2]);
      return;
    }
  }
#endif

  /* MODE #channel +o nick ...
   * Scan for +o or -o...
   */

  if (parc < 4)
    return;

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
        FORLISTDEL(node, nnode, ch->ops, struct User *, usr2)
          if (usr2 == usr)
            remove_from_list(&ch->ops, node);
        FORLISTDEL(node, nnode, ch->nonops, struct User *, usr2)
          if (usr2 == usr)
            remove_from_list(&ch->nonops, node);
        add_to_list(dir ? &ch->nonops : &ch->ops, usr);
      case 'v':
      case 'h':
        arg++;
        hack++;
        break;
      case 'i':
        hack++;
        ch->modes |= CHMODE_INVITE;
        break;
      case 'k':
        hack++;
        arg++;
        if (dir == 0)
          ch->modes |= CHMODE_KEY;
        else
          ch->modes &= ~CHMODE_KEY;
        break;
      case 'l':
        hack++;
        if (dir == 0)
        {
          arg++;
          ch->modes |= CHMODE_LIMIT;
        }
        else
          ch->modes &= ~CHMODE_LIMIT;
        break;
      case 'b':
        if (arg >= parc)
          continue;
        if (dir == 0)
          add_to_list(&ch->bans, strdup(parv[arg++]));
        else
        {
          char *b, *bc;
          bc = parv[arg++];
          FORLIST(node, ch->bans, char *, b)
            if (!strcasecmp(b, bc))
            {
              free(bc);
              remove_from_list(&ch->bans, node);
              break;
            }
        }
        break;
      case 'e':
      case 'I':
        arg++;
    }
#ifdef USE_AUTOJUPE
  if (hack == 0)
    return;
  if ((svr = find_server(sender)) != NULL)
    place_autojupe(svr, "[Auto] Server mode hack, probably compromised.");
  if ((usr = find_user(sender)) != NULL)
  {
    FORLIST(node, ch->ops, struct User *, usr2)
      if (usr2 == usr)
        break;
    if (node == NULL)
      FORLIST(node, ch->nonops, struct User *, usr2)
        if (usr2 == usr)
           break;
    if (node == NULL)
      place_autojupe(usr->server, "[Auto] Channel mode hack by non-member; "
                     "Probably compromised.");
  }
#endif
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
    strncpy(ch->name, name, CHANLEN - 1)[CHANLEN - 1] = '\0';
    add_to_hash(HASH_CHAN, ch->name, ch);
    add_to_list(&Channels, ch);
  }

  for (p = mode; (c = *p); p++)
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

  FORLISTDEL(node, nnode, ch->ops, struct User *, usr)
    if (kick_excluded(ch, usr))
    {
      remove_from_list(&ch->ops, node);
      continue;
    }

  FORLISTDEL(node, nnode, ch->nonops, struct User *, usr)
    if (kick_excluded(ch, usr))
    {
      remove_from_list(&ch->nonops, node);
      continue;
    }
}
#endif
