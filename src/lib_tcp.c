/*
 * tcp_lib.c - routines for managing TCP connections.
 *
 * Copyright (c) 1994 Larry McVoy.
 */
#include	"bench.h"
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<netdb.h>
#include	<arpa/inet.h>

/* #define	LIBTCP_VERBOSE	/**/

void sock_optimize(int sock, int rdwr);

u_short	pmap_getport();

/*
 * Get a TCP socket, bind it, figure out the port,
 * and advertise the port as program "prog".
 *
 * XXX - it would be nice if you could advertise ascii strings.
 */
int
tcp_server(prog, rdwr)
	u_long	prog;
	int	rdwr;
{
	int	sock, namelen;
	struct	sockaddr_in s;

	if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror("socket");
		exit(1);
	}
	sock_optimize(sock, rdwr);
	bzero((char*)&s, sizeof(s));
	s.sin_family = AF_INET;
#ifdef	NO_PORTMAPPER
	s.sin_port = htons(prog);
#endif
	if (bind(sock, (struct sockaddr*)&s, sizeof(s)) < 0) {
		perror("bind");
		exit(2);
	}
	if (listen(sock, 1) < 0) {
		perror("listen");
		exit(4);
	}
#ifndef	NO_PORTMAPPER
	namelen = sizeof(s);
	if (getsockname(sock, (struct sockaddr *)&s, &namelen) < 0) {
		perror("getsockname");
		exit(3);
	}
#ifdef	LIBTCP_VERBOSE
	fprintf(stderr, "Server port %d\n", ntohs(s.sin_port));
#endif
	(void)pmap_unset(prog, (u_long)1);
	if (!pmap_set(prog, (u_long)1, (u_long)IPPROTO_TCP,
	    (u_long)ntohs(s.sin_port))) {
		perror("pmap_set");
		exit(5);
	}
#endif
	return (sock);
}

/*
 * Unadvertise the socket
 */
tcp_done(prog)
	u_long	prog;
{
#ifndef	NO_PORTMAPPER
	pmap_unset((u_long)prog, (u_long)1);
#endif
	return (0);
}

/*
 * Accept a connection and return it
 */
tcp_accept(sock, rdwr)
	int	sock, rdwr;
{
	struct	sockaddr_in s;
	int	newsock, namelen;

	namelen = sizeof(s);
	bzero((char*)&s, namelen);

retry:
	if ((newsock = accept(sock, (struct sockaddr*)&s, &namelen)) < 0) {
		if (errno == EINTR)
			goto retry;
		perror("accept");
		exit(6);
	}
#ifdef	LIBTCP_VERBOSE
	namelen = sizeof(s);
	if (getsockname(newsock, (struct sockaddr *)&s, &namelen) < 0) {
		perror("getsockname");
		exit(3);
	}
	fprintf(stderr, "Server newsock port %d\n", ntohs(s.sin_port));
#endif
	sock_optimize(newsock, rdwr);
	return (newsock);
}

/*
 * Connect to the TCP socket advertised as "prog" on "host" and
 * return the connected socket.
 *
 * Hacked Thu Oct 27 1994 to cache pmap_getport calls.  This saves
 * about 4000 usecs in loopback lat_connect calls.  I suppose we
 * should time gethostbyname() & pmap_getprot(), huh?
 */
tcp_connect(host, prog, rdwr)
	char	*host;
	u_long	prog;
	int	rdwr;
{
	static	struct hostent *h;
	static	struct sockaddr_in s;
	static	u_short	save_port;
	static	u_long save_prog;
	static	char *save_host;
	int	sock, namelen;

	if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror("socket");
		exit(1);
	}
#ifdef	LIBTCP_VERBOSE
	bzero((char*)&s, sizeof(s));
	s.sin_family = AF_INET;
	if (bind(sock, (struct sockaddr*)&s, sizeof(s)) < 0) {
		perror("bind");
		exit(2);
	}
	namelen = sizeof(s);
	if (getsockname(sock, (struct sockaddr *)&s, &namelen) < 0) {
		perror("getsockname");
		exit(3);
	}
	fprintf(stderr, "Client port %d\n", ntohs(s.sin_port));
#endif
	sock_optimize(sock, rdwr);
	if (!h || host != save_host || prog != save_prog) {
		save_host = host;	/* XXX - counting on them not
					 * changing it - benchmark only.
					 */
		save_prog = prog;
		if (!(h = gethostbyname(host))) {
			perror(host);
			exit(2);
		}
		bzero((char *) &s, sizeof(s));
		s.sin_family = AF_INET;
		bcopy(h->h_addr, (char *) &s.sin_addr, h->h_length);
#ifndef	NO_PORTMAPPER
		save_port = pmap_getport(&s, prog, (u_long)1, IPPROTO_TCP);
		if (!save_port) {
			perror("lib TCP: No port found");
			exit(3);
		}
#endif
#ifdef	LIBTCP_VERBOSE
		fprintf(stderr, "Server port %d\n", save_port);
#endif
	}
#ifdef	NO_PORTMAPPER
	s.sin_port = htons(prog);
#else
	s.sin_port = htons(save_port);
#endif
	if (connect(sock, (struct sockaddr*)&s, sizeof(s)) < 0) {
		perror("connect");
		exit(4);
	}
	return (sock);
}

/*
 * This is identical in lib_tcp.c and lib_udp.c
 */
void
sock_optimize(int sock, int rdwr)
{
	if (rdwr == SOCKOPT_READ || rdwr == SOCKOPT_RDWR) {
		int	sockbuf = SOCKBUF;

		while (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &sockbuf,
		    sizeof(int))) {
			sockbuf -= SOCKSTEP;
		}
#ifdef	LIBTCP_VERBOSE
		fprintf(stderr, "sockopt %d: RCV: %dK\n", sock, sockbuf>>10);
#endif
	}
	if (rdwr == SOCKOPT_WRITE || rdwr == SOCKOPT_RDWR) {
		int	sockbuf = SOCKBUF;

		while (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sockbuf,
		    sizeof(int))) {
			sockbuf -= SOCKSTEP;
		}
#ifdef	LIBTCP_VERBOSE
		fprintf(stderr, "sockopt %d: SND: %dK\n", sock, sockbuf>>10);
#endif
	}
}
