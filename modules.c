/*
 *  modules.c: The module controller.
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
 * $Id: modules.c,v 1.10 2002/04/15 20:25:23 wcampbel Exp $
 */

#define PATH PREFIX
#include "config.h"
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include "utils.h"
#include "struct.h"

int reload_module = 0, die = 0;
int connected = 0, server_count = 0, minimum_servers = 0;
time_t channel_record_time = 0;
void *mmod;
int server_fd = -1;
char *server_name = NULL, *server_pass = NULL, *server_host = NULL;
char *sn = NULL;
int port;
FILE *logfile;

/* After all that, these globals do need to be here. */
struct List *HKeywords, *Channels, *serv_admins, *VoteServers, *Hubs,
  *JupeExempts, *Servers, *Users, *Monitors, *Hosts;
struct Server *first_server;
struct HashEntry *hash[HASHSIZE];



/* This is here because it has to be. */
struct List *structtypes = NULL, *structinsts = NULL;
unsigned long reloadno = 0;

void
handle_sighup(int n)
{
  void (*do_rehash) (void);
  if (!mmod)
    return;
  do_rehash = dlsym(mmod, "do_rehash");
  do_rehash();
}

/* This is the severe measure of actually quitting IRC(!) and restarting
 * services. */
void
restart(void)
{
  fprintf(stderr, "Restarting...\n");
  if (fork() == 0)
  {
    execl(PREFIX "/efserv", "");
  }
  exit(0);
}

void
handle_sigusr1(int n)
{
  reload_module = 1;
}

int
main(int argc, char **argv)
{
  void (*do_main_loop) (void);
  void (*do_setup) (void);
  void (*translate_all) (void);
  void *dt;
  /* Force efence in... */
  dt = malloc(1);
  signal(SIGHUP, handle_sighup);
  signal(SIGUSR1, handle_sigusr1);
  /* FreeBSD & gdb detach correction */
  signal(SIGTRAP, SIG_IGN);
  mmod = dlopen(PATH "efserv.so", RTLD_NOW);
  if (mmod == NULL)
  {
    printf("Error loading efserv: %s\n", dlerror());
    exit(-1);
  }
  do_setup = (void (*)(void))dlsym(mmod, "do_setup");
  reload_module = 1;
  while (die == 0)
  {
    if (reload_module)
    {
      dlclose(mmod);
      if ((mmod = dlopen(PATH "efserv.so", RTLD_NOW)) == NULL)
      {
        printf("Error loading efserv: %s\n", dlerror());
        exit(-1);
      }
      do_main_loop = (void (*)(void))dlsym(mmod, "do_main_loop");
      translate_all = (void (*)(void))dlsym(mmod, "TranslateAll");
      reload_module = 0;
      reloadno++;
      translate_all();
      if (reloadno == 1)
        do_setup();
    }
    do_main_loop();
  }
  return 0;
}
