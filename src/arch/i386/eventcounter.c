/*
 * Copyright (c) 1997 The President and Fellows of Harvard College.
 * All rights reserved.
 * Copyright (c) 1997 Aaron B. Brown.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program, in the file COPYING in this distribution;
 *   if not, write to the Free Software Foundation, Inc., 675 Mass Ave,
 *   Cambridge, MA 02139, USA.
 *
 * Results obtained from this benchmark may be published only under the
 * name "HBench-OS".
 */

/*
 * eventcounter.c -- code to support Intel Pentium and Pentium Pro event 
 *                   counters under NetBSD (requires NetBSD counter driver).
 *
 * $Id: eventcounter.c,v 1.3 1997/06/27 00:35:25 abrown Exp $
 */

/* must define u_int64_t */
#if defined(__bsdi__)
#include <machine/types.h>
#else
#include <sys/types.h>
#endif

#include <stdlib.h>

#ifndef i386
#  error Event counters not supported on this architecture
#endif

#ifndef __GNUC__
#  error Gnu C is required to compile this file
#endif

#ifndef __NetBSD__
#  error Event counter driver is only supported under NetBSD/i386
#endif

/*
 * Set up counter size types; same for P5 and P6
 */
typedef u_int64_t eventcounter_t;
#define EVENTCOUNTERTFMT	"%qd"
#define EVENTCOUNTERTSTR(x)	((eventcounter_t)strtoq(x, NULL, 10))

/*
 * Check if they want Pentium or Pentium Pro support
 */
#if EVENT_COUNTERS == 5
#error Pentium support not yet implemented.
#elif EVENT_COUNTERS == 6

/**
 ** Pentium Pro
 **/
#include "p6counter.h"

#define select_eventcounter(ctr,str) select_p6counter((ctr),strtol((str),NULL,0))
#define select_eventcounter_defaults() {select_eventcounter(0,"0x79");select_eventcounter(1,"0x79");}

static u_int64_t c0s, c0e, c1s, c1e;
#define start_eventcounters() { RDPMC(0, &c0s); RDPMC(1, &c1s); }
#define stop_eventcounters() { RDPMC(0, &c0e); RDPMC(1, &c1e); }

#define get_eventcounter(ctr) (ctr == 0 ? (c0e-c0s) : (ctr == 1 ? (c1e - c1s) :\
						       (c0s - c0s)))
#define set_eventcounter_val(ctr, val) (ctr == 0 ? (c0s = 0, c0e = val) :\
					(ctr == 1) ? (c1s = 0, c1e = val) :\
					-1)
#else
#error You must define EVENT_COUNTERS to be either 5 (Pentium) or 6 (PPro)
#endif
