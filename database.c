/*
 *  database.c: The efserv database controller.
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
 * $Id: database.c,v 1.3 2001/07/30 06:51:04 a1kmm Exp $
 */
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "define.h"
#include "struct.h"
#include "utils.h"
#include "funcs.h"

#define EFCDB_MAJOR_VERSION 0
#define EFCDB_MINOR_VERSION 1

struct FileHeader
{
 char check[5];
 unsigned long v_maj, v_min;
 time_t record_time;
};

struct ChannelHeader
{
 char name[CHANLEN];
 unsigned long entries;
};

void
save_channel_opdb(void)
{
 struct List *node, *node2;
 struct Channel *ch;
 struct ChanopUser *cou;
 FILE *efcdb_file;
 struct FileHeader fh =
  {{'E','F','C','D','B'}, EFCDB_MAJOR_VERSION, EFCDB_MINOR_VERSION, 0};
 struct ChannelHeader chh;
 int exopc;
 if ((efcdb_file = fopen(CHANNEL_DB, "w")) == NULL)
 {
  log("[CHANDB] Could not open the channel database for write.\n");
  return;
 }
 fh.record_time = channel_record_time;
 fwrite(&fh, sizeof(fh), 1, efcdb_file);
 FORLIST(node,Channels,struct Channel*,ch)
  if (ch->exops)
  {
   exopc = 0;
   FORLIST(node2,ch->exops,struct ChanopUser *,cou)
    exopc++;
   strncpy(chh.name, ch->name, CHANLEN-1)[CHANLEN-1] = 0;
   chh.entries = exopc;
   fwrite(&chh, sizeof(chh), 1, efcdb_file);
   FORLIST(node2,ch->exops,struct ChanopUser *,cou)
    fwrite(cou, sizeof(*cou), 1, efcdb_file);
  }
 fclose(efcdb_file);
}

void
load_channel_opdb(void)
{
 FILE *efcdb_file;
 struct FileHeader fh;
 struct ChannelHeader chh;
 struct Channel *ch;
 struct ChanopUser *cou;
 int i;
 if ((efcdb_file = fopen(CHANNEL_DB, "r")) == NULL)
 {
  log("[CHANDB] Could not open the channel database for read.\n");
  /* Set the channel record time, so we don't go doing stuff on too
   * little data... */
  channel_record_time = timenow;
  return;
 }
 /* Check the header. Fatal errors are bad, but if we cannot be
  * confident it is the right file, we do not want to risk overwriting
  * it... */
 if ((fread(&fh, sizeof(fh), 1, efcdb_file) <= 0) ||
     memcmp(fh.check, "EFCDB", 5))
  fatal_error(
   "[CHANDB] Channel record database is corrupt or wrong type.\n");
 if (fh.v_maj != EFCDB_MAJOR_VERSION || fh.v_min != EFCDB_MINOR_VERSION)
  fatal_error(
   "[CHANDB] The database found is for version %lu.%lu, need %d.%d\n",
   fh.v_maj, fh.v_min, EFCDB_MAJOR_VERSION, EFCDB_MINOR_VERSION);
 channel_record_time = fh.record_time;
 /* Okay, header is okay, now read channel headers followed by
  * records...*/
 while (fread(&chh, sizeof(chh), 1, efcdb_file) > 0)
 {
  chh.entries = 1;
  ch = malloc(sizeof(*ch));
  strncpy(ch->name, chh.name, CHANLEN-1)[CHANLEN-1] = 0;
  ch->flags = 0;
  ch->ts = -1;
  ch->ops = ch->nonops = ch->exops = NULL;
#ifdef USE_CYCLE
  ch->cycops = NULL;
#endif
  /* *sigh* hope services stays up for longer than 2 hour average... */
  ch->last_notempty = timenow;
  add_to_list(&Channels, ch);
  add_to_hash(HASH_CHAN, ch->name, ch);
  for (i=chh.entries; i>0; i--)
  {
   cou = malloc(sizeof(*cou));
   if (fread(cou, sizeof(*cou), 1, efcdb_file) <= 0)
   {
    log("[CHANDB] Warning: Channel database is corrupt.\n");
    free(cou);
    return;
   }
   add_to_list(&ch->exops, cou);
  }
 }
}
