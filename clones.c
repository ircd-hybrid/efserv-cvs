/*
 *  clones.c: Clone detection and reporting.
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
 * $Id: clones.c,v 1.9 2001/12/02 03:59:38 a1kmm Exp $
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
cleanup_hosts(void)
{
  struct List *node, *nnode;
  struct Host *h;
  FORLISTDEL(node,nnode,Hosts,struct Host*,h)
  {
    int rtime = h->full ? UHOST_NICKCHANGE_RATE : HOST_NICKCHANGE_RATE;
    h->rate -= (timenow-h->last_recalc)/rtime;
    h->last_recalc = timenow - (timenow-h->last_recalc)%rtime;
    if (h->rate < 0)
      h->rate = 0;
    if (h->count == 0 && h->rate == 0)
    {
      remove_from_hash(HASH_HOST, h->host);
      remove_from_list(&Hosts, node);
      free(h);
    }
  }
}

void
report_cloner(struct Host *h, char *term, char *onick)
{
  struct List *node;
  struct User *usr;

  if (h->warned == 0)
  {
    FILE *warning_file = fopen(WARNINGFILE, "r");
    char buffer[512];
    h->warned = 1;
    send_msg(":%s NOTICE %s :You have been detected as a %s offender.",
             sn, onick, term);
    if (warning_file == NULL)
      return;
    while (fgets(buffer, 512, warning_file))
      send_msg(":%s NOTICE %s :%s", sn, onick, buffer);
    fclose(warning_file);
    return;
  }

  if ((timenow - h->last_report) < 30)
    return;

  h->last_report = timenow;
  FORLIST(node,Monitors,struct User*,usr)

  if (h->full)
    send_msg(":%s NOTICE %s :%s ON %s", server_name, usr->nick,
	     term, h->host);
  else
    send_msg(":%s NOTICE %s :%s ON *@%s", server_name, usr->nick,
	     term, h->host);
}

void
add_cloner(char *nick, char *user, char *host)
{
  struct Host *h1, *h2;
  char uah[HOSTLEN+USERLEN+1];

  strncpy(uah, user, USERLEN-1)
    [USERLEN-1] = '\0';

  strcat(uah, "@");
  strncat(uah, host, HOSTLEN-1);

  if ((h1 = find_host(uah)) == NULL)
  {
    h1 = malloc(sizeof(*h1));
    strncpy(h1->host, uah, HOSTLEN+USERLEN)
      [HOSTLEN+USERLEN] = '\0';
    h1->warned = 0;
    h1->count = h1->rate = 0;
    h1->full = 1;
    h1->last_recalc = timenow;
    h1->last_report = 0;
    add_to_hash(HASH_HOST, h1->host, h1);
    add_to_list(&Hosts, h1);
  }

  if ((h2 = find_host(host)) == NULL)
  {
    h2 = malloc(sizeof(*h2));
    strncpy(h2->host, host, HOSTLEN-1)
      [HOSTLEN-1] = '\0';
    h2->count = h2->rate = h2->full = 0;
    h2->last_recalc = timenow;
    h2->last_report = 0;
    h2->warned = 0;
    add_to_hash(HASH_HOST, h2->host, h2);
    add_to_list(&Hosts, h2);
  }
  h1->rate -= (timenow-h1->last_recalc)/UHOST_NICKCHANGE_RATE;
  h1->last_recalc = timenow -
                    (timenow-h1->last_recalc)%UHOST_NICKCHANGE_RATE;
  if (h1->rate < 0)
    h1->rate = 0;
  h2->rate -= (timenow-h2->last_recalc)/HOST_NICKCHANGE_RATE;
  h2->last_recalc = timenow -
                    (timenow-h2->last_recalc)%HOST_NICKCHANGE_RATE;
  if (h2->rate < 0)
    h2->rate = 0;
  h1->count++;
  h1->rate++;
  h2->count++;
  h2->rate++;
  if (h2->count > MAXCLONES_HOST)
  {
    report_cloner(h2, "CLONES", nick);
    return;
  }
  if (h1->count > MAXCLONES_UHOST)
  {
    report_cloner(h1, "CLONES", nick);
    return;
  }
  if (h2->rate > MAXNICKRATE_HOST)
  {
    report_cloner(h2, "NICKFLOODER", nick);
    return;
  }
  if (h1->rate > MAXNICKRATE_UHOST)
  {
    report_cloner(h1, "NICKFLOODER", nick);
    return;
  }
}

void
add_nickchange(char *nick, char *user, char *host)
{
  struct Host *h1, *h2;
  char uah[HOSTLEN+USERLEN+1];

  strncpy(uah, user, USERLEN-1)
    [USERLEN-1] = '\0';

  strcat(uah, "@");
  strncat(uah, host, HOSTLEN-1);

  if ((h1 = find_host(uah)) == NULL)
    return;
  if ((h2 = find_host(host)) == NULL)
    return;
  h1->rate -= (timenow-h1->last_recalc)/UHOST_NICKCHANGE_RATE;
  h1->last_recalc = timenow -
                    (timenow-h1->last_recalc)%UHOST_NICKCHANGE_RATE;
  if (h1->rate < 0)
    h1->rate = 0;
  h2->rate -= (timenow-h2->last_recalc)/HOST_NICKCHANGE_RATE;
  h2->last_recalc = timenow -
                    (timenow-h2->last_recalc)%HOST_NICKCHANGE_RATE;
  if (h2->rate < 0)
    h2->rate = 0;
  h1->rate++;
  h2->rate++;
  if (h2->rate > MAXNICKRATE_HOST)
  {
    report_cloner(h2, "NICKFLOODER", nick);
    return;
  }
  if (h1->rate > MAXNICKRATE_UHOST)
  {
    report_cloner(h1, "NICKFLOODER", nick);
    return;
  }
}

void
remove_cloner(char *user, char *host)
{
  struct Host *h1, *h2;
  char uah[HOSTLEN+USERLEN+1];

  strncpy(uah, user, USERLEN-1)
    [USERLEN-1] = '\0';
  strcat(uah, "@");
  strncat(uah, host, HOSTLEN-1);
  if ((h1 = find_host(uah)) == NULL || (h2 = find_host(host)) == NULL)
    return;
  h1->count--;
  h2->count--;
}
