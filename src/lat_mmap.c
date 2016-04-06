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
 * lat_mmap.c - time how fast a mapping can be made and broken down
 *
 * Usage: lat_mmap size file
 *
 * XXX - If an implementation did lazy address space mapping, this test
 * will make that system look very good.
 *
 * Based on:
 *	$lmbenchId: lat_mmap.c,v 1.4 1995/10/26 01:03:42 lm Exp $
 *
 * $Id: lat_mmap.c,v 1.7 1997/06/27 00:33:58 abrown Exp $
 */
char	*id = "$Id: lat_mmap.c,v 1.7 1997/06/27 00:33:58 abrown Exp $\n";

#include "common.c"

#include <sys/stat.h>
#include <sys/mman.h>

#define	CHK(x)		if ((int)(x) == -1) { perror("x"); exit(1); }

/* Worker function */
int do_mmap();

/*
 * Global variables: these are the parameters required by the worker routine.
 * We make them global to avoid portability problems with variable argument
 * lists and the gen_iterations function 
 */
int		fd;		/* file descriptor of file to map */
unsigned int 	size;		/* size of region to map */

int
main(ac, av)
	int ac;
	char **av;
{
	clk_t		totaltime;
	unsigned int	niter;
	struct stat sbuf;

	/* print out RCS ID to stderr*/
	fprintf(stderr, "%s", id);

	/* Check command-line arguments */
	if (parse_counter_args(&ac, &av) || ac != 4) {
		fprintf(stderr, "usage: %s%s iterations size file\n", 
			av[0], counter_argstring);
		exit(1);
	}

	/* parse command line parameters */
	niter = atoi(av[1]);
	size = parse_bytes(av[2]);
	CHK(fd = open(av[3], 0));
	CHK(fstat(fd, &sbuf));
	if (sbuf.st_size < size) {
		fprintf(stderr, "%s: file %s is not as big as size %d\n",
		    av[0], av[3], size);
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
		niter = gen_iterations(&do_mmap, clock_multiplier);
		printf("%d\n",niter);
		return (0);
	}

	/*
	 * Take the real data and average to get a result
	 */
	do_mmap(1, &totaltime);	/* prime caches, etc. */
#else
	niter = 1;
#endif
	do_mmap(niter, &totaltime);	/* get cached reread */
	
	output_latency(totaltime, niter);
	
	return (0);
}

/*
 * Worker function: does num_iter mmap/munmap pairs and times the entire
 * thing.
 */
int
do_mmap(num_iter, t)
	int num_iter;
	clk_t *t;
{
	int i;
	char *where;

	/* Start clocks */
	start();

	for (i = num_iter; i > 0; i--) {
#ifdef	MAP_FILE
		where = mmap(0, size, PROT_READ, MAP_FILE|MAP_SHARED, fd, 0);
#else
		where = mmap(0, size, PROT_READ, MAP_SHARED, fd, 0);
#endif
		if ((int)where == -1) {
			perror("mmap");
			exit(1);
		}
		munmap(where, size);
	}
	*t = stop();

	return (0);
}
