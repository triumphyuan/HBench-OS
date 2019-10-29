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
 * bw_file_rd.c - time reading & summing of a file
 *
 * Usage: bw_file_rd size_to_read read_increment file
 *
 * The intent is that the file is in memory, so cold cache numbers taken
 * with this benchmark are not very useful, as they include disk timing.
 * Disk benchmarking should be done with hbdd.
 *
 * Based on:
 *	$lmbenchId: bw_file_rd.c,v 1.2 1995/03/11 02:19:56 lm Exp $
 */
char	*id = "$Id: bw_file_rd.c,v 1.4 1997/06/27 00:33:58 abrown Exp $\n";

#include "common.c"

#include <unistd.h>
#include <stdlib.h>

#define	CHK(x)		if ((int)(x) == -1) { perror("x"); exit(1); }
#define	MIN(a, b)	((a) < (b) ? (a) : (b))

/* 
 * The worker function. We don't really need it here; it is just to make 
 * the structure parallel the other tests.
 */
int 	do_fileread();

/*
 * Global variables: these are the parameters required by the worker routine.
 * We make them global to avoid portability problems with variable argument
 * lists and the gen_iterations function 
 */

unsigned int 	bytes;		/* the number of bytes to be read */
unsigned int	bufsize;
int		fd;		/* file descriptor of open file */

int
main(ac, av)
	int ac;
	char  **av;
{
	clk_t		totaltime;
	int		niter;

	/* print out RCS ID to stderr*/
	fprintf(stderr, "%s", id);

	/* Check command-line arguments */
	if (parse_counter_args(&ac, &av) || ac != 5) {
		fprintf(stderr, "Usage: %s%s ignored sizetoread readincrement file\n", 
			av[0], counter_argstring);
		exit(1);
	}
	
	/* parse command line parameters */
	niter = atoi(av[1]);
	bytes = parse_bytes(av[2]);
	bufsize = parse_bytes(av[3]);
	CHK(fd = open(av[4], 0));
	
	/* Get the number of iterations */
	if (niter == 0) {
		/* We always do 1 iteration here */
		printf("1\n");
		return (0);
	}

	/* initialize timing module (calculates timing overhead, etc) */
	init_timing();

	/*
	 * Take the real data
	 */
#ifndef COLD_CACHE
	do_fileread(1, &totaltime);	/* prime the cache */
#endif
	do_fileread(1, &totaltime);	/* get cached reread */
	output_bandwidth(bytes, totaltime);
	
	return (0);
}

/*
 * This function does all the work. It reads "bytes" from "fd" "num_iter"
 * times and reports the total time in whatever unit our clock is using.
 *
 * Returns 0 if the benchmark was successful, and -1 if there were too many
 * iterations.
 */
int
do_fileread(num_iter, t)
	int num_iter;
	clk_t *t;
{
	/*
	 * 	Global parameters 
	 *
	 * unsigned int bytes;
	 * unsigned int bufsize;
	 * int fd;
	 */
	register unsigned int sum, *p;
	unsigned int j, size;
	int n;
	char *buf = (char *) malloc(bufsize);
	
	if (!buf) {
		perror("malloc");
		exit(1);
	}
	bzero(buf, bufsize);

	/* We first rewind the file */
	lseek(fd, 0, SEEK_SET);

#define	SIXTEEN	sum += p[0]+p[1]+p[2]+p[3]+p[4]+p[5]+p[6]+p[7]+p[8]+p[9]+ \
		p[10]+p[11]+p[12]+p[13]+p[14]+p[15]; p += 16;
#define	SIXTYFOUR	SIXTEEN SIXTEEN SIXTEEN SIXTEEN

	/* 
	 * Now we do the real work 
	 */
	sum = 0;
	size = bytes * num_iter;

	start();		/* start the clocks */
	while (size > 0) {
		CHK(n = read(fd, buf, bufsize));
		if (n < bufsize) {
			break;
		}
		for (p=(unsigned int*)buf, j=bufsize/1024; j > 0; j--) {
			/*
			 * This assumes that sizeof(int) == 4
			 */
			SIXTYFOUR
			SIXTYFOUR
			SIXTYFOUR
			SIXTYFOUR	/* 256 * 4 = 1K; * 64 in loop = 64K */
		}
		size -= n;
	}
	*t = stop(sum);		/* stop the clocks, return the value */
	
	free(buf);
	return (0);		/* success */
}

