/*
 *  utils.c: Some general utility functions.
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
 * $Id: utils.c,v 1.8 2001/11/11 22:13:52 wcampbel Exp $
 */

#include <ctype.h>
#include <stdlib.h>
#include <strings.h>
#include <assert.h>
#include "config.h"
#include "define.h"
#include "utils.h"

#define HASHSIZE 0x1000

extern struct HashEntry *hash[0x1000];

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
find_in_hash(int type, const char *name)
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
   *phe = he->next;
   free(he);
   return;
  }
}

void
wipe_type_from_hash(int type, void (*cdata)(void*))
{
 int hv;
 struct HashEntry *he, **phe;
 for (hv=0; hv<HASHSIZE; hv++)
  for (he=hash[hv],phe=&hash[hv]; he; he=*phe)
  {
   if (he->type == type)
   {
    *phe = he->next;
    if (cdata)
     cdata(he->data);
    free(he);
   } else
    phe = &he->next;
  }
}

struct List*
add_to_list(struct List **list, void *data)
{
 struct List *nlist = malloc(sizeof(*nlist));
 nlist->next = *list;
 if (nlist->next)
  nlist->next->prev = nlist;
 nlist->prev = NULL;
 *list = nlist;
 assert(data);
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

void
move_list(struct List **list1, struct List **list2)
{
 struct List *node;
 for (node=*list1; node && node->next; node=node->next)
  ;
 if (node == NULL)
  *list1 = *list2;
 else
 {
  node->next = *list2;
  (*list2)->prev = node;
 }
 *list2 = NULL;
 return;
}

struct List*
add_to_list_before(struct List **list, struct List *before, void *d)
{
 struct List *nlist;
 nlist = malloc(sizeof(*nlist));
 nlist->data = d;
 if (*list == NULL)
 {
  nlist->next = NULL;
  nlist->prev = NULL;
  nlist->data = d;
  *list = nlist;
  return nlist;
 }
 if (before == NULL)
 {
  struct List *node;
  for (node=*list; node->next; node=node->next)
   ;
  node->next = nlist;
  nlist->next = NULL;
  nlist->prev = node;
  return nlist;
 }
 nlist->prev = before->prev;
 nlist->next = before;
 before->prev = nlist;
 if (nlist->prev == NULL)
  *list = nlist;
 return nlist; 
}
