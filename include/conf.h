/*
 *  config.h: Defines stuff relating to the config parser.
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
 * $Id: conf.h,v 1.1 2001/05/31 08:52:07 a1kmm Exp $
 */

extern struct yystype
{
 unsigned long number;
 char *string;
} yylval;
#define YYSTYPE struct yystype

enum
{
 ALEVEL_ADMIN,
 ALEVEL_OPER,
 ALEVEL_SERVADMIN,
 ALEVEL_ANY,
};

