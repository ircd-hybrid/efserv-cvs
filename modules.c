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
 * $Id: modules.c,v 1.2 2001/05/27 10:16:28 a1kmm Exp $
 */

#define PATH "/home/andrew/scratch/efserv/"
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

int reload_module=0, die=0;

struct List *Servers = NULL, *Users = NULL, *Channels = NULL,
            *Hosts = NULL, *Monitors = NULL, *VoteServers = NULL;
struct Server *first_server = NULL;
void *mmod;
int server_fd;
#define HASHSIZE 0x1000
struct HashEntry *hash[0x1000];
char *server_name=NULL, *server_pass=NULL, *server_host=NULL;
/* Interface nick... */
char *sn = NULL;
int port;
struct List *serv_admins=NULL;

void
handle_sighup(int n)
{
 void (*do_rehash)(void);
 if (!mmod)
  return;
 do_rehash = dlsym(mmod, "do_rehash");
 do_rehash();
}

void
handle_sigusr1(int n)
{
 reload_module = 1;
}

int
main(int argc, char **argv)
{
 void (*do_main_loop)(void);
 void (*do_setup)(void);
 void *dt;
 /* Force efence in... */
 dt = malloc(1);
 signal(SIGHUP, handle_sighup);
 signal(SIGUSR1, handle_sigusr1);
 mmod = dlopen(PATH"efserv.so", RTLD_NOW);
 if (mmod == NULL)
 {
  printf("Error loading efserv: %s\n", dlerror());
  exit(-1);
 }
 do_setup = (void (*)(void))dlsym(mmod, "do_setup");
 do_main_loop = (void (*)(void))dlsym(mmod, "do_main_loop");
 do_setup();
 while (die == 0)
 {
  if (reload_module)
  {
   dlclose(mmod);
   if ((mmod = dlopen(PATH"efserv.so", RTLD_NOW)) == NULL)
   {
    printf("Error loading efserv: %s\n", dlerror());
    exit(-1);
   }
   do_main_loop = (void (*)(void))dlsym(mmod, "do_main_loop");
   reload_module = 0;
  }
  do_main_loop();
 }
 return 0;
}
