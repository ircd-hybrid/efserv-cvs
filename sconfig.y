%{
  #include <stdlib.h>
  #include <string.h>
  #include <stdio.h>
  #include "efserv.h"
  int yylex();
  void yyerror();
  struct ServAdmin *ad;
  struct AdminHost *ah;
  struct VoteServer *vs;
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
%%

conf: conf conf_item | conf_item;

conf_item: general_block |
           admin_block |
           server_block;

general_block: GENERAL '{' general_items '}' ';';

general_items: general_items general_item | general_item;
general_item: general_name |
              general_port |
              general_host |
              general_pass |
              general_nick;

general_name: NAME '=' QSTRING
{
 DupString(server_name, yylval.string);
} ';' ;

general_port: PORT '=' NUMBER
{
 port = yylval.number;
} ';' ;

general_host: HOST '=' QSTRING
{
 DupString(server_host, yylval.string);
} ';' ;

general_pass: PASS '=' QSTRING
{
 DupString(server_pass, yylval.string);
} ';' ;

general_nick: NICK '=' QSTRING
{
 DupString(sn, yylval.string);
} ';' ;

admin_block: ADMIN '{'
{
 ad = malloc(sizeof(*ad));
 ah = malloc(sizeof(*ah));
 ad->name = NULL;
 ad->pass = NULL;
 ad->hosts = NULL;
 ad->caps = 0;
 ad->refcount = 1;
} admin_items '}'
{
 if (ad->name == NULL || ad->pass == NULL || ad->hosts == NULL)
  deref_admin(ad);
 else
 {
  ad = add_to_list(&serv_admins, ad);
 }
 free(ah);
}';';

admin_items: admin_items admin_item | admin_item;
admin_item: admin_name | admin_pass | admin_auth

admin_name: NAME '=' QSTRING
{
 DupString(ad->name, yylval.string);
} ';';

admin_pass: PASS '=' QSTRING
{
 DupString(ad->pass, yylval.string);
} ';';

admin_auth: AUTH '{' auth_items '}'
{
 if (ah->server && ah->server && ah->host)
 {
  add_to_list(&ad->hosts, ah);
  ah = malloc(sizeof(*ah));
 } else
 {
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
  free(vs);
 else
  add_to_list(&VoteServers, vs);
} ';';

server_items: server_items server_item | server_item;
server_item: server_name;
server_name: NAME '=' QSTRING
{
 add_to_list(&vs->names, strdup(yylval.string));
} ';';

%%
