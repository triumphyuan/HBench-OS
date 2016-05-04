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
 * lat_udp.c - UDP transaction (read/write) latency test
 *
 * Three programs in one -
 *	server usage:	lat_udp -s
 *	client usage:	lat_udp hostname
 *	shutdown:	lat_udp -hostname
 *
 * Based on:
 * 	$lmbenchId: lat_udp.c,v 1.2 1995/03/11 02:15:39 lm Exp $
 *
 * $Id: lat_udp.c,v 1.4 1997/06/27 00:33:58 abrown Exp $
 */
char	*id = "$Id: lat_udp.c,v 1.4 1997/06/27 00:33:58 abrown Exp $\n";

#include "common.c"
#include "lib_udp.c"
/* Worker functions */
int do_client();
void server_main(void);

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
 * This function does all the work. It initiates a UDP connection
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
	int 	seq;
	register int n = 0;
	int     ret;

	/*
	 * Connect to the server and reset sequence number to 0
	 */
	sock = udp_connect(rhostname, UDP_XACT, SOCKOPT_NONE);
	seq = -100;
	while (seq-- > -105) {
		(void) send(sock, &seq, sizeof(seq), 0);
	}

	/*
	 * Stop server code, if requested.
	 */
	if (killserver) {
		seq = 0;
		while (seq-- > -5) {
			(void) send(sock, &seq, sizeof(seq), 0);
		}
		*t = (clk_t)0;
		return (0);
	}

	start();
	for (seq = 1, n = 0; n < num_iter; seq++) {
		if (send(sock, &seq, sizeof(seq), 0) != sizeof(seq)) {
			perror("lat_udp client: send failed");
			exit(5);
		}
		if (recv(sock, &ret, sizeof(ret), 0) != sizeof(ret)) {
			perror("lat_udp client: recv failed");
			exit(5);
		}
		if (seq == ret) {
			n++;
		}
	}
	*t = stop(NULL);

	return (0);
}

void
server_main(void)
{
	int     sock, sent, namelen, seq = 0;
	struct sockaddr it;

	GO_AWAY;

	sock = udp_server(UDP_XACT, SOCKOPT_NONE);

	while (1) {
		namelen = sizeof(it);
		if (recvfrom(sock, &sent, sizeof(sent), 0, &it, &namelen) < 0) {
			fprintf(stderr, "lat_udp server: recvfrom: got wrong size\n");
			exit(9);
		}
		if (sent < -100) {
			seq = 0;
			continue;
		}
		if (sent < 0) {
			udp_done(UDP_XACT);
			exit(0);
		}
		if (sent != ++seq) {
printf("lat_udp server: wanted %d, got %d, resyncing\n", seq, sent);	/**/
			seq = sent;
		}
		if (sendto(sock, &seq, sizeof(seq), 0, &it, sizeof(it)) < 0) {
			perror("lat_udp sendto");
			exit(9);
		}
	}
}
