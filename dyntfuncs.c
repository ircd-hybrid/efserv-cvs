/*
 *  dyntfuncs.c: Dynamic translation routines
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
 * $Id: dyntfuncs.c,v 1.1 2001/12/10 07:04:45 a1kmm Exp $
 */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include "define.h"
#include "config.h"
#include "struct.h"
#include "dyntrans.h"
#include "utils.h"
#include <assert.h>

extern struct List *structtypes, *structinsts;
extern unsigned long reloadno;
struct List* addrhash[0x3FF];
struct List *tptrs = NULL, *queueptrs = NULL, *simplemap = NULL;

/* The generated setup function: */
void setup_dyntrans(void);

struct QueuedTranslation
{
#define AHT_QT ((void*)1)
#define AHO_QTADDR (sizeof(int) + sizeof(struct List*))
 void *type;
 struct List *n;
 void *address;
 struct StructType *t;
};

struct TPtr
{
#define AHT_TPTR ((void*)2)
#define AHO_TPOLD (sizeof(int)+sizeof(struct List*))
 void *type;
 struct List *n;
 void *old;
 void *new;
};

struct SimpleMap
{
 void *old;
 void **saveto;
};

void*
FindByAddress(void *a, void *type, int offset)
{
  void *d;
  struct List *n;
  FORLIST(n, addrhash[(((unsigned long)a)>>2) & 0x3FF], void*, d)
    if ((*(void**)d) == type && *(void**)(((char*)d)+offset) == a)
      return d;
  return NULL;
}

void
AddToAddressHash(void *a, void *type, int offset)
{
  struct List **h = addrhash +
                (((*(unsigned long*)(((char*)a) + offset))>>2) & 0x3FF);
  *(void**)a = type;
  *(struct List**)(((char*)a)+sizeof(int)) = add_to_list(h, a);
}

void
DeleteFromAddressHash(void *a, int offset)
{
  struct List *n, **h;
  char *ad = (char*)a;
  n = *(struct List**)(ad + sizeof(int));
  h = addrhash + (((*(unsigned long*)(ad + offset))>>2) & 0x3FF);
  remove_from_list(h, n);
}

void
RegisterStructType(struct StructType *st)
{
  struct StructType *stt;
  struct Field *f1, *f2;
  struct List *n, *n2, *n3, *n4;
  int ofc=0, mfc=0;
  /* This is needed as the string may become unavailable later. */
  st->name = strdup(st->name);
  st->serno = reloadno;
  FORLIST(n, st->fields, struct Field*, f1)
  {
    f1->old_offset = -1;
    f1->old_nrepeats = -1;
    ofc++;
  }
  FORLIST(n, structtypes, struct StructType*,stt)
    if (!strcmp(stt->name, st->name))
    {
      st->olen = stt->len;
      /* Time to set up the translation fields. */
      n->data = st;
      FORLIST(n2, st->fields, struct Field*, f1)
        FORLISTDEL(n3, n4, stt->fields, struct Field*, f2)
          if (!strcmp(f2->name, f1->name))
          {
            mfc++;
            if (f1->new_offset != f2->new_offset)
              st->flags |= SF_MODIFIED;
            f1->old_nrepeats = f2->nrepeats;
            f1->old_offset = f2->new_offset;
            remove_from_list(&stt->fields, n3);
            free(f2);
          }
      free(stt->name);
      FORLISTDEL(n2, n3, stt->fields, struct Field*, f1)
      {
        free(n2);
        free(f1->name);
        free(f1);
      }
      free(stt);
      if (mfc != ofc)
        st->flags |= SF_MODIFIED;
      return;
    }
  st->flags |= SF_NEW;
  add_to_list(&structtypes, st);
}

struct StructType*
FindStructType(const char *name)
{
  struct StructType *st;
  struct List *n;
  FORLIST(n, structtypes, struct StructType*, st)
    if (!strcmp(st->name, name))
      return st;
  return NULL;
}

void
SaveStructType(const char *name, struct StructType **to)
{
  struct StructType *st;
  struct List *n;
  struct SimpleMap *sm;
  FORLIST(n, structtypes, struct StructType*, st)
    if (!strcmp(st->name, name) && st->serno == reloadno)
    {
      *to = st;
      return;
    }
  sm = malloc(sizeof(*sm));
  sm->old = (void*)name;
  sm->saveto = (void**)to;
  add_to_list(&simplemap, sm);
}

void
RegisterStructInst(const char *name, const char *type, int isptr,
                   int nreps, void *data)
{
  struct List *n;
  struct StructInst *si;
  struct StructType *st;
  FORLIST(n, structinsts, struct StructInst*, si)
  {
    if (strcmp(si->name, name))
      continue;
    /* Someone has probably renamed a structure, so we have to
     * restart :( */
    if (strcmp(si->typename, type))
      restart();
    if (si->isptr != isptr)
      restart();
    if (isptr)
      *((void**)data) = *((void**)si->data);
    si->data = data;
    si->serno = reloadno;
    return;
  }
  si = malloc(sizeof(*si));
  si->name = strdup(name);
  si->typename = strdup(type);
  si->isptr = isptr ? 1 : 0;
  si->data = data;
  si->serno = reloadno;
  si->nreps = nreps;
  if ((st = FindStructType(type)) != NULL)
    memset(data, 0, (isptr ? sizeof(void*) : st->len) * nreps);
  add_to_list(&structinsts, si);
}

void
QueueTranslate(void *address, struct StructType *t, void **ptrto)
{
  struct QueuedTranslation *qt;
  struct TPtr *tp;
  char c;
  if (address == NULL)
    return;
  if (t == NULL)
    restart();
  /* Check it isn't already translated... */
  if (ptrto != NULL)
  {
    if ((tp = (struct TPtr*)FindByAddress(address, AHT_TPTR, AHO_TPOLD)))
    {
      *ptrto = tp->new;
      return;
    }
  }
  /* If it isn't already queued for translation, queue it. */
  if (FindByAddress(address, AHT_QT, AHO_QTADDR) == NULL)
  {
    char c;
    qt = malloc(sizeof(*qt));
    qt->address = address;
    qt->t = t;
    /* Do a test to catch bugs... */
    c = *((char*)qt->address);
    AddToAddressHash(qt, AHT_QT, AHO_QTADDR);
    add_to_list(&queueptrs, qt);
  }
  else
  {
    c = ((char*)address)[0];
    ((char*)address)[0] = c;
  }
  /* It doesn't matter if we schedule the same simple mapping
   * twice.
   */
  if (ptrto != NULL)
  {
    struct SimpleMap *sm = malloc(sizeof(*sm));
    sm->saveto = ptrto;
    sm->old = address;
    add_to_list(&simplemap, sm);
  }
}

void
Translate(struct QueuedTranslation *qt, void *addr)
{
  struct List *n;
  struct Field *f;
  struct TPtr *tp;
  char *ns, *os = (char*)qt->address;
  int ch = qt->t->flags & (SF_NEW|SF_MODIFIED);
  if (ch && addr == NULL)
    ns = (char *)malloc(qt->t->len);
  else if (addr == NULL)
    ns = (char*)qt->address;
  else
    ns = (char*)addr;
  FORLIST(n, qt->t->fields, struct Field*, f)
  {
    if (ch && f->old_offset != -1)
    {
      memcpy(ns + f->new_offset, os + f->old_offset,
             f->len * ((f->nrepeats > f->old_nrepeats)?
               f->old_nrepeats : f->nrepeats));
      if (f->nrepeats > f->old_nrepeats)
        memset(ns + f->new_offset + f->old_nrepeats*f->len, 0,
               f->len * (f->nrepeats - f->old_nrepeats));
    }
    else if (ch && f->init_func)
    {
      int i;
      for (i=0; i<f->nrepeats; i++)
        f->init_func(ns, ns + f->new_offset + f->len * i, f->len);
    }
    else if (ch && f->flags & SFF_NEEDRESTART)
      restart();
    else if (ch)
      memset(ns + f->new_offset, 0, f->len);
    /* Record the field's new address for later translations...
     * We now always have to do this :( because otherwise we forget
     * we have already tried to translate it.
     * Note that when f->old_offset was 0, we could be using the same
     * address for the start of the structure and the old first
     * field, so we can't add it.
     */
    if (f->old_offset > 0 &&
        FindByAddress(os + f->old_offset, AHT_TPTR, AHO_TPOLD) == NULL)
    {
      tp = malloc(sizeof(*tp));
      tp->new = ns + f->new_offset;
      tp->old = os + f->old_offset;
      AddToAddressHash(tp, AHT_TPTR, AHO_TPOLD);
      add_to_list(&tptrs, tp);
    }
    /* Okay, now having translated the field, see if it points to
     * anything that needs translating itself... */
    if (f->flags & SFF_ISPOLYPOINTER)
      continue;
    if (f->flags & SFF_ISPOINTER && f->type == FIELD_INTTYPE)
    {
      /* We do this just in case it is something like a char * into
       * a structure.
       */
      struct SimpleMap *sm = malloc(sizeof(*sm));
      sm->old = *(void**)(ns + f->new_offset);
      sm->saveto = (void**)(ns + f->new_offset);
      add_to_list(&simplemap, sm);
      continue;
    }
    if (f->type != FIELD_STRUCT)
      continue;
    if (f->flags & SFF_ISPOINTER)
    {
      QueueTranslate(*(void**)(ns + f->new_offset), f->stype,
                     (void**)(ns + f->new_offset));
    }
    else
    {
      struct QueuedTranslation qtn;
      qtn.address = ns + f->new_offset;
      qtn.t = f->stype;
      Translate(&qtn, ns + f->new_offset);
    }
  }
  if (FindByAddress(os, AHT_TPTR, AHO_TPOLD) == NULL)
  {
    tp = malloc(sizeof(*tp));
    tp->new = ns;
    tp->old = os;
    AddToAddressHash(tp, AHT_TPTR, AHO_TPOLD);
    add_to_list(&tptrs, tp);
  }
  if (ch && addr == NULL)
    free(qt->address);
}

/* A kludge for the void* pointers in linked lists... */
void
TranslateList(struct List *l, const char *t)
{
  struct List *n;
  void *p;
  struct StructType *st = FindStructType(t);
  if (st == NULL)
    return;
  FORLIST(n, l, void*, p)
    QueueTranslate(p, st, &n->data);
}

void
TranslateAll(void)
{
  struct List *n;
  struct StructInst *si;
  struct StructType *st;
  struct QueuedTranslation *qt;
  struct SimpleMap *sm;
  struct TPtr *tp;
  tptrs = NULL;
  queueptrs = NULL;
  simplemap = NULL;
  /* Step 1: Setup all the types and globals... */
  setup_dyntrans();
  while (simplemap)
  {
    n = simplemap;
    sm = n->data;
    if ((st = FindStructType((char*)sm->old)))
      *sm->saveto = st;
    simplemap = simplemap->next;
    free(n);
    free(sm);
  }
  TranslateList(Channels, "Channel");
  TranslateList(serv_admins, "ServAdmin");
  TranslateList(VoteServers, "VoteServer");
  TranslateList(Hubs, "Hub");
  TranslateList(JupeExempts, "JExempt");
  TranslateList(Servers, "Server");
  TranslateList(Users, "User");
  TranslateList(Hosts, "Host");
  FORLIST(n, structinsts, struct StructInst*, si)
  {
    int i;
    /* XXX - should we bother to free here? */
    if (si->serno != reloadno)
      continue;
    if ((st = FindStructType(si->typename)) == NULL)
    {
      restart();
      exit(0);
    }
    if (si->data == NULL ||
        (si->isptr && si->nreps == 0 && *((void**)si->data) == NULL))
      continue;
    for (i=0; i<si->nreps; i++)
    {
      if (si->isptr)
        QueueTranslate(((void**)si->data)[i], st, ((void**)si->data) + i);
      else
        QueueTranslate((void*)(((char*)si->data) + i*st->olen), st, NULL);
    }
  }
  /* Okay, we have all those globals on the queue. Go through the
   * queue, and translate structures, queueing new ones as we come
   * to them... */
  while(queueptrs)
  {
    n = queueptrs;
    qt = (struct QueuedTranslation*)queueptrs->data;
    Translate(qt, NULL);
    DeleteFromAddressHash(qt, AHO_QTADDR);
    free(qt);
    remove_from_list(&queueptrs, n);
  }
  while(simplemap)
  {
    n = simplemap;
    sm = (struct SimpleMap*)simplemap->data;
    /* If no translation found, no translation is probably needed. */
    if ((tp = (struct TPtr*)FindByAddress(sm->old, AHT_TPTR, AHO_TPOLD))
         != NULL)
    {
      *sm->saveto = tp->new;
    }
    free(sm);
    remove_from_list(&simplemap, n);
  }
  while(tptrs)
  {
    n = tptrs;
    tptrs = tptrs->next;
    free(((struct TPtr*)n->data)->n);
    free(n->data);
    free(n);
  }
  memset(addrhash, 0, sizeof(addrhash));
}
