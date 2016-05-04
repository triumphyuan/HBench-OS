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
 * Benchmark file system creates and deletes.
 *
 * Usage:
 *	lat_fs [create|delforw|delrev|delrand] scratchdir
 *
 * Unlike other tests, we scale this one to run for 2-4 seconds, rather
 * than 1-2, to average over disk variation.
 *
 * Based on:
 *	$lmbenchId: lat_fs.c,v 1.6 1995/03/11 02:22:25 lm Exp $
 *
 * $Id: lat_fs.c,v 1.8 1997/06/27 00:33:58 abrown Exp $
 */
char	*id = "$Id: lat_fs.c,v 1.8 1997/06/27 00:33:58 abrown Exp $\n";

#include "common.c"
#include <stdlib.h>
#include <sys/time.h>

/* Worker function */
int do_create();		/* wrapper that does timing */
int do_delete();		/* wrapper that does timing */
int real_create();		/* real file creation test */
int real_delete();		/* real file deletion test */
char **generate_names();	/* generate a bunch of temporary filenames */
void release_names();		/* free the temp. filenames */

/*
 * Global variables: these are the parameters required by the worker routine.
 * We make them global to avoid portability problems with variable argument
 * lists and the gen_iterations function
 */
char 	*tmpdir;		/* name of scratch directory */
int	filesize;		/* size of files to create or delete */
char 	**filenames;		/* generated temporary filenames */
char	*databuf;		/* data buffer for writing files */
int	deldir = 0;		/* direction in which to delete files */
#define DEL_FORWARD		1
#define DEL_REVERSE		2
#define DEL_PSEUDORANDOM	3

int
main(ac, av)
	int ac;
	char **av;
{
	clk_t		totaltime;
	unsigned int	niter;
	int 		(*workerfunc)(int, clk_t *);
	struct timeval 	tv;

	/* print out RCS ID to stderr*/
	fprintf(stderr, "%s", id);

	/* Check command-line arguments */
	if (parse_counter_args(&ac, &av) || ac != 5) {
		fprintf(stderr, "usage: %s%s iterations [create|delforw|delrev|delrand] filesize scratchdir\n",
			av[0], counter_argstring);
		exit(1);
	}

	/* parse command line parameters */
	niter = atoi(av[1]);
	if (!strcmp(av[2], "create"))
		workerfunc = &do_create;
	else if (!strcmp(av[2], "delforw")) {
		workerfunc = &do_delete;
		deldir = DEL_FORWARD;
	} else if (!strcmp(av[2], "delrev")) {
		workerfunc = &do_delete;
		deldir = DEL_REVERSE;
	} else if (!strcmp(av[2], "delrand")) {
		workerfunc = &do_delete;
		deldir = DEL_PSEUDORANDOM;
	} else {
		fprintf(stderr, "Error: unknown operation type %s\n", av[2]);
		exit(1);
	}
	filesize = parse_bytes(av[3]);
	tmpdir = av[4];

	/* Switch to temporary directory */
	if (tmpdir)
	    chdir(tmpdir);

	/* initialize timing module (calculates timing overhead, etc) */
	init_timing();

	/* Do some initial setup */
	databuf = (char *)malloc(filesize*2);
	if (!databuf) {
		perror("malloc");
		exit(1);
	}

	/* Set up random number generator */
	gettimeofday(&tv, NULL);
	srand(tv.tv_usec);

#ifndef COLD_CACHE
	/*
	 * Generate the appropriate number of iterations so the test takes
	 * at least one second. For efficiency, we are passed in the expected
	 * number of iterations, and we return it via the process error code.
	 * No attempt is made to verify the passed-in value; if it is 0, we
	 * we recalculate it.
	 */
	if (niter == 0) {
		/* the 2.0 below is to make the test run for 2-4 seconds */
		niter = gen_iterations(workerfunc, clock_multiplier/2.0);
		printf("%d\n",niter);
		return (0);
	}

	/*
	 * Take the real data and average to get a result
	 */
	(*workerfunc)(1, &totaltime);	/* prime caches, etc. */
#else
	niter = 1;
#endif
	(*workerfunc)(niter, &totaltime);	/* get cached reread */

	output_latency(totaltime, niter);

	/* Clean up state */
	free(databuf);

	return (0);
}

/*
 * Do lots of file creations, timing them.
 */
int
do_create(num_iter, t)
	int num_iter;
	clk_t *t;
{
	/*
	 * Global params:
	 *
	 *     char *tmpdir;
	 */
	int ret;

	/*
	 * Generate the file names to use
	 */
	filenames = generate_names(num_iter);

	/*
	 * Create the files, timing the process
	 */
	start();
	ret = real_create(num_iter);
	*t = stop(NULL);

	/*
	 * Now delete all the files we just created.
	 */
	real_delete(num_iter);

	/*
	 * Clean up filename state
	 */
	release_names(filenames);

	return (ret);
}

/*
 * Do lots of file deletions, timing them.
 */
int
do_delete(num_iter, t)
	int num_iter;
	clk_t *t;
{
	/*
	 * Global params:
	 *
	 *     char *tmpdir;
	 */
	int ret;
	register int i;
	register char *tmp;

	/*
	 * Generate the file names to use
	 */
	filenames = generate_names(num_iter);

	/*
	 * First, create all the files.
	 */
	real_create(num_iter);

	/*
	 * Now shuffle the deletion order as requested.
	 */
	if (deldir == DEL_REVERSE) {
		for (i = 0; i < (num_iter +1)/2; i++) {
			/* swap(filenames[i],filenames[numfiles - i]); */
			tmp = filenames[num_iter - i];
			filenames[num_iter - i] = filenames[i];
			filenames[i] = tmp;
		}
	} else if (deldir == DEL_PSEUDORANDOM) {
		/* This isn't terribly random, but it will suffice. */
		int pos;
		for (i = 0; i < num_iter; i++) {
			pos = rand() % num_iter;
			/* swap(filenames[i], filenames[pos]); */
			tmp = filenames[pos];
			filenames[pos] = filenames[i];
			filenames[i] = tmp;
		}
	}

	/*
	 * Now delete all the files, timing them.
	 */
	start();
	ret = real_delete(num_iter);
	*t = stop(NULL);

	/*
	 * Clean up filename state
	 */
	release_names(filenames);

	return (ret);
}

/*
 * real_create: the worker function that actually performs the file
 * creations.
 */
int
real_create(numfiles)
	int numfiles;
{
	register int i;

	for (i = numfiles - 1; i >= 0; i--) {
		int fd = creat(filenames[i], 0666);
		if (filesize > 0)
		    write(fd, databuf, filesize);
		close(fd);
	}

	return (0);
}

/*
 * real_delete: the worker function that actually performs the file
 * deletions. Assumes the files have already been created.
 */
int
real_delete(numfiles)
	int numfiles;
{
	register int i;

	for (i = numfiles - 1; i >= 0; i--) {
		unlink(filenames[i]);
	}

	return (0);
}

/*
 * generate_names: create numfiles temporary filenames for use in
 * the creation stage. The names are of the form "Hxxxxxxx" where xxxxxxx
 * is a hexadecimal number representing the file number. We thus allow
 * up to 2^28 iterations.
 */
static char *namebuf;

char **
generate_names(numfiles)
	int numfiles;
{
	register int i;
	char **ret;
	char *scan;

	if (numfiles > 1024*1024*256) {
		fprintf(stderr,"Too many iterations (%d; max %d)\n",
			numfiles, 1024*1024*256);
		exit(1);
	}

	namebuf = (char *)malloc(numfiles*(strlen("Hxxxxxxx")+1));
	ret = (char **)malloc(numfiles * sizeof(char *));

	if (!namebuf || !ret) {
		fprintf(stderr,"Not enough memory to generate %d filenames\n",
			numfiles);
		exit(1);
	}

	scan = namebuf;
	for (i = numfiles-1; i >= 0; i--) {
		sprintf(scan,"H%07x",i);
		ret[i] = scan;
		scan += strlen("Hxxxxxxx")+1;
	}
	return (ret);
}

/*
 * release_names: release the generated filenames
 */
void
release_names(names)
	char **names;
{
	free(names);
	free(namebuf);
}
