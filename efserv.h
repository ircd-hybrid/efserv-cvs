/*
 *  efserv.h: The main header file for efserv.
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
 * $Id: efserv.h,v 1.6 2001/05/29 09:29:44 a1kmm Exp $
 */

#include <time.h>

#define NICKLEN 20
#define HOSTLEN 64
#define USERLEN 40
#define SERVLEN 80
#define CHANLEN 255

#define MAXCLONES_UHOST 4
#define MAXCLONES_HOST 6

#define JUPE_EXPIRE_TIME 45*60

#define NETNAME "test net"

#define VERSION "pre0.1-test"

struct Command
{
 char *name;
 void (*func)(char *sender, int argc, char **parv);
};

struct User
{
 struct Server *server;
 char nick[NICKLEN], user[USERLEN], host[HOSTLEN];
 unsigned long flags, caps;
 struct ServAdmin *sa;
 struct List *node, *monnode;
};

struct Server
{
 char name[SERVLEN];
 unsigned long flags;
 struct List *node;
 struct Server *uplink;
 struct Jupe *jupe;
};

struct Channel
{
 char name[CHANLEN];
 int flags;
 time_t ts;
 struct List *ops, *nonops, *exops;
 time_t last_notempty;
};

struct ChanopUser
{
 time_t last_opped;
 unsigned long slices;
 char uhost_md5[16];
};

struct AdminHost
{
 char *server, *host, *user;
};

struct ServAdmin
{
 char *name, *pass;
 int refcount, caps;
 struct List *hosts;
};

struct VoteServer
{
 struct List *names;
 int flags;
 int refcount;
};

struct Host
{
 char host[HOSTLEN+USERLEN+1];
 int count, rate, full;
 time_t last_recalc, last_report;
};

struct JupeVote
{
 struct VoteServer *vs;
 struct ServAdmin *vsa;
 int score;
};

struct Jupe
{
 char reason[255];
 int flags;
 /* Note: 15 activates an inactive JUPE, 0 deactivates an active JUPE. */
 int score;
 struct List *jupevotes;
 time_t last_active;
};

enum
{
 HASH_COMMAND,
 HASH_SERVER,
 HASH_USER,
 HASH_CHAN,
 HASH_HOST,
};

enum
{
 ALEVEL_ADMIN,
 ALEVEL_OPER,
 ALEVEL_SERVADMIN,
 ALEVEL_ANY,
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
extern struct List *Servers, *Users, *Channels, *Hosts, *Monitors,
                   *serv_admins, *VoteServers;
extern struct Server *first_server;
extern char *server_name, *server_pass, *server_host, *sn;
extern int port;
extern int reload_module, die;
extern time_t timenow;

void add_to_hash(int type, char *name, void *data);
void remove_from_hash(int type, char *name);
void* find_in_hash(int type, const char *name);
void fatal_error(const char *error, ...);
struct List* add_to_list(struct List **list, void *data);
struct List* add_to_list_before(struct List **list, struct List *before,
                                void *data);
void move_list(struct List **dest, struct List **src);
void remove_from_list(struct List **list, struct List *node);
void process_smode(const char *chname, const char *mode);
int send_msg(char *msg, ...);
void write_dynamic_config(void);
int check_admin(struct User*, const char*, const char*);
void hash_commands(void);
void deref_admin(struct ServAdmin *a);
void deref_voteserver(struct VoteServer *v);
int match(const char *mask, const char *name);
struct VoteServer *find_server_vote(const char *name);
void destroy_server_links(struct Server *svr);
char *getmd5(struct User*);
void cleanup_channels(void);
void cleanup_jupes(void);
void cleanup_hosts(void);

#define find_server(name) (struct Server*)find_in_hash(HASH_SERVER,name)
#define find_user(name) (struct User*)find_in_hash(HASH_USER,name)
#define find_channel(name) (struct Channel*)find_in_hash(HASH_CHAN,name)
#define find_host(name) (struct Host*)find_in_hash(HASH_HOST,name)

#define UFLAG_ADMIN           0x00000001
#define UFLAG_OPER            0x00000002
#define UFLAG_SERVADMIN       0x00000004

#define IsAdmin(x) (x->flags & UFLAG_ADMIN)
#define IsOper(x)  (x->flags & UFLAG_OPER)
#define IsServAdmin(x)  (x->flags & UFLAG_SERVADMIN)

#define CHFLAG_BANNED         0x00000001
#define CHFLAG_OPERONLY       0x00000002
#define CHFLAG_ADMINONLY      0x00000004
#define SMODES CHFLAG_BANNED | CHFLAG_OPERONLY | CHFLAG_ADMINONLY

#define IsBanChan(x) (x->flags & CHFLAG_BANNED)
#define IsOperChan(x) (x->flags & CHFLAG_OPERONLY)
#define IsAdminChan(x) (x->flags & CHFLAG_ADMINONLY)
#define HasSMODES(x) (x->flags & (SMODES))

#define SERVFLAG_JUPED 1
#define SERVFLAG_ACTIVE 2

#define IsJuped(x) (x->flags & SERVFLAG_JUPED)

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

extern struct yystype
{
 unsigned long number;
 char *string;
} yylval;
#define YYSTYPE struct yystype
