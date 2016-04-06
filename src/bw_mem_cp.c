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
 * bw_mem_cp.c - measures copy bandwidth delivered by the memory system
 *
 * Usage: bw_mem_cp size libc|unrolled aligned|unaligned
 *
 * Measures both unrolled (simplistic) and library (general) copy
 * times of aligned & unaligned data.  Aligned here means that the
 * source and destination are aligned to page boundries, not that
 * the pointers are word aligned.
 * The copy does *not* include the cost of an add, and thus may not be
 * directly comparable to the memory read and write bandwidth routines.
 *
 * Based on:
 *	$lmbenchId: bw_mem_cp.c,v 1.2 1995/03/11 02:19:56 lm Exp $
 *
 * $Id: bw_mem_cp.c,v 1.7 1997/06/27 00:33:58 abrown Exp $
 */
char	*id = "$Id: bw_mem_cp.c,v 1.7 1997/06/27 00:33:58 abrown Exp $\n";

#include "common.c"

/*
 * Not all machines have spiffy 64-bit operations (notably the Intel x86's).
 * On the x86, gcc generates slower code with 64-bit copies than with 
 * 32-bit copies (due to data dependencies in the pipeline), so we use 32-bit 
 * ints instead of doubles. Using ints also should allow us to use the 
 * optimal/native word size
 */
#ifndef TYPE
#define TYPE	unsigned int
#endif
#ifndef SIZE
#define	SIZE	sizeof(TYPE)
#endif

/* The worker function */
int 	do_copy(int num_iter, clk_t *time);

/*
 * Global variables: these are the parameters required by the worker routine.
 * We make them global to avoid portability problems with variable argument
 * lists and the gen_iterations function 
 */
unsigned int 	bytes;		/* the number of bytes to be read */
int		libc;		/* 1 if libc bcopy (i.e. not unrolled) */
int		aligned;	/* 1 if aligned */

void	unrolled();

main(ac, av)
        char  **av;
{
	unsigned int	niter;
	clk_t		totaltime;
	unsigned int	xferred;

	/* print out RCS ID to stderr*/
	fprintf(stderr, "%s", id);

	/* Check command-line arguments */
	if (parse_counter_args(&ac, &av) || ac != 5) {
		fprintf(stderr, "Usage: %s%s iterations size libc|unrolled aligned|unaligned\n", 
			av[0], counter_argstring);
		exit(1);
	}
	
	/* parse command line parameters */
	niter = atoi(av[1]);
	bytes = parse_bytes(av[2]);
	if (strcmp(av[3], "libc") == 0)
		libc = 1;
	else
		libc = 0;
	if (strcmp(av[4], "aligned") != 0)
		aligned = 0;
	else
		aligned = 1;
	
	/*
	 * The gory calculation on the next line computes the actual number of
	 * bytes tranferred by the unrolled loop.
	 */
	if (libc)
		xferred = bytes;
	else
		xferred = (16*SIZE) * (bytes / (16 * SIZE));
	if (xferred == 0) {
		fprintf(stderr, "error: buffer size too small: must be at "
			"least %d bytes.\n",16*SIZE);
		printf("<error>\n");
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
		niter = gen_iterations(&do_copy, clock_multiplier);
		printf("%d\n",niter);
		return (0);
	}
	/*
	 * Take the real data and average to get a result
	 */
	do_copy(1, &totaltime);		/* prime the cache with 1 iteration*/
#else
	niter = 1;
#endif
	do_copy(niter, &totaltime);	/* get cached copy */

	output_bandwidth(niter * xferred, totaltime);
	
	return (0);
}

/*
 * This function does all the work. It mallocs two buffers of the appropriate
 * size and repeatedly copies one to the other, timing the entire operation.
 *
 * Returns 0 if the benchmark was successful, and -1 if there were too many
 * iterations.
 */
int
do_copy(num_iter, t)
	int num_iter;
	clk_t *t;
{
	/*
	 * 	Global parameters 
	 *
	 * unsigned int bytes;
	 * int 		aligned;
	 * int		libc;
	 */
        int     i;
        TYPE   *src, *dst, *savsrc;
	unsigned long tmp;

	/* Allocate memory */
        savsrc = src = (TYPE *)malloc(2*(bytes + 16384));
	dst = (TYPE *)(((char *)src)+bytes+16384);
	if (!src || !dst) {
		perror("malloc");
		exit(1);
	}

	/*
	 * Page-align the two regions (assumes <=8K pagesize)
	 */
	tmp = (unsigned long)src;
	tmp += 8192;
	tmp &= ~8191;
	src = (TYPE *)tmp;
	tmp = (unsigned long)dst;
	tmp += 8192;
	tmp &= ~8191;
	if (!aligned)
		tmp += 128;
	dst = (TYPE *)tmp;

	/* Touch the memory to initialize the VM mappings */
#ifndef COLD_CACHE
	bzero(src, bytes);
	bzero(dst, bytes);
#endif

	/* Do the copy measurement: copy num_iter times and time */
	if (libc) {
		start();
		for (i = num_iter; i > 0; i--) {
			bcopy(src, dst, bytes);
		}
		*t = stop(dst);
	} else {
		start();
		for (i = num_iter; i > 0; i--) {
			unrolled(src, dst, bytes);
		}
		*t = stop(dst);
	}
	
	/* Release allocated memory */
	free(savsrc);

	return (0);
}

/*
 * XXX - Neal said that this could be made faster if I did all the loads
 * then all the stores.  Think about this.  It doesn't hold true on a ss2.
 */
void
unrolled(src, dst, numbytes)
	TYPE	*src, *dst;
	int	numbytes;
{
	/*
	 * In some processors, we can avoid stalling the pipeline with
	 * data dependencies by using array-index data accesses rather
	 * than pointer chasing. In addition, this is how the raw
	 * read/write bandwidths are measured, so we get
	 * consistant/comparable numbers by using array-indexing.
	 */
	while (numbytes >= 16 * SIZE) {
		dst[0] = src[0];
		dst[1] = src[1];
		dst[2] = src[2];
		dst[3] = src[3];
		dst[4] = src[4];
		dst[5] = src[5];
		dst[6] = src[6];
		dst[7] = src[7];
		dst[8] = src[8];
		dst[9] = src[9];
		dst[10] = src[10];
		dst[11] = src[11];
		dst[12] = src[12];
		dst[13] = src[13];
		dst[14] = src[14];
		dst[15] = src[15];
		dst += 16;
		src += 16;
		numbytes -= 16 * SIZE;
	}
}
