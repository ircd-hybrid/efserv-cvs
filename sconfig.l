/*
 *  sconfig.l: The efserv lexer. Pulled pretty much straight from
 *             ircd.
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
 * $Id: sconfig.l,v 1.6 2001/11/11 21:22:24 wcampbel Exp $
 */

%option case-insensitive
%option noyywrap

%{
#include "conf.h"
#include "sconfig.tab.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
int lineno = 1;
#define YY_NO_UNPUT
%}

ws        [ \t\r]*
digit     [0-9]
comment   #.*
qstring   \"[^\"\n]*[\"\n]

%%
\n.*	        { lineno++; yyless(1); }

{ws}            ;
{comment}       ;

{digit}+        { yylval.number = atoi(yytext); return NUMBER; }

{qstring}	{ yylval.string = yytext+1;
		  if(yylval.string[yyleng-2] != '"')
		    ; // log(L_ERROR, "Unterminated character string");
		  else
		    yylval.string[yyleng-2] = '\0'; /* remove close quote */
		  return QSTRING;
		}
general { return GENERAL; }
nick { return NICK; }
pass { return PASS; }
name { return NAME; }
host { return HOST; }
port { return PORT; }
admin { return ADMIN; }
auth { return AUTH; }
server { return SERVER; }
user { return USER; }
hub { return HUB; }
minimum_servers { return MINSERVS; }
noreop { return NOREOP; }
yes { return YES; }
no { return NO; }
sunjupe { return SUNJUPE; }
jexempt { return JEXEMPT; }
type { return TYPE; }
manual { return MANUAL; }
auto { return AUTO; }
all { return ALL; }
.		        { return yytext[0]; }
<<EOF>> { yyterminate(); }
%%
