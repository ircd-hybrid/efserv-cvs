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
 * $Id: efserv.c,v 1.12 2001/12/10 07:47:20 a1kmm Exp $
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <arpa/nameser.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/errno.h>
#include <string.h>
#include <time.h>
#include "config.h"
#include "define.h"
#include "struct.h"
#include "utils.h"
#include "funcs.h"

int send_error = 0;
time_t timenow;
extern int server_fd, connected;

void init_hash(void);
void read_all_config(void);
void open_logfile(void);

unsigned long
resolve_host(const char *host)
{
  struct hostent *he;
  unsigned long addr;
  if ((addr = inet_addr(host)))
    return addr;
  if ((he = gethostbyname(host)) == NULL)
    return 0;
  if (he->h_addr_list == NULL || he->h_addr_list[0] == NULL)
    return 0;
  return *(unsigned long *)he->h_addr_list[0];
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
  if (connect(server_fd, (struct sockaddr *)&sai, sizeof(sai)) < 0)
    return -1;
  return 0;
}

int
send_msg(char *msg, ...)
{
  va_list val;
  int i, l;
  char buffer[BUFLEN], *p;
  va_start(val, msg);
  vsnprintf(buffer, BUFLEN - 1, msg, val);
  va_end(val);
  printf("Out: %s\n", buffer);
  i = strlen(buffer);
  buffer[i++] = '\r';
  buffer[i++] = '\n';
  buffer[i] = 0;
  for (p = buffer;
       i != (p - buffer) && (l = write(server_fd, p, i - (p - buffer))) > 0;
       p += l)
    ;
  if (l <= 0)
  {
    send_error = (l == 0) ? -1 : errno;
    return -1;
  }
  return 0;
}

void
parse(char *msg, int len)
{
  struct Command *cmd;
  int parc = 1;
  char *parv[MAX_ARGS], *p, *sender;
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
  }
  else
    sender = first_server ? first_server->name : NULL;
  for (p = strtok(NULL, " "); parc < MAX_ARGS && p; p = strtok(NULL, " "))
  {
    if (*p != ':')
      parv[parc++] = p;
    else
    {
      char *pn = strtok(NULL, "");
      parv[parc++] = p + 1;
      if (pn)
        pn[-1] = ' ';
      break;
    }
  }
  if (parv[0] == NULL)
    return;
  if ((cmd = find_in_hash(HASH_COMMAND, parv[0])) == NULL)
    return;
  cmd->func(sender, parc, parv);
}

void
check_events(void)
{
  static time_t
    last_cleanup_jupes = 0, last_cleanup_chans = 0, last_cleanup_clones = 0;
  if (timenow - last_cleanup_jupes > JUPE_CLEANUP_TIME)
  {
    last_cleanup_jupes = timenow;
    cleanup_jupes();
  }
  if (timenow - last_cleanup_chans > CHANNEL_CLEANUP_TIME)
  {
    last_cleanup_chans = timenow;
    cleanup_channels();
  }
  if (timenow - last_cleanup_clones > CLONE_CLEANUP_TIME)
  {
    last_cleanup_clones = timenow;
    cleanup_hosts();
  }
}

void
wipe_client_status(void)
{
  struct List *node, *nnode;
  struct Server *svr;
  /* This takes care of the users and the channels... */
  FORLISTDEL(node, nnode, Servers, struct Server *, svr)
  {
    if (svr->jupe)
      destroy_server_links(svr);
    else
      destroy_server(svr);
  }
  first_server = NULL;
}

void
do_connect_server(void)
{
  struct List *node;
  struct Server *svr;
  wipe_client_status();
  pick_a_hub();
  log("[Hub] Connecting to hub %s:%d.\n", server_host, port);
  while (connect_server(server_host, port) == -1)
  {
    log("[Hub] Could not connect to hub %s:%d: %s\n", server_host, port,
        strerror(errno));
    sleep(3);
    pick_a_hub();
    log("[Hub] Connecting to hub %s:%d.\n", server_host, port);
  }
  log("[Hub] Connected to hub %s:%d\n", server_host, port);
  connected = 1;
  send_msg("CAPAB :QS EX CHW IE HOPS HUB AOPS");
  send_msg("PASS %s :TS", server_pass);
  send_msg("SERVER %s 1 : * Services *", server_name);
  send_msg("NICK %s 1 1 +o services %s %s :* Services *", sn,
           server_name, server_name);
  FORLIST(node, Servers, struct Server *, svr) if (IsJuped(svr))
  {
    send_msg(":%s SQUIT %s :Juped: %s", sn, svr->name, svr->jupe->reason);
    send_msg("SERVER %s 2 :Juped: %s", svr->name, svr->jupe->reason);
  }
}

void
do_setup_commands(void)
{
  /* Clear commands from hash... */
  wipe_type_from_hash(HASH_COMMAND, NULL);
  hash_commands();
}

void
do_main_loop(void)
{
  char read_buffer[READLEN], *p = read_buffer, *pe = read_buffer, *m;
  int rv, skip = 0;
  do_setup_commands();
  if (connected == 0)
    do_connect_server();
  while (reload_module == 0 && die == 0)
  {
    check_events();
    if ((rv = read(server_fd, p, READLEN - (pe - read_buffer))) <= 0)
    {
      if (rv < 0 && (errno == EAGAIN || errno == EINTR))
        continue;
      log("[Hub] Connection to hub lost: %s\n",
          rv ? strerror(errno) : "Connection reset by peer");
      close(server_fd);
      connected = 0;
      sleep(3);
      return;
    }
    timenow = time(0);
    m = p;
    for (pe = p + rv; p < pe; p++)
      if (*p == '\r' || *p == '\n')
      {
        *p++ = 0;
        if (skip == 0)
          parse(m, p - m - 1);
        if (send_error != 0)
        {
          close(server_fd);
          connected = 0;
          log("[Hub] Connection to hub lost: send error.\n");
          sleep(ERROR_SLEEP_TIME);
          return;
        }
        skip = 0;
        while ((*p == '\r' || *p == '\n') && p < pe)
          p++;
        if (p == pe)
          break;
        m = p;
      }
    if (m == read_buffer + READLEN)
      skip = 1;
    if (m != read_buffer && m != pe)
    {
      memmove(read_buffer, m, pe - m);
      p = read_buffer + (pe - m);
      pe = read_buffer + (pe - m);
      m = read_buffer;
    }
    else if (m == pe)
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
  srand(time(0));
  open_logfile();
  timenow = time(0);
  log("[Status] Services starting.\n");
  init_hash();
  read_all_config();
  load_channel_opdb();
  return;
}
