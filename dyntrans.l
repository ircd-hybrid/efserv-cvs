/*
 *  dyntrans.l: The header lexer for the dynamic translation system.
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
 * $Id: dyntrans.l,v 1.1 2001/12/10 07:04:46 a1kmm Exp $
 */

%{
#include "conf.h"
#include "dyntrans.tab.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#define YY_NO_UNPUT
extern char *yyexpr;
%}

%x COMMENT
%x NOTRANS

preprocess #.*
name [a-z|A-Z|0-9_]+
multiptr "*"+
whitespace [ \t\r\n]
%%
{whitespace} ;
{preprocess} ;
unsigned ;
signed ;
const ;
int {return T_INT;}
char {return T_CHAR;}
long {return T_LONG;}
short {return T_SHORT;}
struct {return T_STRUCT;}
void {return T_VOID;}
extern {return T_EXTERN;}
enum {return T_ENUM;}
"/*PNOTRANS*/" {BEGIN(NOTRANS);}
<NOTRANS>"/*PYESTRANS*/" { BEGIN(INITIAL); }
<NOTRANS>[^/]* ;
<NOTRANS>"/"[^*] ;
<NOTRANS>"/*"[^P] ;
<NOTRANS>. {exit(-1);}
"/*PNEEDINIT*/" {return T_NEEDINIT;}
"/*PIFUNC(\""[^\"]*"\")*/" {
 yylval.string = yytext+10;
 *(strchr(yylval.string, ')')) = 0;
 yylval.string = strdup(yylval.string);
 return T_IFUNC;
}
"/*PINC(\""[^\"]*"\")*/" {
  yylval.string = yytext+8;
  *(strchr(yylval.string, '\"')) = 0;
  yylval.string = strdup(yylval.string);
  return T_INC;
}
"/*PGINC(\""[^\"]*"\")*/" {
  yylval.string = yytext+9;
  *(strchr(yylval.string, '\"')) = 0;
  yylval.string = strdup(yylval.string);
  return T_GINC;
}
"/*PGLOBAL(" { return T_GLOBAL; }
")*/" { return T_UCEND; }

"/*" {BEGIN(COMMENT);}
<COMMENT>"*/" {BEGIN(INITIAL);}
<COMMENT>"*"[^/] ;
<COMMENT>[^*]* ;
"["[^\]]*"]" {
  yytext[yyleng-1] = 0;
  if (yyexpr != NULL)
    free(yyexpr);
  yyexpr = strdup(yytext+1);
  return T_EXPR;
}
{name} {
  yylval.string = strdup(yytext);
  yylval.string[yyleng] = 0;
  return T_NAME;
}
. {return yytext[0];}
