/*
 *  md5.c: The MD5 code.
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
 * $Id: md5.c,v 1.1 2001/05/29 09:29:45 a1kmm Exp $
 */
#include <string.h>
#include "efserv.h"

void md5_block(unsigned long *in, unsigned long *out, unsigned long *x);

char*
getmd5(struct User *usr)
{
 static unsigned long md5sum[4];
 int l, p;
 char userathost[USERLEN+HOSTLEN+17];
 memset(userathost, 0, sizeof(userathost));
 memset(md5sum, 0, sizeof(md5sum));
 strcpy(userathost, usr->user);
 strcat(userathost, "@");
 strcat(userathost, usr->host);
 l = strlen(userathost);
 l |= 0xF;
 for (p=0; p<l; p+=16)
  md5_block((unsigned long*)(userathost+p), md5sum, md5sum);
 return (char*)md5sum;
}

#define rotl(x,n) ((((x)<<(n))&(-(1<<(n))))|(((x)>>(32-(n)))&((1<<(n))-1)))
#define F(x,y,z) (((x)&(y))|((~x)&(z)))
#define G(x,y,z) (((x)&(z))|((y)&(~z)))
#define H(x,y,z) ((x)^(y)^(z))
#define I(x,y,z) ((y)^((x)|(~z)))

static int ork[64] = {
  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 
  1,  6, 11,  0,  5,  0,  5,  4,  9, 14,  3,  8, 13,  2,  7, 12, 
  5,  8, 11, 14,  1,  4,  7, 10, 13,  0,  3,  6,  9, 12, 15,  2, 
  0,  7, 14,  5, 12,  3, 10,  1,  8, 15,  6, 13,  4, 11,  2,  9
};

static int ors[64] = {
  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22, 
  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20, 
  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23, 
  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21
};

static unsigned long t[64] = {
 0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
 0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
 
 0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
 0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
 
 0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
 0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
 
 0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
 0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
};


void
md5_block(unsigned long *in, unsigned long *out, unsigned long *x)
{
 unsigned long a, b, c, d;
 int i, j;

 a = in[0];
 b = in[1];
 c = in[2];
 d = in[3];
 for (i=0; i<4; i++)
 {
  j = 4*i;
  a = b + rotl(a + F(b, c, d) + x[ork[j]] + t[j], ors[j]);
  d = a + rotl(d + F(a, b, c) + x[ork[j+1]] + t[j+1], ors[j+1]);
  c = d + rotl(c + F(d, a, b) + x[ork[j+2]] + t[j+2], ors[j+2]);
  b = c + rotl(b + F(c, d, a) + x[ork[j+3]] + t[j+3], ors[j+3]);
 }
 for (i=0; i<4; i++)
 {
  j = 4*i + 16;
  a = b + rotl(a + G(b, c, d) + x[ork[j]] + t[j], ors[j]);
  d = a + rotl(d + G(a, b, c) + x[ork[j+1]] + t[j+1], ors[j+1]);
  c = d + rotl(c + G(d, a, b) + x[ork[j+2]] + t[j+2], ors[j+2]);
  b = c + rotl(b + G(c, d, a) + x[ork[j+3]] + t[j+3], ors[j+3]);
 }
 for (i=0; i<4; i++)
 {
  j = 4*i + 32;
  a = b + rotl(a + H(b, c, d) + x[ork[j]] + t[j], ors[j]);
  d = a + rotl(d + H(a, b, c) + x[ork[j+1]] + t[j+1], ors[j+1]);
  c = d + rotl(c + H(d, a, b) + x[ork[j+2]] + t[j+2], ors[j+2]);
  b = c + rotl(b + H(c, d, a) + x[ork[j+3]] + t[j+3], ors[j+3]);
 }
 for (i=0; i<4; i++)
 {
  j = 4*i + 48;
  a = b + rotl(a + I(b, c, d) + x[ork[j]] + t[j], ors[j]);
  d = a + rotl(d + I(a, b, c) + x[ork[j+1]] + t[j+1], ors[j+1]);
  c = d + rotl(c + I(d, a, b) + x[ork[j+2]] + t[j+2], ors[j+2]);
  b = c + rotl(b + I(c, d, a) + x[ork[j+3]] + t[j+3], ors[j+3]);
 }
 a += in[0];
 b += in[1];
 c += in[2];
 d += in[3];
 out[0] = a;
 out[1] = b;
 out[2] = c;
 out[3] = d;
}
