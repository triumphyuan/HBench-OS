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
 * lat_sig.c - signal handler latency benchmark
 *
 * XXX - this benchmark requires the POSIX sigaction interface.  The reason
 * for that is that the signal handler stays installed with that interface.
 * The more portable signal() interface may or may not stay installed and
 * reinstalling it each time is expensive.
 *
 * Based on:
 *	$lmbenchID: lat_sig.c,v 1.3 1995/09/26 05:38:58 lm Exp $
 *
 * $Id: lat_sig.c,v 1.6 1997/06/27 00:33:58 abrown Exp $
 *
 */
char	*id = "$Id: lat_sig.c,v 1.6 1997/06/27 00:33:58 abrown Exp $\n";

#include <signal.h>
#include "common.c"

/* Worker function */
int do_install();
int do_handle();

int
main(ac, av)
	int ac;
	char **av;
{
	clk_t		totaltime;
	unsigned int	niter;
	int 		(*fn)();

	/* print out RCS ID to stderr*/
	fprintf(stderr, "%s", id);

	/* Check command-line arguments */
	if (parse_counter_args(&ac, &av) || ac != 3) {
		fprintf(stderr, "usage: %s%s iterations [install|handle]\n", 
			av[0], counter_argstring);
		exit(1);
	}

	/* parse command line parameters */
	niter = atoi(av[1]);
	if (!strcmp(av[2],"install")) 
		fn = &do_install;
	else
		fn = &do_handle;

	/* initialize timing module (calculates timing overhead, etc) */
	init_timing();

#ifndef COLD_CACHE
	/* 
	 * Generate the appropriate number of iterations so the test takes
	 * at least one second. For efficiency, we are passed in the expected
	 * number of iterations, and we return it via the process error code.
	 * No attempt is made to verify the passed-in value; if it is 0, we
	 * we recalculate it.
	 */
	if (niter == 0) {
		niter = gen_iterations(fn, clock_multiplier);
		printf("%d\n",niter);
		return (0);
	}

	/*
	 * Take the real data and average to get a result
	 */
	(*fn)(1, &totaltime);		/* prime caches, etc. */
#else
	niter = 1;
#endif
	(*fn)(niter, &totaltime);	/* get latency */
	
	output_latency(totaltime, niter);
	
	return (0);
}

int	caught = 0;

void	handler() { caught++; }
void	handler2() { caught++; }

int
do_install(num_iter, t)
	int 	num_iter;
	clk_t	*t;
{
	int	i, me;
	struct	sigaction sa, old;

	/* 
	 * Get my PID and set up signal handler 
	 */
	me = getpid();
	sa.sa_handler = handler2;
	sigemptyset(&sa.sa_mask);	/* don't care */
	sa.sa_flags = 0;		/* don't care */
	sigaction(SIGUSR1, &sa, 0);

	if (num_iter == 1) {	/* special case for cold cache */
		start();
		sa.sa_handler = handler;
		sigemptyset(&sa.sa_mask);	/* don't care */
		sa.sa_flags = 0;		/* don't care */
		sigaction(SIGUSR1, &sa, &old);
		*t = stop();
		sigaction(SIGUSR1, &old, 0);
		return (0);
	}
	/* Start timing */
	start();
	for (i = num_iter/2; i > 0; i--) { /* each loop does 2 installations */
		sa.sa_handler = handler;
		sigemptyset(&sa.sa_mask);	/* don't care */
		sa.sa_flags = 0;		/* don't care */
		sigaction(SIGUSR1, &sa, &old);
		sigaction(SIGUSR1, &old, 0);
	}
	*t = stop();

	return(0);
}

int
do_handle(num_iter, t)
	int 	num_iter;
	clk_t 	*t;
{
	int 	i, me;
	clk_t 	overhead;
	struct	sigaction sa;

	/* 
	 * Get my PID and set up signal handler 
	 */
	me = getpid();
	sa.sa_handler = handler2;
	sigemptyset(&sa.sa_mask);	/* don't care */
	sa.sa_flags = 0;		/* don't care */
	sigaction(SIGUSR1, &sa, 0);

	/*
	 * First we measure the system call overhead for kill by using a 
	 * null signal
	 */
	start();
	for (i = num_iter; i > 0; i--) {
		kill(me, 0);
	}
	overhead = stop();

	/*
	 * Now make the real measurement
	 */
	start();
	for (i = num_iter; i > 0; i--) {
		kill(me, SIGUSR1);
	}
	*t = stop();
	*t -= overhead;

	return (0);
}
