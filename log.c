/*
 *  log.c: The efserv logger.
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
 * $Id: log.c,v 1.4 2001/12/10 07:47:20 a1kmm Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include "config.h"
#include "define.h"
#include "struct.h"

extern FILE *logfile;

void
fatal_error(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  if (logfile != NULL)
  {
    vfprintf(logfile, format, args);
    fclose(logfile);
  }
  exit(-1);
}

void
open_logfile(void)
{
  if (logfile != NULL)
    return;
  logfile = fopen(LOGFILE, "a");
  if (logfile == NULL)
    fatal_error("Could not open the log file.\n");
}

void
log(const char *format, ...)
{
  va_list args;
  char tnow[50];
  timenow = time(0);
  va_start(args, format);
  strcpy(tnow, ctime(&timenow));
  tnow[strlen(tnow) - 1] = 0;
  fprintf(stderr, "[%s] ", tnow);
  vfprintf(stderr, format, args);
  if (logfile == NULL)
    return;
  fprintf(logfile, "[%s] ", tnow);
  vfprintf(logfile, format, args);
  fflush(logfile);
}
