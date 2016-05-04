/*
 * Copyright (c) 1997 The President and Fellows of Harvard College.
 * All rights reserved.
 * Copyright (c) 1997 Aaron B. Brown.
 * Copyright (c) 1995 Larry McVoy.
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
 * memsize.c - figure out how much memory we have to use.
 *
 * Usage: memsize [max_wanted_in_MB]
 *
 * Based on:
 *	$lmbenchId: memsize.c,v 1.6 1995/10/26 01:03:42 lm Exp $
 *
 * $Id: memsize.c,v 1.8 1997/06/27 00:33:58 abrown Exp $
 */
char	*id = "$Id: memsize.c,v 1.8 1997/06/27 00:33:58 abrown Exp $\n";

#include "common.c"

#define	CHK(x)	if ((x) == -1) { perror("x"); exit(1); }

#ifndef	TOO_LONG
#define	TOO_LONG	10	/* usecs */
#endif

main(ac, av)
	char **av;
{
	char	*where;
	char	*tmp;
	size_t	size;
	size_t	max;

	if (parse_counter_args(&ac, &av) || ac > 2) {
		fprintf(stderr, "usage: %s%s [maxmemsizeinMB]\n",
			av[0], counter_argstring);
		exit(1);
	}
	if (ac == 2) {
		max = size = atoi(av[1]) * 1024 * 1024;
	} else {
		max = size = 1024 * 1024 * 1024;
	}
	/*
	 * Binary search down and then linear search up
	 */
	for (where = 0; !where; where = malloc(size)) {
		size >>= 1;
	}
	free(where);
	tmp = 0;
	do {
		if (tmp) {
			free(tmp);
		}
		size += 1024*1024;
		tmp = malloc(size);
		if (tmp) {
			where = tmp;
		} else {
			size -= 1024*1024;
			tmp = malloc(size);
			break;
		}
	} while (tmp && (size < max));

	init_timing();

	timeit(where, size);
	exit(0);
}

timeit(where, size)
	char	*where;
{
	clk_t	lat = (clk_t) 0;
	int	n;
	char	*end = where + size;
	int	range;

	if (size < 1024*1024 - 16*1024) {
		fprintf(stderr, "Bad size\n");
		return;
	}

	/* Go up in 1MB chunks until we find one too big */
	for (range = 2*1024*1024; range <= size; range += 1<<20) {
		touch(where, end, range);
		start();
		touch(where, end, range);
		lat = stop(NULL);
		n = range / 4096;
		if ((lat / n) > (clk_t)((float)TOO_LONG / clock_multiplier)) {
			fprintf(stderr, "\n");
			printf("%d\n", (range>>20) - 1);
			exit(0);
		}
		fprintf(stderr, "%dMB OK\r", range/(1024*1024));
	}
	fprintf(stderr, "\n");
	printf("%d\n", (size>>20));
}

touch(char *p, char *end, int range)
{
	char	*tmp = p;

	while (range > 0 && (tmp < end)) {
		*tmp = 0;
		tmp += 4096;
		range -= 4096;
	}
}
