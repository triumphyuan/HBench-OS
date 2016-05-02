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
 * lat_syscall.c - time simple entry into the system. We try several different
 *                 uncacheable "null" system calls to determine the minimum
 *                 system entry overhead.
 *
 * Based on lmbench, file
 * 	$lmbenchId: lat_syscall.c,v 1.2 1995/09/24 01:32:37 lm Exp $
 *
 * $Id: lat_syscall.c,v 1.7 1997/06/27 00:33:58 abrown Exp $
 */
char	*id = "$Id: lat_syscall.c,v 1.7 1997/06/27 00:33:58 abrown Exp $\n";

#include "common.c"

#include <sys/time.h>
#include <sys/resource.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>

/* Worker functions */
int do_sigaction();
int do_gettimeofday();
int do_sbrk();
#ifndef NO_RUSAGE
int do_getrusage();
#endif
int do_writedevnull();
int do_getpid();

/*
 * Global variables: these are the parameters required by the worker routine.
 * We make them global to avoid portability problems with variable argument
 * lists and the gen_iterations function
 */
int	fd;			/* file descriptor of /dev/null */

main(ac, av)
	int ac;
	char  **av;
{
	clk_t		totaltime;
	unsigned int	niter;
	char *		scname;
	int 		(*scfunc)(int, clk_t *);

	/* print out RCS ID to stderr*/
	fprintf(stderr, "%s", id);

	/* Check command-line arguments */
	if (parse_counter_args(&ac, &av) || ac != 3) {
		fprintf(stderr, "Usage: %s%s iterations "
			"[sigaction | gettimeofday | sbrk | getrusage | write | getpid]\n",
			av[0], counter_argstring);
		exit(1);
	}

	/* parse command line parameters */
	niter = atoi(av[1]);
	fd = open("/dev/null", 1);
	if (fd == -1) {
		perror("open");
		exit(1);
	}
	scname = av[2];
	if (!strcmp(scname, "sigaction"))
		scfunc = &do_sigaction;
	else if (!strcmp(scname, "gettimeofday"))
		scfunc = &do_gettimeofday;
	else if (!strcmp(scname, "sbrk"))
		scfunc = &do_sbrk;
	else if (!strcmp(scname, "getrusage")) {
#ifndef NO_RUSAGE
		scfunc = &do_getrusage;
#else
		fprintf(stderr,"rusage not supported on this machine\n");
		exit(1);
#endif
	}
	else if (!strcmp(scname, "write"))
		scfunc = &do_writedevnull;
	else if (!strcmp(scname, "getpid"))
		scfunc = &do_getpid;
	else {
		fprintf(stderr, "Usage: %s%s iterations_persec "
			"[sigaction | yyy | zzz]\n", av[0],
			counter_argstring);
		exit(1);
	}

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
		niter = gen_iterations(scfunc, clock_multiplier);
		printf("%d\n",niter);
		return (0);
	}

	/*
	 * Take the real data and average to get a result
	 */
	(*scfunc)(1, &totaltime); /* prime caches */
#else
	niter = 1;
#endif
	(*scfunc)(niter, &totaltime);	/* get cached reread */

	output_latency(totaltime, niter);

	return (0);
}

/*
 * Write a byte to /dev/null
 */
int
do_writedevnull(num_iter, t)
	int num_iter;
	clk_t *t;
{
	register int i;
	char c;

	start();
	for (i = num_iter; i > 0; i--) {
		if (write(fd, &c, 1) != 1) {
			perror("/dev/null");
			exit(1);
		}
	}
	*t = stop(c);

	return (0);
}

/*
 * Install a signal handler
 */
int	caught = 0;

void	handler() { caught++; }
void	handler2() { caught++; }

int
do_sigaction(num_iter, t)
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

	sa.sa_handler = handler;
	sigemptyset(&sa.sa_mask);	/* don't care */
	sa.sa_flags = 0;		/* don't care */

	if (num_iter == 1) {	/* special case for cold cache */
		start();
		sigaction(SIGUSR1, &sa, &old);
		*t = stop();
		sigaction(SIGUSR1, &old, 0);
		return (0);
	}
	/* Start timing */
	start();
	for (i = num_iter/2; i > 0; i--) { /* each loop does 2 installations */
		sigaction(SIGUSR1, &sa, &old);
		sigaction(SIGUSR1, &old, 0);
	}
	*t = stop();

	return(0);
}

/*
 * Call gettimeofday()
 */
int
do_gettimeofday(num_iter, t)
	int num_iter;
	clk_t *t;
{
	register int i;
	struct timeval tv;

	start();
	for (i = num_iter; i > 0; i--) {
		gettimeofday(&tv, NULL);
	}
	*t = stop(tv);

	return (0);
}

/*
 * Call sbrk()
 */
int
do_sbrk(num_iter, t)
	int num_iter;
	clk_t *t;
{
	register int i;
	register void *brkval;

	start();
	for (i = num_iter; i > 0; i--) {
		brkval = sbrk(0);
	}
	*t = stop(brkval);

	return (0);
}

#ifndef NO_RUSAGE
/*
 * Call getrusage()
 */
int
do_getrusage(num_iter, t)
	int num_iter;
	clk_t *t;
{
	register int i;
	struct rusage ru;

	start();
	for (i = num_iter; i > 0; i--) {
		getrusage(RUSAGE_SELF, &ru);
	}
	*t = stop(ru);

	return (0);
}
#endif

/*
 * Call getpid() -- probably cached
 */
int
do_getpid(num_iter, t)
	int num_iter;
	clk_t *t;
{
	register int i;
	register int p = 0;

	start();
	for (i = num_iter; i > 0; i--) {
		p += getpid();
	}
	*t = stop(p);

	return (0);
}
