/*
 *  dyntrans.h: Dynamic translation includes
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
 * $Id: dyntrans.h,v 1.2 2002/04/15 17:44:34 wcampbel Exp $
 */

#ifndef _DYNTRANS_H
#define _DYNTRANS_H

struct StructType
{
 unsigned long serno;
 char *name;
 int len, flags;
 struct List *node, *fields;
 /* Old length needed for array translation: */
 int olen;
};

struct Field
{
 char *name;
 int type, len, flags;
 struct StructType *stype;
 int nrepeats, old_nrepeats;
 /* Is passed the struct, the field and the length. */
 void (*init_func)(void*, void*, int);
 /* Below this is only filled for translations... */
 int old_offset, new_offset; /* old_offset = -1 if new. */
};

struct StructInst
{
 char *name, *typename;
 int isptr, nreps;
 void *data;
 unsigned long serno;
};

enum
{
 FIELD_INTTYPE,
 FIELD_STRUCT,
 FIELD_FUNCPTR /* Not implemented yet */
};

#define SFF_ISPOINTER 1
#define SFF_POINTEDTO 2
#define SFF_ISPOLYPOINTER 4 /* These are harder to handle. */
#define SFF_NEEDRESTART 8 /* Must restart if this didn't exist before. */

#define SF_MODIFIED 1
#define SF_NEW 2

void restart(void);
void RegisterStructType(struct StructType *st);
struct StructType *FindStructType(const char *name);
void SaveStructType(const char *name, struct StructType **to);
void RegisterStructInst(const char *name, const char *type, int isptr,
                        int nrep, void *data);

#endif
