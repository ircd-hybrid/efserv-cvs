/*
 *  config.h: Contains option definitions for efserv.
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
 * $Id: config.h,v 1.2 2001/11/12 00:43:12 wcampbel Exp $
 */

#ifndef CONFIG_H
#define CONFIG_H

/* USE_AUTOJUPE - Automatically jupe servers that are showing explicit
 * op hacking behavior
 */

#define USE_AUTOJUPE

/* USE_CYCLE - Enable the CYCLE command, usable by any chanop on the
 * channel in question.  This will lock the channel, kick all users,
 * then allow the channel to be reconstructed and fixed according to the
 * channels database.
 */

#define USE_CYCLE

/* USE_SMODE - Enable the SMODE command, set channels banned (closed),
 * oper only, or admin only.
 *
 * XXX - Not yet complete, it will not save dynamic settings between
 *       restarts
 */

#undef USE_SMODE

/* These should be set to the maximum permitted by any irc
 * server on the network
 */

#define NICKLEN 20
#define HOSTLEN 64
#define USERLEN 40
#define SERVLEN 80
#define CHANLEN 255

#endif
