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
 * lat_mem_rd.c - measure memory load latency
 *
 * usage: lat_mem_rd clk_ns nloops path freemem stride [stride ...]
 *
 * Based on:
 *	$lmbenchID: lat_mem_rd.c,v 1.1 1994/11/18 08:49:48 lm Exp $
 *
 * $Id: lat_mem_rd.c,v 1.8 1997/06/27 00:33:58 abrown Exp $
 */
char	*id = "$Id: lat_mem_rd.c,v 1.8 1997/06/27 00:33:58 abrown Exp $\n";

#define	LOWER	512

#include	"common.c"

#include	<stdio.h>
#include	<fcntl.h>

int 	step(int k);
int 	do_loads();

#if !defined(FILENAME_MAX) || FILENAME_MAX < 255
#undef FILENAME_MAX
#define FILENAME_MAX 255
#endif
char	fname[FILENAME_MAX];	/* output filename */

/*
 * Global parameters to do_loads
 */
char 	*addr = NULL;
int	range = 0;
int	stride = 0;
float	clk = 1.0;

int
main(ac, av)
	int	ac;
        char  	**av;
{
        int     len;
	int	i, rlen;
	char   *path;
	clk_t	totaltime;
	clk_t	tmp, result;
	int	fd;
	int 	niter, nloops;
	int 	j;

	/* print out RCS ID to stderr*/
	fprintf(stderr, "%s", id);

	/* Check command-line arguments */
	if (parse_counter_args(&ac, &av) || ac < 6) {
		fprintf(stderr, "usage: %s%s clk_ns nloops outputpath memsize "
			"stride [stride ...]\n",
			av[0], counter_argstring);
		exit(1);
	}

	/* parse command line parameters */
	clk = atof(av[1]);
	nloops = atoi(av[2]);
	path = av[3];

        len = parse_bytes(av[4]);

	/* Get memory */
        addr = (char *)malloc(len);
	if (!addr) {
		perror("malloc");
		exit(1);
	}

	sprintf(fname,"%d",len);
	rlen = strlen(fname);

	for (i = 5; i < ac; ++i) {
		stride = atoi(av[i]);
		for (range = LOWER; range <= len; range = step(range)) {
			/* Open the output file: rd_size_stride */
			if (path[strlen(path) - 1] != '/')
				sprintf(fname,"%s/rd_%0.*d_%05d",
					path,rlen,range,stride);
			else
				sprintf(fname,"%srd_%0.*d_%05d",
					path,rlen,range,stride);

			fd = open(fname, O_CREAT|O_APPEND|O_WRONLY, 0666);
			if (fd == -1) {
				fprintf(stderr, "can't open file %s: ",fname);
				perror("error");
				exit(1);
			}

			/* Calculate the number of iterations for 1/2 second */
			niter = gen_iterations(&do_loads,
					       clock_multiplier*2.0);

			niter = niter/1000; /* round down to 1000's */
			niter *= 1000;

			for (j = nloops; j > 0; j--) {
				do_loads(niter, &totaltime);

				/*
				 * We want to get to nanoseconds / load.  We
				 * don't want to lose any precision in the
				 * process.  What we have is the milliseconds
				 * it took to do niter loads
				 *
				 * We want just the memory latency time, not
				 * including the time to execute the load
				 * instruction.  We allow one clock for the
				 * instruction itself.  So we need to subtract
				 * off niter * clk nanoseconds.
				 *
				 * We account for loop overhead below.
				 */
#if defined(CYCLE_COUNTER)
				/*
				 * Easy case: totaltime is already in cycles,
				 * so just knock off the right amount.
				 */
				result = totaltime - (clk_t)niter;
#else
				/*
				 * Since we have no counters, totaltime is in
				 * microseconds. Assume clock_multiplier is 1.
				 */

				/* Compute the overhead, in microseconds */
				tmp = (clk_t)(clk * (float)niter)/(clk_t)1000;

				if (clock_multiplier != 1.0)
					result = (clk_t)((float)totaltime*clock_multiplier);
				else
					result = totaltime;

				result -= tmp; /* remove overhead */
#endif
				output_latency_ns_fd(result, niter, fd);
			}
			close(fd);
		}
	}

	/* free the memory */
	free(addr);

	exit(0);
}

int
do_loads(num_iter, t)
	int num_iter;
	clk_t *t;
{
	register char **p;
        register int i;
	clk_t	ovr;
	/*
	  char	*addr;
	  int	stride;
	  float	clk;
	*/

        /*
	 * First create a list of pointers.
	 */
     	if (stride & (sizeof(char *) - 1)) {
		fprintf(stderr, "list: stride must be aligned.\n");
		return 1;
	}

     	for (i = 0; i < range; i += stride) {
		char	*next;

		p = (char **)&addr[i];
		if (i + stride >= range) {
			next = &addr[0];
		} else {
			next = &addr[i + stride];
		}
		*p = next;
	}

	/*
	 * Now walk them and time it.
	 */
#define	ONE	p = (char **)*p;
#define	FIVE	ONE ONE ONE ONE ONE
#define	TEN	FIVE FIVE
#define	FIFTY	TEN TEN TEN TEN TEN
#define	HUNDRED	FIFTY FIFTY

	i = num_iter;
	p = (char **)addr;
	start();
	while (i > 0) {
		HUNDRED
		HUNDRED
		HUNDRED
		HUNDRED
		HUNDRED
		HUNDRED
		HUNDRED
		HUNDRED
		HUNDRED
		HUNDRED
		i -= 1000;
	}
	*t = stop(p);

	/*
	 * Calculate and remove loop overhead
	 */
	i = num_iter;
	start();
	while(i > 0) {
		i -= 1000;
	}
	ovr = stop(i);
	*t -= ovr;

	return(0);
}

int
step(int k)
{
	if (k < 1024) {
		k = k * 2;
        } else if (k < 4*1024) {
		k += 1024;
        } else if (k < 32*1024) {
		k += 2048;
        } else if (k < 64*1024) {
		k += 4096;
        } else if (k < 128*1024) {
		k += 8192;
        } else if (k < 256*1024) {
		k += 16384;
        } else if (k < 512*1024) {
		k += 32*1024;
	} else {
		k += 512 * 1024;
	}
	return (k);
}
