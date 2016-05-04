/*
 * udp_lib.c - routines for managing UDP connections
 *
 * %W% %G%
 *
 * Copyright (c) 1994 Larry McVoy.
 */
#include	"bench.h"
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<netdb.h>
#include	<arpa/inet.h>

void sock_optimize(int sock, int rdwr);

/*
 * Get a UDP socket, bind it, figure out the port,
 * and advertise the port as program "prog".
 *
 * XXX - it would be nice if you could advertise ascii strings.
 */
int
udp_server(u_long prog, int rdwr)
{
	int	sock, namelen;
	struct	sockaddr_in s;

	if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
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
#ifndef	NO_PORTMAPPER
	namelen = sizeof(s);
	if (getsockname(sock, (struct sockaddr *)&s, &namelen) < 0) {
		perror("getsockname");
		exit(3);
	}
	(void)pmap_unset(prog, (u_long)1);
	if (!pmap_set(prog, (u_long)1, (u_long)IPPROTO_UDP,
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
void
udp_done(prog)
{
#ifndef	NO_PORTMAPPER
	(void)pmap_unset((u_long)prog, (u_long)1);
#endif
}

/*
 * "Connect" to the UCP socket advertised as "prog" on "host" and
 * return the connected socket.
 */
int
udp_connect(char *host, u_long prog, int rdwr)
{
	struct hostent *h;
	struct sockaddr_in sin;
	int	sock;
	u_short	port;
	u_short	pmap_getport();

	if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		perror("socket");
		exit(1);
	}
	sock_optimize(sock, rdwr);
	if (!(h = gethostbyname(host))) {
		perror(host);
		exit(2);
	}
	bzero((char *) &sin, sizeof(sin));
	sin.sin_family = AF_INET;
	bcopy(h->h_addr, (char *) &sin.sin_addr, h->h_length);
#ifdef	NO_PORTMAPPER
	sin.sin_port = prog;
#else
	port = pmap_getport(&sin, prog, (u_long)1, IPPROTO_UDP);
	if (!port) {
		perror("lib UDP: No port found");
		exit(3);
	}
	sin.sin_port = htons(port);
#endif
	if (connect(sock, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
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
	}
	if (rdwr == SOCKOPT_WRITE || rdwr == SOCKOPT_RDWR) {
		int	sockbuf = SOCKBUF;

		while (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sockbuf,
		    sizeof(int))) {
			sockbuf -= SOCKSTEP;
		}
	}
}
