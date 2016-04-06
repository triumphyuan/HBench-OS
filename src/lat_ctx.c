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
 * lat_ctx.c - measures pure context switch latency, without cache conflict 
 *             resolution overhead
 *
 * usage: lat_ctx niter footprint #procs 
 *
 * Based on:
 *	$lmbenchId: lat_ctx.c,v 1.3 1995/10/26 04:03:09 lm Exp $
 *
 * $Id: lat_ctx.c,v 1.8 1997/06/27 00:33:58 abrown Exp $
 */
char	*id = "$Id: lat_ctx.c,v 1.8 1997/06/27 00:33:58 abrown Exp $\n";

#include "common.c"
#include <sys/mman.h>
#include <fcntl.h>

#define MAX_PROCS		128

#define OVERHEADAVG_LOOPS	20
#define OVERHEADAVG_TAILS	0.2

/* Worker functions */
int 	do_ctxsw();
int 	do_overhead1();
int 	do_overhead2();
int	sumit();
void	killem(), child();

/*
 * Global variables: these are the parameters required by the worker routine.
 * We make them global to avoid portability problems with variable argument
 * lists and the gen_iterations function 
 */
int	nprocs;			/* number of processes */
int	sprocs;			/* size of each process */
pid_t	pids[MAX_PROCS];	/* process ID's */
int	*pbuffer;		/* memory buffer for procs to sum */
int	*locdata;		/* proc's memory buffer for procs to sum */

int
main(ac, av)
	int ac;
	char **av;
{
	clk_t		totaltime, overhead;
	unsigned int	niter, tmp;

	/* print out RCS ID to stderr*/
	fprintf(stderr, "%s", id);

	/* Check command-line arguments */
	if (parse_counter_args(&ac, &av) || ac != 4) {
		fprintf(stderr, 
			"usage: %s%s iterations footprint nprocs\n",
			av[0], counter_argstring);
		exit(1);
	}

	/* parse command line parameters */
	niter = atoi(av[1]);
	sprocs = parse_bytes(av[2]);
	nprocs = atoi(av[3]);
	
	if (nprocs > MAX_PROCS) {
		fprintf(stderr, "%s: maximum number of processes (%d) "
			"exceeded\n",MAX_PROCS);
		exit(1);
	}

	/* initialize timing module (calculates timing overhead, etc) */
	init_timing();

	/*
	 * Mmap the data array
	 */
	if (sprocs != 0) {
#if defined(MAP_ANON)
		pbuffer = (int *)mmap(0, sprocs*nprocs,PROT_READ|PROT_WRITE,
				      MAP_ANON|MAP_SHARED, -1, 0);
#elif defined(MAP_ANONYMOUS)
		pbuffer = (int *)mmap(0, sprocs*nprocs,PROT_READ|PROT_WRITE,
				      MAP_ANONYMOUS|MAP_SHARED, -1, 0);
#else 
		int fd0 = open("/dev/zero",O_RDWR);
		if (fd0 == -1) {
			perror("open");
			exit(1);
		}
#if defined(MAP_FILE)
		pbuffer = (int *)mmap(0, sprocs*nprocs,PROT_READ|PROT_WRITE,
				      MAP_FILE|MAP_SHARED, fd0, 0);
#else
		pbuffer = (int *)mmap(0, sprocs*nprocs,PROT_READ|PROT_WRITE,
				      MAP_SHARED, fd0, 0);
#endif
#endif
#ifdef MAP_FAILED
		if ((void *)pbuffer == MAP_FAILED) {
#else
		if ((int)pbuffer == -1) {
#endif
			perror("mmap");
			exit(1);
		}
	}
	
#ifndef COLD_CACHE
	if (niter == 0) {
		/*
		 * Now we need to determine how many iterations of context
		 * switching are necessary to make up one second.
		 */
		niter = gen_iterations(&do_ctxsw, clock_multiplier);
		
		/* Now we output the result */
		printf("%d\n", niter);
		return (0);
	}
#else
	niter = 1;
#endif

#ifdef NOOVERHEAD		/* for internal use only! */
	overhead = 0;
#else
	/* 
	 * We must first measure the overhead of passing a token around
	 * an array in a single process ("pipe overhead").
	 *
	 * We find out how long this takes with do_overhead1,
	 * and then use do_overhead2 to take a
	 * statistically-more-appropriate measure.
	 */
	tmp = gen_iterations(&do_overhead1, clock_multiplier);
	
	/* Use only 1/2 second to make running time reasonable */
	tmp >>= 1;
	do_overhead2(tmp, &overhead);
#endif

	/*
	 * We know the overhead and the number of iterations; take the real
	 * data.
	 */
#ifndef COLD_CACHE
	do_ctxsw(1, &totaltime);		/* prime caches */
#endif
	do_ctxsw(niter, &totaltime);		/* get total ctxsw time */

	totaltime -= overhead * (clk_t)(niter*nprocs); /* remove overhead */
	output_latency(totaltime, niter*nprocs);

	if (sprocs != 0)
		munmap((char *)pbuffer, nprocs*sprocs);

	return (0);
}

/*
 * Worker function #1: measures time to pass a token through nprocs pipes
 * in a single process with a large array of size nprocs*sprocs.
 *
 * The time returned is the time for num_iter iterations of passing the
 * token through all nprocs pipes, summing nprocs times. So the time for
 * a single pipe/sum (i.e. overhead for 1 context switch) is 
 * 	*t/(num_iter*nprocs)
 */
int
do_overhead1(num_iter, t)
	int num_iter;
	clk_t *t;
{
	int	p[MAX_PROCS][2];
	int	msg = 0, sum, i, n, k;
	char 	*data, *where;

	/*
	 * Get the data array 
	 */
	if (sprocs != 0) {
		where = data = (char *) pbuffer;
		bzero(data,nprocs*sprocs);
	}

	/*
	 * Get nprocs pipes
	 */
	n = 0;
	while (n < nprocs && pipe(p[n]) != -1)
		n++;

	/*
	 * Prime the pipes and make sure we can use them
	 */
	for (i = 0; i < n; i++) {
		if (write(p[i][1], &msg, sizeof(msg)) != sizeof(msg)) {
			perror("write on pipe in overhead calculation");
			exit(1);
		}
		if (read(p[i][0], &msg, sizeof(msg)) != sizeof(msg)) {
			perror("read on pipe in overhead calculation");
			exit(1);
		}
	}
	write(p[k = 0][1], &msg, sizeof(msg)); /* start the token */
	
	/*
	 * Now do the loop
	 */
	start();
	for (i = num_iter; i > 0; i--) {
		/* Send a byte through all the pipes, summing an array */
		do {
			read(p[k][0], &msg, sizeof(msg));

			sum += sumit(where);
			where += sprocs;

			if (++k == n) {
				k = 0;
				where = data;
			}

			write(p[k][1], &msg, sizeof(msg));
		} while (k != 0);
	}
	*t = stop(sum);

	/*
	 * Get rid of resources: pipes and memory	
	 */
	for (i = n-1; i >= 0; i--) {
		close(p[i][0]);
		close(p[i][1]);
	}

	return (0);
}

/*
 * Worker function #2: gathers the average of the middle 16 values of 20
 * iterations of do_overhead1().
 */
int
do_overhead2(num_iter, t)
	int num_iter;
	clk_t *t;
{
	int 	i;
	clk_t	val;

	centeravg_reset(OVERHEADAVG_LOOPS, OVERHEADAVG_TAILS);

	for (i = OVERHEADAVG_LOOPS; i > 0; i--) {
		do_overhead1(num_iter, &val);
		centeravg_add(val/(nprocs*num_iter)); /* get 1-sw overhead */
	}
	
	centeravg_done(t);

	return (0);
}

/*
 * Worker function #3: does the actual context switch test.
 *
 * *t gets the time for num_iter trips through all the processes.
 */
int
do_ctxsw(num_iter, t)
	int num_iter;
	clk_t *t;
{
	int 	p[MAX_PROCS][2];
	int 	msg = 0, i, sum;
	
	locdata = pbuffer;

	/*
	 * Get the pipes
	 */
	for (i = 0; i < nprocs; ++i) {
		if (pipe(p[i]) == -1) {
			perror("pipe in ctxsw");
			exit(1);
		}
	}

	/*
	 * Use the pipes as a ring, and fork off a bunch of processes
	 * to pass the byte through their part of the ring.
	 */
	signal(SIGTERM, SIG_IGN);
     	for (i = 1; i < nprocs; ++i) {
		switch (pids[i] = fork()) {
		    case -1: 
			perror("fork");
			killem(nprocs);
			exit(1);
			
		    case 0:	/* child */
			locdata = (int *)(((char *)locdata) + (sprocs * i));
			child(p, i-1, i);
			/* NOTREACHED */

		    default:	/* parent */
		    	;
	    	}
	}

	/*
	 * Go once around the loop to make sure that everyone is ready and
	 * to get the token in the pipeline.
	 */
	if (write(p[0][1], &msg, sizeof(msg)) != sizeof(msg) ||
	    read(p[nprocs-1][0], &msg, sizeof(msg)) != sizeof(msg) ||
	    write(p[0][1], &msg, sizeof(msg)) != sizeof(msg)) {
		perror("write/read/write on pipe");
		exit(1);
	}
	bzero(locdata, sprocs);	/* make sure we have our own copy */

	/*
	 * Main process - all others should be ready to roll, time the
	 * loop.
	 */
	start();
	for (i = num_iter; i--; ) {
		read(p[nprocs-1][0], &msg, sizeof(msg));

		sum = sumit(locdata);

	    	write(p[0][1], &msg, sizeof(msg));
	}
	*t = stop();

	/*
	 * Close the pipes and kill the children.
	 */
     	killem(nprocs);
     	for (i = 0; i < nprocs; ++i) {
		close(p[i][0]);
		close(p[i][1]);
		if (i > 0) {
			wait(0);
		}
	}
	return (0);
}

/*
 * Worker function #4: the code run by each child in the ring of pipes
 */
void
child(p, rd, wr)
	int	p[MAX_PROCS][2];
	int	rd, wr;
{
	int	msg, sum;

	signal(SIGTERM, SIG_DFL);
	bzero(locdata, sprocs);	/* make sure we have our own copy */
	for ( ;; ) {
		read(p[rd][0], &msg, sizeof(msg));
		sum = sumit(locdata);
		write(p[wr][1], &msg, sizeof(msg));
	}
}


/****************/

/*
 * Miscellaneous helpers
 */

void
killem(procs)			/* kill all children */
	int	procs;
{
	int	i;

	for (i = 1; i < procs; ++i) {
		if (pids[i] > 0) {
			kill(pids[i], SIGTERM);
		}
	}
}

/*
 * sumit: unrolled loop to sum an array
 */
int
sumit(data)
	char *data;
{
	int	i, sum = 0;
	int	*d = (int *)data;

#define	TEN	sum+=d[0]+d[1]+d[2]+d[3]+d[4]+d[5]+d[6]+d[7]+d[8]+d[9];d+=10;
#define	FIFTY	TEN TEN TEN TEN TEN
#define	HUNDRED	FIFTY FIFTY
#define	HALFK	HUNDRED HUNDRED HUNDRED HUNDRED HUNDRED TEN sum+=*d++;sum+=*d++;

	for (i = sprocs/sizeof(int); i > 512; i -= 512) {
		HALFK
	}
	return (sum);
}

