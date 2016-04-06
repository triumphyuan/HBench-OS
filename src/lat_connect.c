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
 * lat_connect.c - simple TCP connection latency test
 *
 * Three programs in one -
 *	server usage:	lat_connect -s
 *	client usage:	lat_connect hostname
 *	shutdown:	lat_connect -hostname
 *
 * The test measures the time to set up a connection and transmit one byte.
 *
 * Based on:
 *	$lmbenchId: lat_connect.c,v 1.3 1995/09/26 05:42:08 lm Exp $
 *
 * $Id: lat_connect.c,v 1.7 1997/06/27 00:33:58 abrown Exp $
 */
char	*id = "$Id: lat_connect.c,v 1.7 1997/06/27 00:33:58 abrown Exp $\n";

#include "common.c"
#include "lib_tcp.c"

/* Worker function */
int do_client();

/*
 * Global variables: these are the parameters required by the worker routine.
 * We make them global to avoid portability problems with variable argument
 * lists and the gen_iterations function 
 */
char 	*server;

int
main(ac, av)
	int ac;
	char  **av;
{
	clk_t		totaltime;
	unsigned int	niter;
	int i;

	/* print out RCS ID to stderr*/
	fprintf(stderr, "%s", id);

	/* Check command-line arguments */
	if (parse_counter_args(&ac, &av) || ac != 3) {
		fprintf(stderr, "Usage: %s%s iterations -s OR"
		   "\n       %s%s iterations [-]serverhost\n",
		    av[0], counter_argstring, av[0], counter_argstring);
		exit(1);
	}

	/* parse command line parameters */
	niter = atoi(av[1]);
	
	if (!strcmp(av[2], "-s")) { /* starting server */
		if (fork() == 0) {
			server_main(ac, av);
		}
		exit(0);
	}

	/* Starting client */
	server = av[2][0] == '-' ? &av[2][1] : av[2];

	/* Stop server request */
	if (av[2][0] == '-') {
		int sock;
		sock = tcp_connect(server, TCP_CONNECT, SOCKOPT_NONE);
		write(sock, "0", 1);
		close(sock);
		exit(0);
		/* NOTREACHED */
	}

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
#ifdef CYCLE_COUNTER
		niter = 1;
#else
#ifdef __GNUC__
#warning No counters defined--results of lat_connect may not be accurate
#endif
/*		niter = gen_iterations(&do_client, clock_multiplier);*/
		niter = 1; /* XXX -- should do better! */
#endif
		printf("%d\n",niter);
		return (0);
	}

	/*
	 * Take the real data and average to get a result
	 */
	do_client(1, &totaltime);	/* get TCP latency */
#else
	niter = 1;
#endif
	do_client(niter, &totaltime);

	output_latency(totaltime, niter);

	return (0);
}

/* 
 * This function does all the work. It repeatedly connects to and disconnects
 * from the remote server, timing the entire operation.
 *
 * Returns 0 if the benchmark was successful, or -1 if there are too many
 * iterations
 */
int
do_client(num_iter, t)
	int num_iter;
	clk_t *t;
{
	/*
	 * 	Global parameters 
	 *
	 * char		*server;
	 */
	int     sock;
	int	i, tmp;
	char	c = 65;

	start();		/* start the clocks */
	for (i = num_iter; i > 0; i--) {
		sock = tcp_connect(server, TCP_CONNECT, SOCKOPT_NONE);
	}
	*t = stop();
	sleep(1);
	close(sock);
	return (0);
}

server_main(ac, av)
	char  **av;
{
	int     newsock, sock;
	char	c;

	GO_AWAY;
	sock = tcp_server(TCP_CONNECT, SOCKOPT_NONE);
	for (;;) {
		newsock = tcp_accept(sock, SOCKOPT_NONE);
		c = 0;
		read(newsock, &c, 1);
		if (c && c == '0') {
			exit(0);
		}
		close(newsock);
	}
	/* NOTREACHED */
}
