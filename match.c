/*
 *  match.c: Performs matching of masks.
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
 * $Id: match.c,v 1.1 2001/05/31 08:52:05 a1kmm Exp $
 */
#include <assert.h>
#include <ctype.h>

#define MATCH_MAX_CALLS 512  /* ACK! This dies when it's less that this
                                and we have long lines to parse */
int
match(const char *mask, const char *name)
{
 const unsigned char* m = (const unsigned char*)  mask;
 const unsigned char* n = (const unsigned char*)  name;
 const unsigned char* ma = (const unsigned char*) mask;
 const unsigned char* na = (const unsigned char*) name;
 int   wild  = 0;
 int   calls = 0;
 int   quote = 0;
 assert(0 != mask);
 assert(0 != name);
 if (!mask || !name)
  return 0;
 while (calls++ < MATCH_MAX_CALLS)
 {
  if (quote)
   quote++;
  if (quote == 3)
   quote = 0;
  if (*m == '\\' && !quote)
  {
   m++;
   quote = 1;
   continue;
  }
  if (!quote && *m == '*')
  {
   /*
    * XXX - shouldn't need to spin here, the mask should have been
    * collapsed before match is called
    */
   while (*m == '*')
    m++;
   if (*m == '\\')
   {
    m++;
    /* This means it is an invalid mask -A1kmm. */
    if (!*m)
     return 0;
    quote = 2;
   }
   wild = 1;
   ma = m;
   na = n;
  }
  if (!*m)
  {
   if (!*n)
    return 1;
   if (quote)
    return 0;
   for (m--; (m > (const unsigned char*) mask) && (*m == '?'); m--)
    ;
   if (*m == '*' && (m > (const unsigned char*) mask))
    return 1;
   if (!wild)
    return 0;
   m = ma;
   n = ++na;
  } else if (!*n)
  {
   /*
    * XXX - shouldn't need to spin here, the mask should have been
    * collapsed before match is called
    */
   if (quote)
    return 0;
   while (*m == '*')
    m++;
   return (*m == 0);
  }
  if (tolower(*m) != tolower(*n) && !(!quote && *m == '?'))
  {
   if (!wild)
    return 0;
   m = ma;
   n = ++na;
  } else
  {
   if (*m)
    m++;
   if (*n)
    n++;
  }
 }
 return 0;
}
