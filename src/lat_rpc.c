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
 * lat_rpc.c - RPC transaction latency test
 *
 * Three programs in one -
 *	server usage:	lat_rpc dummy -s
 *	client usage:	lat_rpc [udp|tcp] hostname
 *	shutdown:	lat_rpc dummy -hostname
 *
 * Based on:
 *	$lmbenchId: lat_rpc.c,v 1.2 1995/03/11 02:25:17 lm Exp $
 *
 * $Id: lat_rpc.c,v 1.4 1997/06/27 00:33:58 abrown Exp $
 */
char	*id = "$Id: lat_rpc.c,v 1.4 1997/06/27 00:33:58 abrown Exp $\n";

#include "common.c"

#include <ctype.h>
#include <stdio.h>
#include <rpc/rpc.h>

/* Worker functions */
int do_client();

/*
 * Global variables: these are the parameters required by the worker routine.
 * We make them global to avoid portability problems with variable argument
 * lists and the gen_iterations function 
 */
char 		*rhostname;	/* hostname of remote host */
char		*protocol = 0; 	/* NULL indicates kill-server */

int
main(ac, av)
	int ac;
	char  **av;
{
	unsigned int	niter;
	clk_t		totaltime;

	/* print out RCS ID to stderr*/
	fprintf(stderr, "%s", id);

	if (parse_counter_args(&ac, &av) || ac != 4) {
		fprintf(stderr, "Usage: %s%s iterations dummy -s OR"
		   "\n       %s%s iterations [tcp|udp] [-]serverhost\n",
		    av[0], counter_argstring, av[0], counter_argstring);
		exit(1);
	}

	/* parse command line parameters */
	niter = atoi(av[1]);
	protocol = av[2];

	if (!strcmp(av[3], "-s")) { /* starting server */
		if (fork() == 0) {
			server_main();
		}
		exit(0);
	}

	/* Starting client */	
	if (av[3][0] == '-') {
		protocol = NULL;	/* signal client to kill server */
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

/* Default timeout can be changed using clnt_control() */
static struct timeval TIMEOUT = { 25, 0 };

/*
 * This function does all the work. It initiates a connection
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
	 * char *protocol;
	 */
	register int	i;
	CLIENT *cl;
	char	c;
	char	*resp;
	struct	timeval tv;
	char	*server;
	char 	*proto;

	proto = (protocol == NULL ? "udp" : protocol);

	if (!(cl = clnt_create(rhostname, XACT_PROG, XACT_VERS, proto))) {
		clnt_pcreateerror(rhostname);
		exit(1);
	}
	if (protocol == NULL) {
		clnt_call(cl, RPC_EXIT, xdr_void, 0, xdr_void, 0, TIMEOUT);
		*t = (clk_t)0;
		return (0);
    	}
	if (strcmp(proto, "udp") == 0) {
		tv.tv_sec = 0;
		tv.tv_usec = 2000;
		if (!clnt_control(cl, CLSET_RETRY_TIMEOUT, (char *)&tv)) {
			clnt_perror(cl, "setting timeout");
			exit(1);
		}
	}
	c = 1;

	start();
	for (i = num_iter; i > 0; i--) {
		resp = client_rpc_xact_1(&c, cl);
		if (!resp) {
			clnt_perror(cl, server);
			exit(1);
		}
		if (*resp != 123) {
			fprintf(stderr, "client got bad data\n");
			exit(1);
		}
	}
	*t = stop();

	return (0);
}

char *
client_rpc_xact_1(argp, clnt)
	char *argp;
	CLIENT *clnt;
{
	static char res;

	bzero((char *)&res, sizeof(res));
	if (clnt_call(clnt, RPC_XACT, xdr_char, argp, xdr_char, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}

/*
 * The remote procedure[s] that will be called
 */
char	*
rpc_xact_1(msg, transp)
     	char	*msg;
	register SVCXPRT *transp;
{
	static char r = 123;

	return &r;
}

static void xact_prog_1();

server_main()
{
	register SVCXPRT *transp;

	GO_AWAY;

	(void) pmap_unset(XACT_PROG, XACT_VERS);

	transp = svcudp_create(RPC_ANYSOCK);
	if (transp == NULL) {
		fprintf(stderr, "cannot create udp service.");
		exit(1);
	}
	if (!svc_register(transp, XACT_PROG, XACT_VERS, xact_prog_1, IPPROTO_UDP)) {
		fprintf(stderr, "unable to register (XACT_PROG, XACT_VERS, udp).");
		exit(1);
	}

	transp = svctcp_create(RPC_ANYSOCK, 0, 0);
	if (transp == NULL) {
		fprintf(stderr, "cannot create tcp service.");
		exit(1);
	}
	if (!svc_register(transp, XACT_PROG, XACT_VERS, xact_prog_1, IPPROTO_TCP)) {
		fprintf(stderr, "unable to register (XACT_PROG, XACT_VERS, tcp).");
		exit(1);
	}

	svc_run();
	fprintf(stderr, "svc_run returned");
	exit(1);
	/* NOTREACHED */
}

static void
xact_prog_1(rqstp, transp)
	struct svc_req *rqstp;
	register SVCXPRT *transp;
{
	union {
		char rpc_xact_1_arg;
	} argument;
	char *result;
	bool_t (*xdr_argument)(), (*xdr_result)();
	char *(*local)();

	switch (rqstp->rq_proc) {
	case NULLPROC:
		(void) svc_sendreply(transp, xdr_void, (char *)NULL);
		return;

	case RPC_XACT:
		xdr_argument = xdr_char;
		xdr_result = xdr_char;
		local = (char *(*)()) rpc_xact_1;
		break;

	case RPC_EXIT:
		(void) svc_sendreply(transp, xdr_void, (char *)NULL);
		(void) pmap_unset(XACT_PROG, XACT_VERS);
		exit(0);

	default:
		svcerr_noproc(transp);
		return;
	}
	bzero((char *)&argument, sizeof(argument));
	if (!svc_getargs(transp, xdr_argument, &argument)) {
		svcerr_decode(transp);
		return;
	}
	result = (*local)(&argument, rqstp);
	if (result != NULL && !svc_sendreply(transp, xdr_result, result)) {
		svcerr_systemerr(transp);
	}
	if (!svc_freeargs(transp, xdr_argument, &argument)) {
		fprintf(stderr, "unable to free arguments");
		exit(1);
	}
	return;
}
