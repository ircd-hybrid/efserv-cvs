/*
 *  dyntrans.y: The header parser for the dynamic translation system.
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
 * $Id: dyntrans.y,v 1.1 2001/12/10 07:04:46 a1kmm Exp $
 */
%{
  #include <stdlib.h>
  #include <string.h>
  #include <stdio.h>
  #include <time.h>
  #include "conf.h"
  #include "config.h"
  #include "dyntrans.h"
  #include "utils.h"
  int isptr;
  char *yyexpr = NULL;
  char *len = NULL, *type = NULL, *stype = NULL, *nrepeats = NULL,
       *flags  = NULL, *sname;
  extern int yyleng;
  void yyerror(const char *e);
  void start_function(void);
  int yylex(void);
%}
%token T_NAME
%token T_INT
%token T_CHAR
%token T_SHORT
%token T_LONG
%token T_STRUCT
%token T_NEEDINIT
%token T_IFUNC
%token T_VOID
%token T_EXTERN
%token T_INC
%token T_GINC
%token T_EXPR
%token T_GLOBAL
%token T_UCEND
%token T_ENUM
%%

ALL: ITEMS
ITEMS: ITEM | ITEM ITEMS;
ITEM: STRUCT | VDECLARE | AFPROTOTYPE | T_INC
{
 printf("#include \"%s\"\n", yylval.string);
 free(yylval.string);
} | T_GINC
{
 printf("#include <%s>\n", yylval.string);
 free(yylval.string);
} | AGLOBAL | ENUM;

ENUM: T_ENUM '{' NAMELIST COMMAOK '}' ';'

COMMAOK: /* empty */ | ',';

STRUCTNAME: T_STRUCT T_NAME

RTTYPE: STRUCTNAME '*'
{
  printf("\"%s\", 1, ", yylval.string);
  free(yylval.string);
} | STRUCTNAME
{
  printf("\"%s\", 0, ", yylval.string);
  free(yylval.string);
};

ARTTYPE: RTTYPE T_EXPR
{
  /* ampersand isn't needed for an array. */
  printf("%s, %s);\n", yyexpr, len);
  free(yyexpr);
  free(len);
  len = NULL;
} | RTTYPE
{
  printf("1, &%s);\n", len);
  free(len);
  len = NULL;
};

AGLOBAL: T_GLOBAL T_NAME
{
  if (len == NULL)
    free(len);
  len = yylval.string;
  printf("  RegisterStructInst(\"%s\", ", yylval.string);
} ',' ARTTYPE T_UCEND;

STRUCTTYPE: T_STRUCT T_NAME {free(yylval.string);};
SIMPLETYPE: T_INT | T_CHAR | T_SHORT | T_LONG | STRUCTTYPE | T_NAME;
ANYPTR: '*' | ANYPTR '*';
ANYORNOPTR: /* empty */ | ANYPTR;
ANYTYPE: SIMPLETYPE ANYORNOPTR | T_VOID ANYORNOPTR;
ARG: ANYTYPE T_NAME PARRAY {free(yylval.string);};
ARGLIST: ARG | ARGLIST ',' ARG;
PARGLIST: ARGLIST | T_VOID;
PARRAY: /* empty */ | T_EXPR;
FPROTOTYPE: ANYTYPE T_NAME PARRAY {free(yylval.string);} '(' PARGLIST ')' ';';
AFPROTOTYPE: T_EXTERN FPROTOTYPE | FPROTOTYPE;
NAMELIST: NAMELIST ',' NAME | NAME;
NAME: ANYORNOPTR T_NAME PARRAY{free(yylval.string);}
VDECLARE: T_EXTERN SIMPLETYPE NAMELIST';';
STRUCT: T_STRUCT T_NAME
{
 start_function();
 printf("  /* Begin struct %s */\n", yylval.string);
 printf("  s = (struct StructType*)malloc(sizeof(struct StructType));\n"
        "  s->name = strdup(\"%s\");\n"
        "  s->len = 0;\n"
        "  s->flags = 0;\n"
        "  s->fields = NULL;\n", yylval.string);
 if (stype)
   free(stype);
 stype = strdup("f->stype = NULL");
 if (len)
   free(len);
 len = NULL;
 if (nrepeats != NULL)
   free(nrepeats);
 nrepeats = NULL;
 if (flags != NULL)
   free(flags);
 flags = strdup("0");
 isptr = 0;
 sname = yylval.string;
} '{' SARGLIST '}'
{
 printf("  RegisterStructType(s);\n");
 free(sname);
}';';
SARGLIST: SARG SARGLIST | SARG;
SARG: SSARG SFLAGS ';' | SSARG ';';
SFLAGS: SFLAGS SFLAG | SFLAG;
SFLAG: T_IFUNC
{
 printf("  f->func = &%s\n", yylval.string);
 free(yylval.string);
} | T_NEEDINIT
{
 printf("  f->flags |= SFF_NEEDRESTART;\n");
 free(yylval.string);
};
SSARRAY: /* empty */ |
         T_EXPR
{
  nrepeats = yyexpr;
  yyexpr = NULL;
}

SSARG: SSTYPE SSTNAMES
{
  if (stype)
    free(stype);
  stype = strdup("f->stype = NULL");
  if (len != NULL)
    free(len);
  len = NULL;
  if (type)
    free(type);
  type = NULL;
  if (flags)
    free(flags);
  flags = strdup("0");
}

SSTNAMES: SSTANAME | SSTNAMES ',' SSTANAME;

SSTANAME: SSTNAME
{
  printf("  f = (struct Field*)malloc(sizeof(struct Field));\n"
         "  f->type = %s;\n"
         "  %s;\n"
         "  f->len = %s;\n"
         "  f->name = strdup(\"%s\");\n"
         "  f->flags = %s;\n"
         "  f->nrepeats = %s;\n"
         "  {\n"
         "    struct %s tmpv;\n"
         "    f->new_offset = ((unsigned long)&tmpv.%s) - (unsigned long)&tmpv;\n"
         "  }\n"
         "  if (f->new_offset + f->len > s->len)\n"
         "    s->len = f->new_offset + f->len;\n"
         "  add_to_list(&s->fields, f);\n", type, stype, len,
         yylval.string, flags, nrepeats ? nrepeats : "1",
         sname, yylval.string);
  if (nrepeats)
    free(nrepeats);
  nrepeats = NULL;
  isptr = 0;
  free(yylval.string);
}

SSTNAME: SSPTR T_NAME SSARRAY | T_NAME SSARRAY |
SSTYPE '(' SSPTR T_NAME
{
#if 1
  fprintf(stderr, "Encountered a function pointer field! Dying.\n");
  exit(-1);
#else
  printf("  f->name = \"%s\";\n"
         "  f->type = FIELD_FUNCPTR;\n");
  free(yylval.string);
#endif
} ')''(' PARGLIST ')';
SSTYPE: SSPTR STYPE | STYPE;
SSPTR: '*'
{
 char *oflags = flags;
 isptr = 1;
 flags = malloc(strlen(flags) + sizeof("| SFF_ISPOINTER"));
 strcpy(flags, oflags);
 strcat(flags, "| SFF_ISPOINTER");
 if (len != NULL)
   free(len);
 len = strdup("sizeof(void*)");
} | SSPTR '*'
{
 char *oflags = flags;
 if (isptr < 2)
 {
   flags = malloc(strlen(flags) + sizeof("| SFF_ISPOLYPOINTER"));
   strcpy(flags, oflags);
   strcat(flags, "| SFF_ISPOLYPOINTER");
 }
 isptr++;
};
STYPE: T_INT
{
 type = strdup("FIELD_INTTYPE");
 len = strdup("sizeof(int)");
} | T_CHAR
{
 type = strdup("FIELD_INTTYPE");
 len = strdup("sizeof(char)");
} | T_LONG
{
 type = strdup("FIELD_INTTYPE");
 len = strdup("sizeof(long)");
} | T_VOID
{
 /* This is an error unless it is a pointer... */
 type = strdup("FIELD_INTTYPE");
 len = strdup("sizeof(void*)");
} | T_STRUCT T_NAME
{
 type = strdup("FIELD_STRUCT");
 if (stype)
   free(stype);
 stype = (char*)malloc(strlen(yylval.string) +
                       sizeof("SaveStructType(\"\", &f->stype)"));
 sprintf(stype, "SaveStructType(\"%s\", &f->stype)", yylval.string);
 len = malloc(sizeof("sizeof()") + strlen(yylval.string));
 sprintf(len, "sizeof(%s)", yylval.string);
 free(yylval.string);
} | T_NAME
{
 /* We don't use typedefs for structs in efserv, so if we don't
  * recognise the type, assume it is an integer type of some form.
  */
 type = strdup("FIELD_INTTYPE");
 len = malloc(sizeof("sizeof()") + strlen(yylval.string));
 sprintf(len, "sizeof(%s)", yylval.string);
 free(yylval.string);
};
