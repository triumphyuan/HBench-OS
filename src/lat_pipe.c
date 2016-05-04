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
 * lat_pipe.c - pipe transmission latency benchmark
 *
 * Based on lmbench, file
 * 	$lmbenchId: lat_pipe.c,v 1.1 1994/11/18 08:49:48 lm Exp $
 *
 * $Id: lat_pipe.c,v 1.4 1997/06/27 00:33:58 abrown Exp $
 *
 */
char	*id = "$Id: lat_pipe.c,v 1.4 1997/06/27 00:33:58 abrown Exp $\n";

#include "common.c"

/* Worker function */
int do_pipe();

int
main(ac, av)
	int ac;
	char **av;
{
	clk_t		totaltime;
	unsigned int	niter;

	/* print out RCS ID to stderr*/
	fprintf(stderr, "%s", id);

	/* Check command-line arguments */
	if (parse_counter_args(&ac, &av) || ac != 2) {
		fprintf(stderr, "usage: %s%s iterations\n", av[0],
			counter_argstring);
		exit(1);
	}

	/* parse command line parameters */
	niter = atoi(av[1]);

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
		niter = gen_iterations(&do_pipe, clock_multiplier);
		printf("%d\n",niter);
		return (0);
	}

	/*
	 * Take the real data and average to get a result
	 */
	do_pipe(1, &totaltime);	/* prime caches, etc. */
#else
	niter = 1;
#endif
	do_pipe(niter, &totaltime);	/* get cached reread */

	output_latency(totaltime, niter);

	return (0);
}

/*
 * Worker function: does num_iter writes through the pipe
 */
int
do_pipe(num_iter, t)
	int num_iter;
	clk_t *t;
{
	int	p1[2], p2[2];
	int	i;
	char	c;
	int	pid;

     	if (pipe(p1) == -1 || pipe(p2) == -1) {
		perror("pipe");
		exit(1);
	}

	pid = fork();
	if (pid == -1) {
		perror("fork");
		exit(1);
	}
	if (pid > 0) {		/* parent */
		/*
		 * One time around to make sure both processes are started.
		 */
		if (write(p1[1], &c, 1) != 1 ||
		    read(p2[0], &c, 1) != 1 ||
		    write(p1[1], &c, 1) != 1) {
			perror("read/write on pipe");
			exit(1);
		}

		/* Start timing */
		start();
		for (i = num_iter; i > 0; i--) {
			if (read(p2[0], &c, 1) != 1 ||
			    write(p1[1], &c, 1) != 1) {
				perror("read/write on pipe");
				exit(1);
			}
		}
		*t = stop(NULL);

		kill(pid, 15);
	} else {		/* child */
		for ( ;; ) {
			if (read(p1[0], &c, 1) != 1 ||
			    write(p2[1], &c, 1) != 1) {
				perror("read/write on pipe");
				exit(1);
			}
		}
		exit(0);
	}
	return (0);
}
