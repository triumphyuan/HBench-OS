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
 * bw_pipe.c - pipe bandwidth benchmark.
 *
 * Usage: bw_pipe transfersize
 *
 * Based on:
 *	$lmbenchId: bw_pipe.c,v 1.1 1994/11/18 08:49:48 lm Exp $
 *
 * $Id: bw_pipe.c,v 1.4 1997/06/27 00:33:58 abrown Exp $
 */
char	*id = "$Id: bw_pipe.c,v 1.4 1997/06/27 00:33:58 abrown Exp $\n";

#include "common.c"

extern	void	exit();		/* for lint on SunOS 4.x */
void		writer(), reader();

/* The worker function */
int 	do_pipexfer(int num_iter, clk_t *time);

/*
 * Global variables: these are the parameters required by the worker routine.
 * We make them global to avoid portability problems with variable argument
 * lists and the gen_iterations function
 */
unsigned int 	bufsize;	/* the size of the request buffer */

/* Default transfer: 4MB. Thus even on fast machines we bypass caches */
#define XFERUNIT	(4*1024*1024)

int
main(ac, av)
	int ac;
	char **av;
{
	unsigned int	niter;
	clk_t		totaltime;

	/* print out RCS ID to stderr*/
	fprintf(stderr, "%s", id);

	/* Check command-line arguments */
	if (parse_counter_args(&ac, &av) || ac != 3) {
		fprintf(stderr, "Usage: %s%s iterations transfersize\n",
			av[0], counter_argstring);
		exit(1);
	}

	/* parse command line parameters */
	niter = atoi(av[1]);
	bufsize = parse_bytes(av[2]);

	/* initialize timing module (calculates timing overhead, etc) */
	init_timing();

#ifndef COLD_CACHE
	/*
	 * Generate the appropriate number of iterations so the test takes
	 * at least one second. For efficiency, we are passed in the expected
	 * number of iterations, and we return it via the process error code.
	 * No attempt is made to verify the passed-in value; if it is 0, we
	 * we recalculate it and print it, then exit.
	 */
	if (niter == 0) {
		niter = gen_iterations(&do_pipexfer, clock_multiplier);
		printf("%d\n",niter);
		return (0);
	}

	/*
	 * Take the real data and average to get a result
	 */
	do_pipexfer(1, &totaltime); /* prime caches */
#else
	niter = 1;
#endif
	do_pipexfer(niter, &totaltime);	/* get pipe bandwidth */

	output_bandwidth(niter * XFERUNIT, totaltime);

	return (0);
}

/*
 * This function does all the work. It transfers XFERUNIT through the pipe
 * num_iter times, timing the entire operation.
 *
 * Returns 0 if the benchmark was successful.
 */
int
do_pipexfer(num_iter, t)
	int num_iter;
	clk_t *t;
{
	/*
	 * 	Global parameters
	 *
	 * unsigned int bufsize;
	 */
	int		pipes[2];
	unsigned int	todo, n, done = 0;
	char 		*buf;
	int		termstat;

	if (pipe(pipes) == -1) {
		perror("pipe");
		exit(1);
	}

	/* Amount to transfer */
	todo = XFERUNIT * num_iter;

	/* Allocate buffer */
	buf = (char *) malloc(bufsize);
	if (buf == NULL) {
		perror("malloc");
		exit(1);
		/* NOTREACHED */
	}

	/* Spawn off a writer, then time the read */
	switch (fork()) {
	case 0:			/* writer */
		while ((done < todo) &&
		       ((n = write(pipes[1], buf, bufsize)) > 0))
			done += n;
		exit(0);
		/*NOTREACHED*/

	case -1:
		perror("fork");
		exit(1);
		/*NOTREACHED*/

	default:		/* reader */
		/* wait for writer */
		sleep(1);

		start();	/* start timing */
		while ((done < todo) &&
		       ((n = read(pipes[0], buf, bufsize)) > 0))
			done += n;
		*t = stop(NULL);	/* stop timing */

		wait(&termstat); /* wait for writer to exit */
	}

	free(buf);		/* release memory */

	return (0);
}
