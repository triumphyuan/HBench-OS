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
 * mhz.c - calculate clock rate and megahertz
 *
 * Usage: mhz [-c]
 *
 * The sparc specific code is to get around the double-pumped ALU in the
 * SuperSPARC that can do two adds in one clock. Apparently, only
 * SuperSPARC does this.
 * Thanks to John Mashey (mash@sgi.com) for explaining this.
 *
 * The AIX specific code is because the C compiler on rs6000s does not
 * respect the register directive.  "a" below is a stack variable and
 * there are two instructions per increment instead of one.
 * Please note that this may not be correct for other AIX systems.
 *
 * Based on:
 *	$lmbenchId: mhz.c,v 1.1 1994/11/18 08:51:55 lm Exp $
 *
 * $Id: mhz.c,v 1.4 1997/06/27 00:33:58 abrown Exp $
 */
char	*id = "$Id: mhz.c,v 1.4 1997/06/27 00:33:58 abrown Exp $\n";

#include	"common.c"

/* The worker functions */
int 	do_ops();
int	do_null();
int	do_overhead();

#define MHZ_LOOPS	10

int
main(ac, av)
	int ac;
        char  **av;
{
	unsigned int	niter;
	clk_t		totaltime, overhead, val;
	double 		mhz;
	int 		i;

	/* Check command-line arguments */
	if (ac >= 3) {
		fprintf(stderr, "Usage: %s [-c]\n", av[0]);
	}

	/* initialize timing module (calculates timing overhead, etc) */
	init_timing();
	clock_multiplier = 1.0;

	/*
	 * Generate the appropriate number of iterations so the test takes
	 * at least one second.
	 */
	niter = gen_iterations(&do_ops, clock_multiplier);

	/*
	 * Measure the loop overhead.
	 */
	do_overhead(niter, &overhead);

	/*
	 * Now make the actual mhz measurement.
	 * Here, we actually take the minimum, since we're counting up
	 * 1-cycle instructions. All of these instructions *must* get
	 * executed, so there's no possible system interaction that
	 * could make the measured result lower than the real result.
	 */
	do_ops(niter, &totaltime); /* prime the minimum */

	for (i = MHZ_LOOPS; i > 0; i--) {
		do_ops(niter, &val);
		if (val < totaltime)
			totaltime = val;
	}

	/* remove overhead */
	totaltime -= overhead;

	mhz = ((double)niter*1000.0) / (double) totaltime;

	if (ac == 2 && !strcmp(av[1], "-c")) {
		printf("%.4f\n", 1000 / mhz);
	} else {
		printf("%.2f Mhz, %.0f nanosec clock\n", mhz, 1000 / mhz);
	}
	exit(0);
}

/*
 * This gunk is used in do_ops, below
 */
#ifdef sparc
#	define	FOUR	a >>= 1; a >>= 1; a >>= 1; a >>= 1;
#else
#	ifdef _AIX	/* really for the rs6000 only */
#		define	FOUR	a++; a++;
#	else
#		define	FOUR	a++; a++; a++; a++;
#	endif
#endif
#define TWENTY	FOUR FOUR FOUR FOUR FOUR
#define	H	TWENTY TWENTY TWENTY TWENTY TWENTY

/*
 * Worker function #1: does the actual measurement.
 */
int
do_ops(num_iter, t)
	int num_iter;
	clk_t *t;
{
	register int a, i;

	a = 1024;

	/* Start timing */
	start();

	/* Do 1000 ops, num_iter times. */
	for (i = num_iter; i > 0; i--) {
		H H H H H
		H H H H H
	}
	*t = stop((void *)a);

	return (0);
}

/*
 * Worker function #2: measures the loop overhead
 */
int
do_null(num_iter, t)
	int num_iter;
	clk_t *t;
{
	register int a, i;

	a = 1024;

	/* Start timing */
	start();

	/* Do 1000 ops, num_iter times. */
	for (i = num_iter; i > 0; i--) {
		;
	}
	*t = stop(NULL);

	return (0);
}

/*
 * Worker function #3: gathers the average of the middle 16 values of 20
 * iterations of do_null().
 */
#define OVERHEADAVG_LOOPS	20
#define OVERHEADAVG_TAILS	0.2
int
do_overhead(num_iter, t)
	int num_iter;
	clk_t *t;
{
	int 	i;
	clk_t	val;

	centeravg_reset(OVERHEADAVG_LOOPS, OVERHEADAVG_TAILS);

	for (i = OVERHEADAVG_LOOPS; i > 0; i--) {
		do_null(num_iter, &val);
		centeravg_add(val);
	}

	centeravg_done(t);

	return (0);
}
