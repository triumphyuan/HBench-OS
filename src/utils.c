/*
 * Copyright (c) 1997 The President and Fellows of Harvard College.
 * All rights reserved.
 * Copyright (c) 1997 Aaron B. Brown.
 * Copyright (c) 1994 Larry McVoy.
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
 * This work is derived from, but can no longer be called, lmbench.
 * Results obtained from this benchmark may be published only under the
 * name "HBench-OS".
 *
 */

/*
 * utils.c - various helper/library functions
 *
 * Based on lmbench, file
 * 	$lmbenchId: timing.c,v 1.6 1995/08/25 03:30:30 lm Exp $
 *
 * $Id: utils.c,v 1.5 1997/06/27 00:33:58 abrown Exp $
 */

/*
 * All output goes to stdout; all version information goes to stderr
 */

#include "bench.h"
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>

#include "timing.c"		/* We depend on this for clk_t... */

#define	nz(x)	((x) == 0 ? 1 : (x))
#define	MB	(1024*1024.0)
#define	KB	(1024.0)

/*
 * Functions to get the number of bytes specified by a string in the form
 * nnnK or nnnM
 */
char
lastchar(s)
	char	*s;
{
	while (*s++)
		;
	return (s[-2]);
}

int
parse_bytes(s)
	char	*s;
{
	int	n = atoi(s);

	if ((lastchar(s) == 'k') || (lastchar(s) == 'K'))
		n *= 1024;
	if ((lastchar(s) == 'm') || (lastchar(s) == 'M'))
		n *= (1024 * 1024);
	return (n);
}

/*
 * Functions to produce desired output formats
 */
void 
output_bandwidth(unsigned int bytes, clk_t ticks)
{
	float usecs = ((float)ticks)*clock_multiplier;
	if (usecs > 0.0)
		printf("%.4f", (((float)bytes)/MB)/(((float)usecs)/1000000.));
	else
		printf("%.4f", 0.0);

#ifdef EVENT_COUNTERS
	/* 
	 * XXX We assume that the last "start-stop" pair contains all
	 * of the interesting timing data!
	 *
	 * We print the number of bytes transferred before the counter
	 * values so that the values actually mean something.
	 */
	printf(" %u", bytes);
	if (eventcounter_active[0])
		printf(" " EVENTCOUNTERTFMT, 
		       get_eventcounter(0));
	if (eventcounter_active[1])
		printf(" " EVENTCOUNTERTFMT, 
		       get_eventcounter(1));
#endif
	printf("\n");
}

void 
output_latency(clk_t usecs, unsigned int niter)
{
	float usec_per_iter = (((float)usecs)/((float)niter))*clock_multiplier;
	printf("%.4f", usec_per_iter);

#ifdef EVENT_COUNTERS
	/* 
	 * XXX We assume that the last "start-stop" pair contains all
	 * of the interesting timing data!
	 */
	if (eventcounter_active[0])
		printf(" " EVENTCOUNTERTFMT, 
		       get_eventcounter(0)/(eventcounter_t)niter);
	if (eventcounter_active[1])
		printf(" " EVENTCOUNTERTFMT, 
		       get_eventcounter(1)/(eventcounter_t)niter);
#endif
	printf("\n");
}

/*
 * Output a latency to a fd, in nanoseconds
 */
void 
output_latency_ns_fd(clk_t usecs, unsigned int niter, int fd)
{
	char buf[96], buf1[32], buf2[32];

	float ns_per_iter = (((float)usecs*1000.0)/((float)niter))*clock_multiplier;
#ifdef EVENT_COUNTERS
	/* 
	 * XXX We assume that the last "start-stop" pair contains all
	 * of the interesting timing data!
	 */
	buf1[0] = buf2[0] = '\0';
	if (eventcounter_active[0])
		sprintf(buf1, " " EVENTCOUNTERTFMT, 
			get_eventcounter(0)/(eventcounter_t)niter);
	if (eventcounter_active[1])
		sprintf(buf2, " " EVENTCOUNTERTFMT, 
			get_eventcounter(1)/(eventcounter_t)niter);
	sprintf(buf, "%.4f %s %s\n", ns_per_iter, buf1, buf2);
#else
	sprintf(buf, "%.4f\n", ns_per_iter);
#endif
	if (write(fd, buf, strlen(buf)) != strlen(buf)) {
		perror("file write");
		exit(1);
	}
}

/*
 * Functions to do center-weighted averaging
 */
static int 	centeravg_max;
static clk_t	*centeravg_array = NULL;
static int	centeravg_cur;
static float	centeravg_tailpct;

void
centeravg_reset(numpoints, tailpct)
	int numpoints;
	float tailpct;
{
	centeravg_max = numpoints;
	
	if (centeravg_array != NULL)
		free(centeravg_array);

	centeravg_array = (clk_t *)malloc(numpoints * sizeof(clk_t));
	if (!centeravg_array) {
		perror("malloc");
		exit(1);
	}

	centeravg_cur = 0;
	centeravg_tailpct = tailpct;
}

void
centeravg_add(dat)
	clk_t dat;
{
	if (centeravg_cur >= centeravg_max) {
		fprintf(stderr,"centeravg_add: no more array space\n");
		exit(1);
	}
	if (!centeravg_array) {
		fprintf(stderr,"centeravg_add: array not initialized\n");
		exit(1);
	}
		
	centeravg_array[centeravg_cur++] = dat;
}

void
centeravg_done(t)
	clk_t *t;
{
	int i, tailsize;

	/* Handle case where fewer datapoints are entered */
	centeravg_max = centeravg_cur;
	
	/* Sort the data points */
	qsort(centeravg_array, centeravg_max, sizeof(clk_t), clktcomp);

	/* Now average the middle (1-centeravg_tailpct)*100 percent */
	tailsize = (int)(((float)centeravg_max)*centeravg_tailpct);

	*t = 0;
	for (i = tailsize; i < centeravg_max - tailsize; i++)
		*t += centeravg_array[i];
	*t /= centeravg_max - (2 * tailsize);

	free(centeravg_array);
	centeravg_array = NULL;
	centeravg_max = centeravg_cur = 0;
}
