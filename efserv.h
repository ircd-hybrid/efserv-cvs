/*
 *  efserv.h: The main header file for efserv.
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

#define NICKLEN 20
#define HOSTLEN 40
#define USERLEN 40
#define SERVLEN 80

struct Command
{
 char *name;
 void (*func)(char *sender, int argc, char **parv);
};

struct User
{
 char nick[NICKLEN], user[USERLEN], host[HOSTLEN];
 unsigned long flags;
 struct List *node;
 struct Server *server;
};

struct Server
{
 char name[SERVLEN];
 unsigned long flags;
 struct List *node;
 struct Server *uplink;
};

enum
{
 HASH_COMMAND,
 HASH_SERVER,
 HASH_USER,
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

extern struct Command Commands[];
extern struct Server *first_server;
extern char *server_name, *server_pass, *server_host, *sn;
extern int port;

void add_to_hash(int type, char *name, void *data);
void remove_from_hash(int type, char *name);
void* find_in_hash(int type, char *name);
void fatal_error(const char *error, ...);
void* add_to_list(struct List **list, void *data);
void remove_from_list(struct List **list, struct List *node);
int send_msg(char *msg, ...);


#define find_server(name) (struct Server*)find_in_hash(HASH_SERVER,name)
#define find_user(name) (struct User*)find_in_hash(HASH_USER,name)

#define UFLAG_ADMIN           0x00000001
#define UFLAG_OPER            0x00000002

#define IsAdmin(x) (x->flags & UFLAG_ADMIN)
#define IsOper(x)  (x->flags & UFLAG_OPER)

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
