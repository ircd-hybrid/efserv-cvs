/*
 *  struct.h: Defines many of the structures.
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
 * $Id: struct.h,v 1.1 2001/05/31 08:52:08 a1kmm Exp $
 */

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

struct Hub
{
 char *host;
 char *pass;
 int port;
};

extern struct Command Commands[];
extern struct Server *first_server;
extern struct List *Servers, *Users, *Channels, *Hosts, *Monitors,
                   *serv_admins, *VoteServers, *Hubs, *HKeywords;
extern struct Server *first_server;
extern char *server_name, *server_pass, *server_host, *sn;
extern int port;
extern int reload_module, die;
extern time_t timenow, channel_record_time;
extern int server_count, minimum_servers;

void add_to_hash(int type, char *name, void *data);
void remove_from_hash(int type, char *name);
void* find_in_hash(int type, const char *name);
struct List* add_to_list(struct List **list, void *data);
struct List* add_to_list_before(struct List **list, struct List *before,
                                void *data);
void move_list(struct List **dest, struct List **src);
void remove_from_list(struct List **list, struct List *node);
