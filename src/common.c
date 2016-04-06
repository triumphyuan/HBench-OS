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
 * Common include file for hbench-OS benchmarks. Basically just pulls in
 * other files, but guarantees things are included in the right order.
 *
 * $Id: common.c,v 1.3 1997/06/27 00:33:58 abrown Exp $
 */

#ifndef __COMMON_C__
#define __COMMON_C__

/*
 * Pull in the bench.h header
 */
#include "bench.h"

/*
 * Pull in the counters. Note that we assume that event counters => cycle 
 * counter.
 */
#if defined(EVENT_COUNTERS)
#define CYCLE_COUNTER
#  if defined(i386)
#    include "arch/i386/cyclecounter.c"
#    include "arch/i386/eventcounter.c"
#  else
#    error Event counters not supported on this architecture
#  endif
#elif defined(CYCLE_COUNTER)
#  if defined(i386)
#    include "arch/i386/cyclecounter.c"
#  else
#    error Cycle counter not supported on this architecture
#  endif
#else
#  ifdef COLD_CACHE
#  warning Cold cache results are not reliable without cycle counters
#  endif
#endif

#include "counter-common.c"

/*
 * Now pull in the timing support
 */
#include "timing.c"

/*
 * Now pull in the utility functions
 */
#include "utils.c"

#endif /* __COMMON_C__ */
