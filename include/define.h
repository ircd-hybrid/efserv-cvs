/*
 *  define.h: Contains macro definitions for efserv.
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
 * $Id: define.h,v 1.7 2001/11/11 22:13:52 wcampbel Exp $
 */

#define MAXCLONES_UHOST 4
#define MAXCLONES_HOST 6
#define MAX_SJOIN_DELAY 60

#define JUPE_EXPIRE_TIME 45 *60
#define MINIMUM_OPS  0 /* 4 */
#define EXOP_EXPIRE_TIME 2*60 /* *60 */
#define CHAN_SLICE_LENGTH 5
#define CYCLE_REJOIN_TIME 30

#define NETNAME "test net"
#define LOGFILE PREFIX "efserv.log"
#define CHANNEL_DB PREFIX "efchans.db"
#define VERSION "pre0.1-test"

#define OKILL_MAX 10
#define SKILL_MAX 200

#define ETCPATH PREFIX "etc/"

#define MAX_ARGS 256

#define JUPE_CLEANUP_TIME 10
#define CHANNEL_CLEANUP_TIME 5
#define CLONE_CLEANUP_TIME 10
#define BUFLEN 512
#define READLEN 2048
#define ERROR_SLEEP_TIME 3

/* Below this should not be changed on setup... */
#define UFLAG_ADMIN           0x00000001
#define UFLAG_OPER            0x00000002
#define UFLAG_SERVADMIN       0x00000004

#define IsAdmin(x) (x->flags & UFLAG_ADMIN)
#define IsOper(x)  (x->flags & UFLAG_OPER)
#define IsServAdmin(x)  (x->flags & UFLAG_SERVADMIN)

#define CHFLAG_BANNED         0x00000001
#define CHFLAG_OPERONLY       0x00000002
#define CHFLAG_ADMINONLY      0x00000004
#define SMODES CHFLAG_BANNED | CHFLAG_OPERONLY | CHFLAG_ADMINONLY

#define IsBanChan(x) (x->flags & CHFLAG_BANNED)
#define IsOperChan(x) (x->flags & CHFLAG_OPERONLY)
#define IsAdminChan(x) (x->flags & CHFLAG_ADMINONLY)
#define HasSMODES(x) (x->flags & (SMODES))

#define SERVFLAG_JUPED 1
#define SERVFLAG_ACTIVE 2

#define IsJuped(x) (x->flags & SERVFLAG_JUPED)

#define SACAP_SUNJUPE 1
#define CanSUnjupe(x) (x->sa && x->sa->caps & SACAP_SUNJUPE)

#define JEFLAG_AUTO 1
#define JEFLAG_MANUAL 2
