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
 */

/*
 * timing.c - support routines for timing.
 *
 * Based on lmbench, file
 * 	$lmbenchId: timing.c,v 1.6 1995/08/25 03:30:30 lm Exp $
 *
 * $Id: timing.c,v 1.13 1997/06/27 03:51:57 abrown Exp $
 */
#ifndef __TIMING_C__		/* protect against multiple inclusions */
#define __TIMING_C__

#include "bench.h"
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>

#if !defined (CYCLE_COUNTER) && !defined(EVENT_COUNTERS)
typedef	unsigned int clk_t;	/* microseconds can be u_int's */
#define CLKTFMT		"%d"
#define CLKTSTR(x) 	atoi(x)
#endif /* COUNTERS */

clk_t	stop();

#ifdef CYCLE_COUNTER
static internal_clk_t start_clk, stop_clk;
#else
static struct timeval start_tv, stop_tv;
#endif

#define OVERHEAD_INNER_LOOPS	100
#define OVERHEAD_OUTER_LOOPS	10
#define OVERHEAD_TAILS		2
clk_t timing_overhead = 0;

/** 
 ** Timing functions
 **/

/*
 * Calculate timing overhead; initialize timing systems
 *
 * We measure the counter overhead by starting and stopping 100 times,
 * then averaging the results. We do this 10 times and take the middle 8.
 */
static int 
clktcomp(const void *a, const void *b)
{
        /* we assume at most a 30 bit difference! */
        return (int)((*((clk_t *)a)) - (*((clk_t *)b)));
}

void 
init_timing() 
{
	int i,j;
	clk_t *vals;

	vals = (clk_t *)malloc(OVERHEAD_OUTER_LOOPS * sizeof(clk_t));
	if (!vals) {
		perror("malloc");
		exit(1);
	}

#ifdef CYCLE_COUNTER
	zero_cycle_counter();
#endif

	timing_overhead = 0;
	start();		/* prime caches, etc */
	stop();

	for (i = OVERHEAD_OUTER_LOOPS-1; i >= 0; i--) {
		vals[i] = 0;
		for (j = OVERHEAD_INNER_LOOPS; j > 0; j--) {
			start();
			vals[i] += stop();
		}
		vals[i] /= OVERHEAD_INNER_LOOPS;
	}
	qsort(vals, OVERHEAD_OUTER_LOOPS, sizeof(clk_t), clktcomp);

	/* pluck off tails */
	for (i = OVERHEAD_TAILS; i < OVERHEAD_OUTER_LOOPS-OVERHEAD_TAILS; i++)
		timing_overhead += vals[i];

	timing_overhead /= OVERHEAD_OUTER_LOOPS - 2*OVERHEAD_TAILS;
#ifdef DEBUG
	printf(">> timing overhead %d\n",timing_overhead);
#endif
	free(vals);
}

/*
 * Start timing now.
 */
void
start()
{
#ifdef CYCLE_COUNTER
	read_cycle_counter(&start_clk);
#else
	(void) gettimeofday(&start_tv, (struct timezone *) 0);
#endif

#ifdef EVENT_COUNTERS
	/*  
	 * We start the event counters *after* the cycle counter under the
	 * assumption that the user is more interested in precise event
	 * timing, otherwise why would they be using the counters at all?
	 */
	start_eventcounters();
#endif
}

/*
 * Stop timing and return real time in microseconds.
 */
clk_t
stop()
{
#ifdef EVENT_COUNTERS
	stop_eventcounters();
#endif
#ifdef CYCLE_COUNTER
	read_cycle_counter(&stop_clk);
#else
	struct timeval tdiff;

	(void) gettimeofday(&stop_tv, (struct timezone *) 0);
#endif /* CYCLE_COUNTER */

#ifdef CYCLE_COUNTER
	/* Sanity checking; prevent "negative" results */
	if (stop_clk <= start_clk || (stop_clk-start_clk) <= timing_overhead)
		return (0);
	else 
		return (((clk_t)(stop_clk - start_clk)) - timing_overhead);
#else
	tvsub(&tdiff, &stop_tv, &start_tv);
	if ((tdiff.tv_sec * 1000000 + tdiff.tv_usec) <= 0 ||
	    (tdiff.tv_sec * 1000000 + tdiff.tv_usec) <= timing_overhead)
		return (0);
	else
		return ((tdiff.tv_sec * 1000000 + tdiff.tv_usec) - 
			timing_overhead);
#endif /* CYCLE_COUNTER */
}

/*
 * Figure out how many iterations of workfn() are needed to make it take
 * one second.
 *
 * workfn(i, t) takes a number of iterations to run, and returns its
 * time in clk_t *t; workfn should return 0 on success, -1 on error.
 */
unsigned int 
gen_iterations(workfn, clkmul)
	int (*workfn)(int, clk_t *);
	float clkmul;
{
	unsigned int num = 1;
	clk_t rtntime;
	float time=0.;

	if (!workfn)
		return 1;
	
	/* special-case the 1-iteration case for easier error-checking */
	if ((*workfn)(1, &rtntime) != 0)
		return (1);
#ifdef DEBUG
	printf(">> %d iteration gives %f seconds\n",num,((float)rtntime)*clkmul/1000000.);
#endif
	while ((time = ((float)rtntime)*clkmul) < 1000000.) {
		/* while less than one second */
		num <<= 1;
		if ((*workfn)(num, &rtntime) != 0) {
			num >>= 1;
#ifdef DEBUG
			printf(">> backing off\n");
#endif
			break;
		}
#ifdef DEBUG
		printf(">> %d iterations gives %f seconds\n",num,((float)rtntime)*clkmul/1000000.);
#endif
	}
#ifdef DEBUG
	printf(">> Choosing %d iterations\n",num);
#endif
	return (num);
}

/*
 * Subtract two timevals 
 */
void
tvsub(tdiff, t1, t0)
	struct timeval *tdiff, *t1, *t0;
{
	tdiff->tv_sec = t1->tv_sec - t0->tv_sec;
	tdiff->tv_usec = t1->tv_usec - t0->tv_usec;
	if (tdiff->tv_usec < 0)
		tdiff->tv_sec--, tdiff->tv_usec += 1000000;
}

/* XXX Remainder is old from lmbench */
#ifdef UTIL

#define	nz(x)	((x) == 0 ? 1 : (x))
#define	MB	(1024*1024.0)
#define	KB	(1024.0)

void
bandwidth(bytes, verbose)
	int	bytes, verbose;
{
	struct timeval td;
	double  s, bs;

	tvsub(&td, &stop_tv, &start_tv);
	s = td.tv_sec + td.tv_usec / 1000000.0;
	bs = bytes / nz(s);
	if (verbose) {
		if ((bs / KB) > 1024) {
			(void) fprintf(stderr, "%.2f MB in %.2f secs, %.2f MB/sec\n",
			    bytes / MB, s, bs / MB);
		} else {
			(void) fprintf(stderr, "%.2f MB in %.2f secs, %.2f KB/sec\n",
			    bytes / MB, s, bs / KB);
		}
	} else {
		(void) fprintf(stderr, "%.4f %.2f\n",
		    bytes / MB, bs / MB);
	}
}

void
kb(bytes)
	int	bytes;
{
	struct timeval td;
	double  s, bs;

	tvsub(&td, &stop_tv, &start_tv);
	s = td.tv_sec + td.tv_usec / 1000000.0;
	bs = bytes / nz(s);
	(void) fprintf(stderr, "%.0f KB/sec\n", bs / KB);
}

void
mb(bytes)
	int	bytes;
{
	struct timeval td;
	double  s, bs;

	tvsub(&td, &stop_tv, &start_tv);
	s = td.tv_sec + td.tv_usec / 1000000.0;
	bs = bytes / nz(s);
	(void) fprintf(stderr, "%.2f MB/sec\n", bs / MB);
}

void
latency(xfers, size)
	int	xfers, size;
{
	struct timeval td;
	double  s;

	tvsub(&td, &stop_tv, &start_tv);
	s = td.tv_sec + td.tv_usec / 1000000.0;
	fprintf(stderr,
	    "%d xfers in %.2f secs, %.4f millisec/xfer, %.2f KB/sec\n",
	    xfers, s, s * 1000 / xfers,
	    (xfers * size) / (1024. * s));
}

void
context(xfers)
	int	xfers;
{
	struct timeval td;
	double  s;

	tvsub(&td, &stop_tv, &start_tv);
	s = td.tv_sec + td.tv_usec / 1000000.0;
	fprintf(stderr,
	    "%d context switches in %.2f secs, %.0f microsec/switch\n",
	    xfers, s, s * 1000000 / xfers);
}

void
nano(s, n)
	char	*s;
	int	n;
{
	struct timeval td;
	double	micro;

	tvsub(&td, &stop_tv, &start_tv);
	micro = td.tv_sec * 1000000 + td.tv_usec;
	micro *= 1000;
	fprintf(stderr, "%s: %.0f nanoseconds\n", s, micro / n);
}

void
micro(s, n)
	char	*s;
	int	n;
{
	struct timeval td;
	int	micro;

	tvsub(&td, &stop_tv, &start_tv);
	micro = td.tv_sec * 1000000 + td.tv_usec;
	fprintf(stderr, "%s: %d microseconds\n", s, micro / n);
}

void
milli(s, n)
	char	*s;
	int	n;
{
	struct timeval td;
	int	milli;

	tvsub(&td, &stop_tv, &start_tv);
	milli = td.tv_sec * 1000 + td.tv_usec / 1000;
	fprintf(stderr, "%s: %d milliseconds\n", s, milli / n);
}

void
ptime(n)
	int	n;
{
	struct timeval td;
	double  s;

	tvsub(&td, &stop_tv, &start_tv);
	s = td.tv_sec + td.tv_usec / 1000000.0;
	fprintf(stderr,
	    "%d in %.2f secs, %.0f microseconds each\n", n, s, s * 1000000 / n);
}

double
timespent()
{
	struct timeval td;

	tvsub(&td, &stop_tv, &start_tv);
	return (td.tv_sec + td.tv_usec / 1000000);
}
#endif /*UTIL*/

#endif /* __TIMING_C__ */
