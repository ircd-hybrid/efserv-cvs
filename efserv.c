/*
 *  efserv.c: The startup and other miscellaneous efserv functions.
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
 * $Id: efserv.c,v 1.4 2001/05/26 01:41:03 a1kmm Exp $
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/errno.h>
#include <string.h>
#include "efserv.h"

int send_error = 0;
time_t timenow;
extern int server_fd;

void init_hash(void);
void read_all_config(void);

void
fatal_error(const char *error, ...)
{
 va_list args;
 va_start(args, error);
 vfprintf(stderr, error, args);
 va_end(args);
 exit(-1);
}

unsigned long
resolve_host(const char *host)
{
 struct hostent *he;
 unsigned long addr;
 if ((addr = inet_addr(host)))
  return addr;
 if ((he = gethostbyname(host)) == NULL)
  return 0;
 if (he->h_addr_list==NULL || he->h_addr_list[0]==NULL)
  return 0;
 return *(unsigned long*)he->h_addr_list[0];
}

int
connect_server(const char *host, int port)
{
 unsigned long ip_addr;
 struct sockaddr_in sai;
 if (!(ip_addr = resolve_host(host)))
  return -1;
 server_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
 if (server_fd < 0)
  return -1;
 sai.sin_family = AF_INET;
 sai.sin_port = htons(port);
 sai.sin_addr.s_addr = ip_addr;
 if (connect(server_fd, (struct sockaddr*)&sai, sizeof(sai)) < 0)
  return -1;
 return 0;
}

int
send_msg(char *msg, ...)
{
 va_list *val;
 int i, l;
 char buffer[512], *p;
 va_start(val, msg);
 vsnprintf(buffer, 510, msg, val);
 va_end(val);
 printf("Out: %s\n", buffer);
 i = strlen(buffer);
 buffer[i++] = '\r';
 buffer[i++] = '\n';
 buffer[i] = 0;
 for (p=buffer; 
      i!=(p-buffer) && (l=write(server_fd, p, i-(p-buffer)))>0; 
      p += l)
  ;
 if (l <= 0)
 {
  send_error = (l==0) ? -1 : errno;
  return -1;
 }
 return 0;
}

void
parse(char *msg, int len)
{
 struct Command *cmd;
 int parc = 1;
 char *parv[256], *p, *sender;
 if (len == 0)
  return;
 printf("In: %s\n", msg);
 parv[0] = strtok(msg, " ");
 if (parv[0] == NULL)
  return;
 if (parv[0][0] == ':')
 {
  sender = ++parv[0];
  parv[0] = strtok(NULL, " ");
 } else
  sender = first_server ? first_server->name : NULL;
 for (p = strtok(NULL, " "); parc<256 && p; p = strtok(NULL, " "))
 {
  if (*p != ':')
   parv[parc++] = p;
  else
  {
   char *pn = strtok(NULL, "");
   parv[parc++] = p+1;
   if (pn)
    pn[-1] = ' ';
   break;
  }
 }
 if ((cmd = find_in_hash(HASH_COMMAND, parv[0])) == NULL)
  return;
 cmd->func(sender, parc, parv);
}

void
do_main_loop(void)
{
 char read_buffer[2048], *p = read_buffer, *pe = read_buffer, *m;
 int rv, skip = 0;
 while (reload_module == 0 && die == 0)
 {
  if ((rv = read(server_fd, p, 2048-(pe-read_buffer))) <= 0)
   return;
  timenow = time(0);
  m = p;
  for (pe = p + rv; p < pe; p++)
   if (*p == '\r' || *p == '\n')
   {
    *p++ = 0;
    if (skip == 0)
     parse(m, p-m-1);
    if (send_error != 0)
     return;
    skip = 0;
    while ((*p == '\r' || *p == '\n') && p < pe)
     p++;
    if (p == pe)
     break;
    m = p;
   }
  if (m == read_buffer+2048)
   skip = 1;
  if (m != read_buffer && m != pe)
  {
   memmove(read_buffer, m, pe-m);
   p = read_buffer + (pe-m);
   pe = read_buffer + (pe-m);
   m = read_buffer;
  } else if (m == pe)
  {
   p = read_buffer;
   pe = read_buffer;
   m = read_buffer;
  }
 }
}

void
do_setup(void)
{
 init_hash();
 read_all_config();
 printf("server_name: %s\n", server_name);
 hash_commands();
 if (connect_server(server_host, port))
  fatal_error("Could not connect to the server: %s\n", strerror(errno));
 send_msg("PASS %s :TS", server_pass);
 printf("server_name: %s\n", server_name);
 send_msg("SERVER %s 1 : * Services *", server_name);
 send_msg("NICK %s 1 1 +o services %s %s :* Services *", sn,
          server_name, server_name);
 return;
}
