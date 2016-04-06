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
 * counter-common.c -- common declarations for counter support, and stubs
 *                     for when counters are not compiled in.
 *
 * $Id: counter-common.c,v 1.4 1997/06/27 00:33:58 abrown Exp $
 */

#include <stdlib.h>
#include <string.h>

/*
 * The multiplier for the clock, used to convert cycles to time in some
 * cases. In units of microseconds per clock-tick.
 */
float clock_multiplier = 1.0;

#if defined (EVENT_COUNTERS)
static char *counter_argstring = " [-c1 csel1] [-c2 csel2] clock_multiplier";

static int eventcounter_active[2] = {0, 0};

/*
 * Parse the clock multiplier and event counter selectors; 
 * return 0 on success and 1 on failure
 */
int parse_counter_args(int *acp, char ***avp)
{
	char *av0 = (*avp)[0];
	
	/*
	 * Start out with default, sane values for the counters in case
	 * the hardware barfs by default.
	 */
	select_eventcounter_defaults();

	/* 
	 * Be simplistic: assume that c1 always preceeds c2, and don't do
	 * any error-checking (since this is invoked by the driver anyway).
	 */
	if (*acp >= 3 && !strcmp((*avp)[1], "-c1")) {
		/* handle c1, update argc by 2, argv by 2 */
		select_eventcounter(0, (*avp)[2]);
		eventcounter_active[0] = 1;

		*acp -= 2;
		*avp += 2;
	}
	if (*acp >= 3 && !strcmp((*avp)[1], "-c2")) {
		/* handle c2, update argc by 2, argv by 2 */
		select_eventcounter(1, (*avp)[2]);
		eventcounter_active[1] = 1;

		*acp -= 2;
		*avp += 2;
	}

	/* Get clock multiplier */
	if (*acp < 2) {
		(*avp)[0] = av0;
		return 1;
	}

	clock_multiplier = (float)atof((*avp)[1]);
	(*acp)--;
	(*avp)++;
	(*avp)[0] = av0;

	return 0;
}	
#elif defined (CYCLE_COUNTER)
static char *counter_argstring = " clock_multiplier";

/*
 * Parse the clock multiplier; return 0 on success and 1 on failure
 */
int parse_counter_args(int *acp, char ***avp)
{
	if (*acp < 2)
		return 1;

	(*acp)--;
	clock_multiplier = (float)atof((*avp)[1]);
	(*avp)[1] = (*avp)[0];
	(*avp)++;
	return 0;
}
#else
static char *counter_argstring = "";
int parse_counter_args(int *acp, char ***avp)
{
	clock_multiplier = 1.0;
	return 0;
}
#endif
