/*
 *  utils.h: Defines the hash and linked list tools.
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
 * $Id: utils.h,v 1.1 2001/05/31 08:52:08 a1kmm Exp $
 */

enum
{
 HASH_COMMAND,
 HASH_SERVER,
 HASH_USER,
 HASH_CHAN,
 HASH_HOST,
};

struct HashEntry
{
 char *name;
 int type;
 void *data;
 struct HashEntry *next;
};

struct List
{
 struct List *next, *prev;
 void *data;
};

/* Loop through a linked list... */
#define FORLIST(node,list,type,var) \
 for(node=list,var=node?(type)node->data:NULL;\
     node;\
     node=node->next,var=node?(type)node->data:NULL\
    )
/* Use this form when items could be deleted from the list... */
#define FORLISTDEL(node,nnode,list,type,var) \
 for(node=list,nnode=node?node->next:NULL,var=node?(type)node->data:NULL;\
     node;\
     node=nnode,nnode=node?node->next:NULL,var=node?(type)node->data:NULL\
    )

#define find_server(name) (struct Server*)find_in_hash(HASH_SERVER,name)
#define find_user(name) (struct User*)find_in_hash(HASH_USER,name)
#define find_channel(name) (struct Channel*)find_in_hash(HASH_CHAN,name)
#define find_host(name) (struct Host*)find_in_hash(HASH_HOST,name)
