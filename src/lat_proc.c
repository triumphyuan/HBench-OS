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
 * lat_proc.c - process creation benchmarks
 *
 * Usage: lat_proc [null|simple|sh] [static|dynamic]
 *
 * Based on:
 *	$lmbenchId: lat_proc.c,v 1.5 1995/11/08 01:40:21 lm Exp $
 *
 * $Id: lat_proc.c,v 1.8 1997/06/27 00:33:58 abrown Exp $
 */
char	*id = "$Id: lat_proc.c,v 1.8 1997/06/27 00:33:58 abrown Exp $\n";

#include	"common.c"

#define	PROG_S "/tmp/hello-s"
#define	PROG "/tmp/hello"

/* Worker function */
int do_pcreate();

/*
 * Global variables: these are the parameters required by the worker routine.
 * We make them global to avoid portability problems with variable argument
 * lists and the gen_iterations function
 */
int	type;			/* 1 = null, 2 = simple, 3 = /bin/sh */
int	dynamic;		/* 1 = dynamic; 0 = static */

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
	if (parse_counter_args(&ac, &av) || ac != 4) {
		fprintf(stderr, "usage: %s%s iterations [null|simple|sh]"
			" [static|dynamic]\n", av[0], counter_argstring);
		exit(1);
	}

	/* parse command line parameters */
	niter = atoi(av[1]);
	if (!strcmp(av[2],"sh"))
		type = 3;
	else if (!strcmp(av[2],"simple"))
		type = 2;
	else
		type = 1;
	if (!strcmp(av[3],"static"))
		dynamic = 0;
	else
		dynamic = 1;

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
		niter = gen_iterations(&do_pcreate, clock_multiplier);
		printf("%d\n",niter);
		return (0);
	}

	/*
	 * Take the real data and average to get a result
	 */
	do_pcreate(1, &totaltime);	/* prime caches, etc. */
#else
	niter = 1;
#endif
	do_pcreate(niter, &totaltime);	/* get cached reread */

	output_latency(totaltime, niter);

	return (0);
}

/*
 * Worker functions: does num_iter process creations and times the entire
 * thing.
 */

clk_t null_proc(), simple_proc(), sh_proc();

int
do_pcreate(num_iter, t)
	int num_iter;
	clk_t *t;
{
	switch(type) {
	case 1:			/* null */
		*t = null_proc(num_iter);
		break;
	case 2:			/* simple */
		*t = simple_proc(num_iter);
		break;
	case 3:
		*t = sh_proc(num_iter);
		break;
	default:
		*t = 0;
	}
	return (0);
}

clk_t
null_proc(num_iter)
	int num_iter;
{
	register int pid, i;

	/* start timing */
	start();

	for (i = num_iter; i > 0; i--) {
		switch (pid = fork()) {
		    case -1:
			perror("fork");
			exit(1);

		    case 0:	/* child */
			exit(1);

		    default:
			while (wait(0) != pid)
				;
		}
	}
	return (stop(NULL));
}

clk_t
simple_proc(num_iter)
	int num_iter;
{
	int pid, i;
	char	*nav[2];

	nav[0] = (dynamic ? PROG : PROG_S);
	nav[1] = 0;

	start();
	for (i = num_iter; i > 0; i--) {
		switch (pid = fork()) {
		    case -1:
			perror("fork");
			exit(1);

		    case 0: 	/* child */
			close(1);
		    	execve(nav[0], nav, 0);
			exit(1);

		    default:
			while (wait(0) != pid)
				;
		}
	}
	return (stop(NULL));
}

clk_t
sh_proc(num_iter)
	int num_iter;
{
	int pid, i;

	start();
	for (i = num_iter; i > 0; i--) {
		switch (pid = fork()) {
		    case -1:
			perror("fork");
			exit(1);

		    case 0:	/* child */
			close(1);
		    	execlp("/bin/sh", "sh", "-c",
			       (dynamic ? PROG : PROG_S), 0);
			exit(1);

		    default:
			while (wait(0) != pid)
				;
		}
	}
	return (stop(NULL));
}
