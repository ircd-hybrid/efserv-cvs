/*
 *  utils.c: Some general utility functions.
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

#include <ctype.h>
#include <stdlib.h>
#include <strings.h>
#include "efserv.h"


#define HASHSIZE 0x1000

struct HashEntry *hash[0x1000];

unsigned long
hash_text(const char *txt)
{
 unsigned int h = 0;
 while (*txt)
 {
  h = (h << 4) - (h + (unsigned char)tolower(*txt++));
 }
 return(h & (HASHSIZE - 1));
}

void
init_hash(void)
{
 memset(hash, 0, sizeof(hash));
}

void
add_to_hash(int type, char *name, void *data)
{
 int hv = hash_text(name);
 struct HashEntry *he;
 he = malloc(sizeof(*he));
 he->next = hash[hv];
 hash[hv] = he;
 he->type = type;
 he->data = data;
 he->name = name;
}

void*
find_in_hash(int type, char *name)
{
 struct HashEntry *he;
 int hv = hash_text(name);
 for (he=hash[hv]; he; he=he->next)
  if (he->type == type && !strcasecmp(name, he->name))
   return he->data;
 return NULL;
}

void
remove_from_hash(int type, char *name)
{
 struct HashEntry *he, **phe;
 int hv = hash_text(name);
 for (he=hash[hv], phe=&hash[hv]; he; phe=&(he->next), he=he->next)
  if (he->type == type && !strcasecmp(name, he->name))
  {
   free(he);
   return;
  }
}

void*
add_to_list(struct List **list, void *data)
{
 struct List *nlist = malloc(sizeof(*nlist));
 nlist->next = *list;
 *list = nlist;
 nlist->data = data;
 return nlist;
}

void
remove_from_list(struct List **list, struct List *node)
{
 if (node->prev)
  node->prev->next = node->next;
 else
  *list = node->next;
 if (node->next)
  node->next->prev = node->prev;
 free(node);
}
