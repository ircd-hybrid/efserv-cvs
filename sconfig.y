/*
 *  sconfig.y: The efserv config file parser.
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
 * $Id: sconfig.y,v 1.9 2001/11/11 22:13:52 wcampbel Exp $
 */

%{
  #include <stdlib.h>
  #include <string.h>
  #include <stdio.h>
  #include <time.h>
  #include "config.h"
  #include "define.h"
  #include "struct.h"
  #include "utils.h"
  #include "funcs.h"
  #include "conf.h"
  int yylex();
  void yyerror();
  struct ServAdmin *ad;
  struct AdminHost *ah;
  struct VoteServer *vs;
  struct JExempt *je;
  struct Hub *hub;
  extern int lineno;
  #define DupString(x,y) if(x) free(x); x = strdup(y);
%}

%token NUMBER
%token STRING
%token QSTRING
%token GENERAL
%token NAME
%token PORT
%token HOST
%token NICK
%token PASS
%token ADMIN
%token USER
%token AUTH
%token SERVER
%token HUB
%token MINSERVS
%token NOREOP
%token YES
%token NO
%token SUNJUPE
%token JEXEMPT
%token TYPE
%token AUTO
%token MANUAL
%token ALL
%%

conf: conf conf_item | conf_item;

conf_item: general_block |
           admin_block |
           hub_block |
           server_block |
           jexempt_block |
           noreop_block;

general_block: GENERAL '{' general_items '}' ';';

general_items: general_items general_item | general_item;
general_item: general_name |
              general_nick |
              general_minservers;

general_name: NAME '=' QSTRING
{
 DupString(server_name, yylval.string);
} ';' ;

general_nick: NICK '=' QSTRING
{
 DupString(sn, yylval.string);
} ';' ;

general_minservers: MINSERVS '=' NUMBER
{
 minimum_servers = yylval.number;
} ';';

hub_block: HUB '{'
{
 hub = malloc(sizeof(*hub));
 hub->host = NULL;
 hub->pass = NULL;
 hub->port = 0;
} hub_items '}'
{
 if (hub->host == NULL)
 {
  log("[Config] Hub block needs host, line %d.\n", lineno);
  if (hub->pass)
   free(hub->pass);
  free(hub);
 } else if (hub->pass == NULL)
 {
  free(hub->host);
  free(hub);
  log("[Config] Hub block needs password, line %d.\n", lineno);
 } else if (hub->port <= 0 || hub->port > 0xFFFF)
 {
  free(hub->host);
  free(hub->pass);
  free(hub);
  log("[Config] Hub block needs valid port number, line %d.\n", lineno);
 } else
 {
  add_to_list(&Hubs, hub);
 }
}';';
hub_items: hub_items hub_item | hub_item;
hub_item: hub_host | hub_pass | hub_port;
hub_host: HOST '=' QSTRING
{
 DupString(hub->host, yylval.string);
} ';';
hub_pass: PASS '=' QSTRING
{
 DupString(hub->pass, yylval.string);
} ';';
hub_port: PORT '=' NUMBER
{
 hub->port = yylval.number;
} ';';

admin_block: ADMIN '{'
{
 ad = malloc(sizeof(*ad));
 ah = malloc(sizeof(*ah));
 ah->user = NULL;
 ah->host = NULL;
 ah->server = NULL;
 ad->name = NULL;
 ad->pass = NULL;
 ad->hosts = NULL;
 ad->caps = 0;
 ad->refcount = 1;
} admin_items '}'
{
 if (ad->name == NULL || ad->pass == NULL || ad->hosts == NULL)
 {
  log("[Config] Admin block needs name, password and at least one "
      "auth{} block, line %d.\n", lineno);
  deref_admin(ad);
 }
 else
 {
  add_to_list(&serv_admins, ad);
 }
 free(ah);
}';';

admin_items: admin_items admin_item | admin_item;
admin_item: admin_name | admin_pass | admin_auth | admin_sunjupe;

admin_name: NAME '=' QSTRING
{
 DupString(ad->name, yylval.string);
} ';';

admin_pass: PASS '=' QSTRING
{
 DupString(ad->pass, yylval.string);
} ';';

admin_sunjupe: SUNJUPE '=' YES ';'
{
 ad->caps |= SACAP_SUNJUPE;
} |
SUNJUPE '=' NO ';'
{
 ad->caps &= ~SACAP_SUNJUPE;
};

admin_auth: AUTH '{' auth_items '}'
{
 if (ah->server && ah->user && ah->host)
 {
  add_to_list(&ad->hosts, ah);
  ah = malloc(sizeof(*ah));
  ah->host = NULL;
  ah->user = NULL;
  ah->server = NULL;
 } else
 {
  log(
   "[Config] Admin auth block needs server, user and host entries, "
   "line %d.\n", lineno);
  if (ah->user)
   free(ah->user);
  ah->user = NULL;
  if (ah->host)
   free(ah->host);
  ah->host = NULL;
  if (ah->server)
   free(ah->server);
  ah->server = NULL;
 }
}';'

auth_items: auth_items auth_item | auth_item;
auth_item: auth_server | auth_user | auth_host;

auth_server: SERVER '=' QSTRING ';'
{
 DupString(ah->server, yylval.string);
};

auth_user: USER '=' QSTRING ';'
{
 DupString(ah->user, yylval.string);
};

auth_host: HOST '=' QSTRING ';'
{
 DupString(ah->host, yylval.string);
};

server_block: SERVER '{'
{
 vs = malloc(sizeof(*vs));
 vs->names = NULL;
 vs->refcount = 1;
} server_items '}'
{
 if (vs->names == NULL)
 {
  log("[Config] Server block lacks any names, line %d\n", lineno);
  free(vs);
 }
 else
  add_to_list(&VoteServers, vs);
} ';';

server_items: server_items server_item | server_item;
server_item: server_name;
server_name: NAME '=' QSTRING
{
 add_to_list(&vs->names, strdup(yylval.string));
} ';';

noreop_block: NOREOP '{' noreop_items '}' ';';
noreop_items: noreop_items noreop_host | noreop_host;
noreop_host: HOST '=' QSTRING
{
 add_to_list(&HKeywords, strdup(yylval.string));
} ';';

jexempt_block: JEXEMPT
{
 je = malloc(sizeof(*je));
 je->flags = 0;
 je->name = NULL;
} '{' jexempt_items '}'
{
 if (je->name == NULL || je->flags == 0)
 {
  if (je->name != NULL)
   free(je->name);
  free(je);
 }
 else
 {
  add_to_list(&JupeExempts, je);
 }
} ';';
jexempt_items: jexempt_item jexempt_items | jexempt_item;
jexempt_item: jexempt_name | jexempt_type;
jexempt_name: NAME '=' QSTRING
{
 if (je->name != NULL)
  free(je->name);
 je->name = strdup(yylval.string);
} ';';
jexempt_type: TYPE '=' AUTO
{
 je->flags |= JEFLAG_AUTO;
}';' | TYPE '=' MANUAL
{
 je->flags |= JEFLAG_AUTO;
} ';' | TYPE '=' ALL
{
 je->flags |= JEFLAG_AUTO | JEFLAG_MANUAL;
} ';';
%%
