/* efserv funcs.h - Prototypes
 * $Id: funcs.h,v 1.5 2002/04/15 17:44:34 wcampbel Exp $
 */

#ifndef _FUNCS_H
#define _FUNCS_H

void process_smode(const char *chname, const char *mode);
void pick_a_hub(void);
int check_admin(struct User*, const char*, const char*);
void hash_commands(void);
void deref_admin(struct ServAdmin *a);
void deref_voteserver(struct VoteServer *v);
int match(const char *mask, const char *name);
struct VoteServer *find_server_vote(const char *name);
void destroy_server_links(struct Server *svr);
void destroy_server(struct Server *svr);
char *getmd5(struct User*);
void cleanup_channels(void);
void cleanup_jupes(void);
void cleanup_hosts(void);
void add_nickchange(char *nick, char *user, char *host);
void wipe_type_from_hash(int type, void (*cdata)(void*));
void save_channel_opdb(void);
void load_channel_opdb(void);
#ifdef USE_AUTOJUPE
void place_autojupe(struct Server *svr, const char *reason);
#endif

#ifdef __GNUC__
void fatal_error(const char *, ...)
 __attribute__((format(printf,1,2)));
int send_msg(char *msg, ...)
 __attribute__((format(printf,1,2)));
void log(const char *error, ...)
 __attribute__((format(printf,1,2)));
#else
void fatal_error(const char *, ...);
int send_msg(char *msg, ...);
void log(const char *error, ...);
#endif

#endif
