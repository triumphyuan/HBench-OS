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
 * lat_tcp.c - TCP transaction (read/write) latency test
 *
 * Three programs in one -
 *	server usage:	lat_tcp -s
 *	client usage:	lat_tcp hostname
 *	shutdown:	lat_tcp -hostname
 *
 * Based on:
 * 	$lmbenchId: lat_tcp.c,v 1.2 1995/03/11 02:25:31 lm Exp $
 *
 * $Id: lat_tcp.c,v 1.4 1997/06/27 00:33:58 abrown Exp $
 */
char	*id = "$Id: lat_tcp.c,v 1.4 1997/06/27 00:33:58 abrown Exp $\n";

#include <sys/wait.h>

#include "common.c"
#include "lib_tcp.c"

/* Worker functions */
int do_client();
void server_main();
void doserver(int sock);

/*
 * Global variables: these are the parameters required by the worker routine.
 * We make them global to avoid portability problems with variable argument
 * lists and the gen_iterations function
 */
char 		*rhostname;	/* hostname of remote host */
int		killserver = 0;	/* flag to tell client to kill server */

int
main(ac, av)
	int ac;
	char  **av;
{
	unsigned int	niter;
	clk_t		totaltime;

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
			server_main();
		}
		exit(0);
	}

	/* Starting client */
	if (av[2][0] == '-') {
		killserver = 1;	/* signal client to kill server */
		rhostname = &av[2][1];
		do_client(1,&totaltime); /* run client to kill server */
		exit(0);	/* quit */
	} else {
		rhostname = av[2];
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
		niter = gen_iterations(&do_client, clock_multiplier);

		printf("%d\n",niter);
		return (0);
	}

	/*
	 * Take the real data and average to get a result
	 */
	do_client(1, &totaltime);	/* prime caches */
#else
	niter = 1;
#endif
	do_client(niter, &totaltime);	/* get TCP latency */

	output_latency(totaltime, niter);

	return (0);
}

/*
 * This function does all the work. It initiates a TCP connection
 * with the server and transfers the requisite amount of data in
 * num_iter transactions.
 */
int
do_client(num_iter, t)
	int num_iter;
	clk_t *t;
{
	/*
	 * 	Global parameters
	 *
	 * char *rhostname;
	 * int killserver;
	 */
	int	sock;
	register int     i;
	char    c;

	/*
	 * Connect to server
	 */
	sock = tcp_connect(rhostname, TCP_XACT, SOCKOPT_NONE);

	/*
	 * Stop server code, if requested.
	 */
	if (killserver) {
		close(sock);
		*t = (clk_t)0;
		return (0);
	}

	start();
	for (i = num_iter; i > 0; i--) {
		write(sock, &c, 1);
		read(sock, &c, 1);
	}
	*t = stop();

	close(sock);

	return (0);
}

void
child(unused)
{
	wait(0);
	signal(SIGCHLD, child);
}

void
server_main()
{
	int     newsock, sock;

	GO_AWAY;
	signal(SIGCHLD, child);
	sock = tcp_server(TCP_XACT, SOCKOPT_NONE);
	for (;;) {
		newsock = tcp_accept(sock, SOCKOPT_NONE);
		switch (fork()) {
		case -1:
			perror("fork");
			break;
		case 0:
			doserver(newsock);
			exit(0);
			break;
		default:
			close(newsock);
			break;
		}
	}
	/* NOTREACHED */
}

void
doserver(int sock)
{
	char    c;
	int	n = 0;

	while (read(sock, &c, 1) == 1) {
		write(sock, &c, 1);
		n++;
	}

	/*
	 * A connection with no data means shut down.
	 */
	if (n == 0) {
		tcp_done(TCP_XACT);
		kill(getppid(), SIGTERM);
		exit(0);
	}
}
