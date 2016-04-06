/*
 * lat_pagefault.c - time a page fault in
 *
 * Usage: lat_pagefault file [file file...]
 *
 * Copyright (c) 1994 Larry McVoy.  Distributed under the FSF GPL with
 * additional restriction that results may published only if
 * (1) the benchmark is unmodified, and
 * (2) the version in the sccsid below is included in the report.
 * Support for this development by Sun Microsystems is gratefully acknowledged.
 */
char	*id = "$Id: lat_pagefault.c,v 1.5 1995/11/08 01:39:50 lm Exp $\n";

#include "timing.c"

#include <sys/stat.h>
#include <sys/mman.h>

#define	CHK(x)	if ((x) == -1) { perror("x"); exit(1); }

main(ac, av)
	char **av;
{
#ifdef	MS_INVALIDATE
	int fd;
	int i;
	char *where;
	struct stat sbuf;

	write(2, id, strlen(id));
	if (ac < 2) {
		fprintf(stderr, "usage: %s file [file...]\n", av[0]);
		exit(1);
	}
	for (i = 1; i < ac; ++i) {
		CHK(fd = open(av[i], 0));
		CHK(fstat(fd, &sbuf));
		sbuf.st_size &= ~(16*1024 - 1);		/* align it */
		if (sbuf.st_size < 1024*1024) {
			fprintf(stderr, "%s: %s too small\n", av[0], av[i]);
			continue;
		}
		where = mmap(0, sbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
		if (msync(where, sbuf.st_size, MS_INVALIDATE) != 0) {
			perror("msync");
			exit(1);
		}
		timeit(av[i], where, sbuf.st_size);
		munmap(where, sbuf.st_size);
	}
#endif
	exit(0);
}

/*
 * Get page fault times by going backwards in a stride of 256K
 */
timeit(file, where, size)
	char	*file, *where;
{
	char	*end = where + size - 16*1024;
	int	sum = 0;
	int	lowest = 0x7fffffff;

	while (end > where) {
		start();
		sum += *end;
		end -= 256*1024;
		sum = stop(sum);
		if (sum < lowest)
			lowest = sum;
	}
	fprintf(stderr, "Pagefaults on %s: %d usecs\n", file, lowest);
}
