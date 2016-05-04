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
 * bw_tcp.c - simple TCP bandwidth test
 *
 * Three programs in one -
 *	server usage:	bw_tcp -s
 *	client usage:	bw_tcp hostname
 *	shutdown:	bw_tcp -hostname
 *
 * IMPORTANT NOTE: If using remote (non-localhost) measurement along with
 *                 cycle counters, the two machines MUST HAVE THE SAME CLOCK
 *		   RATE for the measurement to be valid. If this is impossible,
 *		   then run the test manually and feed the server's clock
 *     		   multiplier to both the server and client. Note that the
 * 		   driver script WILL NOT do this automatically.
 *
 * Based on:
 *	$lmbenchId: bw_tcp.c,v 1.3 1995/06/21 21:02:49 lm Exp $
 *
 * $Id: bw_tcp.c,v 1.7 1997/06/27 00:33:58 abrown Exp $
 */
char	*id = "$Id: bw_tcp.c,v 1.7 1997/06/27 00:33:58 abrown Exp $\n";

#include <sys/wait.h>

#include "common.c"
#include "lib_tcp.c"


/*
 * General philosophy for getting reproducible numbers: we transfer
 * data in multiples of the user-supplied buffer size until the data
 * transfer takes one second or more. This can all be done nicely on
 * the client end, except that we have to tell the server the buffer
 * size when it starts up. So you need to kill and restart the server
 * each time you feed in a different buffer-size parameter.
 */

#define XFERUNIT	(1024*1024) 	/* Amount to transfer per iteration */

/* The worker function */
int 	do_client(int num_iter, clk_t *time);
void    server_main(void);
void    absorb(int control, int data);

/*
 * Global variables: these are the parameters required by the worker routine.
 * We make them global to avoid portability problems with variable argument
 * lists and the gen_iterations function
 */
unsigned int 	bufsize;	/* size of transfer requests to make */
char 		*rhostname;	/* hostname of remote host */

int
main(ac, av)
	int 	ac;
	char  **av;
{
	unsigned int	niter;
	clk_t		totaltime;
	unsigned int	xferred;

	/* print out RCS ID to stderr*/
	fprintf(stderr, "%s", id);

	/* Check command-line arguments */
	if (parse_counter_args(&ac, &av) || ac != 4) {
		fprintf(stderr, "Usage: %s%s iterations requestsize -s OR"
		   "\n       %s%s iterations requestsize [-]serverhost\n",
		    av[0], counter_argstring, av[0], counter_argstring);
		exit(1);
	}

	/* parse command line parameters */
	niter = atoi(av[1]);
	bufsize = parse_bytes(av[2]);

	if (!strcmp(av[3], "-s")) { /* starting server */
		if (fork() == 0) {
			server_main();
		}
		exit(0);
	}

	/* Starting client */
	if (av[3][0] == '-') {
		bufsize = 0;	/* signal client to kill server */
		rhostname = &av[3][1];
		do_client(1,&totaltime); /* run client to kill server */
		exit(0);	/* quit */
	} else {
		rhostname = av[3];
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

		/* double time to 2 sec to smooth over network burstiness */
		niter *= 2;

		/* Make sure we send at least 10MB */
		if (niter * XFERUNIT < 10*1024*1024)
			niter = 1 + (10*1024*1024)/XFERUNIT;

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
	do_client(niter, &totaltime);	/* get TCP bandwidth */

	output_bandwidth(niter * XFERUNIT, totaltime);

	return (0);
}

/*
 * This function does all the work. It transfers XFERUNIT to the
 * server num_iter times, timing the entire operation.
 *
 * Returns 0 if the benchmark was successful.  */
int
do_client(num_iter, t)
	int num_iter;
	clk_t *t;
{
	/*
	 * 	Global parameters
	 *
	 * unsigned int bufsize;
	 * char		*rhostname;
	 */
	char    *buf, *obuf;
	int     data, control;
	int     c;
	int     bytes;
#ifdef EVENT_COUNTERS
	eventcounter_t c0, c1;
#endif

	if (!bufsize) {
		/* We need to kill server */
		bytes = 0;
		bufsize = 64*1024;
	} else {
		bytes = num_iter * XFERUNIT;
	}

	buf = valloc(bufsize);
	if (!buf) {
		perror("valloc");
		exit(1);
	}

	/* Connect to server */
	control = tcp_connect(rhostname, TCP_CONTROL, SOCKOPT_NONE);
	data = tcp_connect(rhostname, TCP_DATA, SOCKOPT_WRITE);
	(void)sprintf(buf, "%d", bytes);
	if (write(control, buf, strlen(buf)) != strlen(buf)) {
		perror("control write");
		exit(1);
	}

	/*
	 * Disabler message to other side.
	 */
	if (bytes == 0) {
		return (0);
	}

	/* Write the data */
	while (bytes > 0 && (c = write(data, buf, bufsize)) > 0) {
		bytes -= c;
#if 0
		/*
		 * On IRIX/Hippi, this slows things down from 89 to 42MB/sec.
		 */
		bzero(buf, c);
#endif
	}
	(void)close(data);

	/* Get performance results back from server */
	if (read(control, buf, bufsize) <= 0) {
		perror("control timing");
		exit(1);
	}
	*t = CLKTSTR(buf);
#ifdef EVENT_COUNTERS
	obuf = buf;
	while (*(buf++) != '\n')
		;
	if (*buf == '\000')
		buf++;
	if (buf[0] == '@' && buf[1] == '1' && buf[2] == '@') {
		buf += 3;
		c0 = EVENTCOUNTERTSTR(buf);
		/* XXX THIS IS SUCH A HACK! */
		set_eventcounter_val(0, c0);
		eventcounter_active[0] = 1;
		while (*(buf++) != '\n')
			;
		if (*buf == '\000')
			buf++;
	}
	if (buf[0] == '@' && buf[1] == '2' && buf[2] == '@') {
		buf += 3;
		c1 = EVENTCOUNTERTSTR(buf);
		set_eventcounter_val(1, c1);
		eventcounter_active[1] = 1;
	}

	buf = obuf;
#endif
	free(buf);

	return (0);
}

void
child(dummy)
	int dummy;
{
	wait(0);
	signal(SIGCHLD, child);
}

void
server_main(void)
{
	int	data, control, newdata, newcontrol;

	GO_AWAY;

	signal(SIGCHLD, child);
	data = tcp_server(TCP_DATA, SOCKOPT_READ);
	control = tcp_server(TCP_CONTROL, SOCKOPT_NONE);

	for ( ;; ) {
		newcontrol = tcp_accept(control, SOCKOPT_NONE);
		newdata = tcp_accept(data, SOCKOPT_READ);
		switch (fork()) {
		    case -1:
			perror("fork");
			break;
		    case 0:
			absorb(newcontrol, newdata);
			exit(0);
		    default:
			close(newcontrol);
			close(newdata);
			break;
		}
	}
}

/*
 * Read the number of bytes to be transfered on the control socket.
 * Read that many bytes on the data socket.
 * Write the performance results on the control socket.
 */
void
absorb(int control, int data)
{
	int	nread, save, nbytes;
	char	*buf = valloc(bufsize);
	clk_t	timing;

	if (!buf) {
		perror("valloc");
		exit(1);
	}
	bzero(buf, bufsize);
	if (read(control, buf, bufsize) <= 0) {
		perror("control nbytes");
		exit(7);
	}
	nbytes = save = atoi(buf);

	/*
	 * A hack to allow turning off the absorb daemon.
	 */
     	if (nbytes == 0) {
		tcp_done(TCP_DATA);
		tcp_done(TCP_CONTROL);
		kill(getppid(), SIGTERM);
		exit(0);
	}
	start();
	while (nbytes > 0 && (nread = read(data, buf, bufsize)) > 0)
		nbytes -= nread;
	timing = stop(NULL);

	(void)close(2);		/* stderr - timing stats go to stderr */
	(void)dup(control);	/* stderr == control now */

	fprintf(stderr, CLKTFMT"\n\000", timing);
#ifdef EVENT_COUNTERS
	if (eventcounter_active[0])
		fprintf(stderr,"@1@"EVENTCOUNTERTFMT"\n\000",
			get_eventcounter(0));
	if (eventcounter_active[1])
		fprintf(stderr,"@2@"EVENTCOUNTERTFMT"\n\000",
			get_eventcounter(1));
#endif
}
