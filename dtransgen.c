/*
 *  dtransgen.c: Dynamic translation registration routine generator.
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
 * $Id: dtransgen.c,v 1.1 2001/12/10 07:22:35 a1kmm Exp $
 */
#include <stdio.h>

void yyparse(void);

void
start_function(void)
{
  static int done = 0;
  if (done)
    return;
  printf("void\n"
         "setup_dyntrans(void)\n"
         "{\n"
         "  struct StructType *s;\n"
         "  struct Field *f;\n");
  done++;
}

int
main(int argc, char **argv)
{
  yyparse();
  start_function();
  printf("}\n");
  return 0;
}


void
yyerror(const char *m)
{
  fprintf(stdout, "dtransgen error: %s\n", m);
  // exit(0);
}
