/* XXX ABB: add rusage stuff trimmed from timing.c */
char	*id = "$Id: lmdd.c,v 1.18 1997/06/23 22:09:34 abrown Exp $\n";
/*
 * defaults:
 *	bs=8k
 *	count=forever
 *	if=internal
 *	of=internal
 *	ipat=0
 *	opat=0
 *	mismatch=0
 *	rusage=0
 *	flush=0
 *	rand=0
 *	print=0
 *	label=""
 * shorthands:
 *	recognizes 'k' or 'm' at the end of a number for 1024 & 1024^2
 *	recognizes "internal" as an internal /dev/zero /dev/null file.
 *
 * Copyright (c) 1994,1995 Larry McVoy.  All rights reserved.
 */

#include	"timing.c"

#ifdef	FLUSH
#include	<unistd.h>
#include	<sys/mman.h>
#include	<sys/stat.h>
#endif

#include	<fcntl.h>

#ifdef	sgi
#define	AIO	10
#ifdef	AIO
#include	<aio.h>
#endif
#endif

#define	USE_VALLOC
#ifdef	USE_VALLOC
#define	VALLOC	valloc
#else
#define	VALLOC	malloc
#endif

int     out, Print, Fsync, Sync, Flush, Rand, Bsize, ru, c;/* done needs it */
char	*Label;
#ifdef	AIO
int	Aios;
struct aiocb aios[AIO];
struct aiocb *a;
int	next_aio = 0;
#endif

long	getarg();
char   *cmds[] = {
	"if",			/* input file */
	"of",			/* output file */
	"ipat",			/* check input for pattern */
	"opat",			/* generate pattern on output */
	"mismatch",		/* stop at first mismatch */
	"bs",			/* block size */
	"count",		/* number of blocks */
	"skip",			/* skip this number of blocks on input */
	"fsync",		/* fsync output before exit */
	"sync",			/* sync output before exit */
	"print",		/* report type */
	"label",		/* prefix print out with this */
	"move",			/* instead of count, limit transfer to this */
	"rand",			/* do randoms over the specified size */
				/* must be power of two, not checked */
#ifdef	RUSAGE
	"rusage",		/* dump rusage stats */
#endif
#ifdef	FLUSH
	"flush",		/* map in out and invalidate (flush) */
#endif
#ifdef	sgi
	"direct",		/* O_DIRECT */
#endif
#ifdef	AIO
	"aios",			/* multiple writers */
#endif
	0,
};

main(ac, av)
	int	ac;
	char  **av;
{
	uint  *buf;
	int     misses, mismatch, outpat, inpat, in, gotcnt, count;
	int     skip;
	long	size;
	void    done();
	void	chkarg();
	long     i;
	off_t	off = 0;

	if (sizeof(int) != 4) {
		fprintf(stderr, "sizeof(int) != 4\n");
		exit(1);
	}
	for (i = 1; i < ac; ++i) {
		chkarg(av[i]);
	}
	signal(SIGINT, done);
	misses = mismatch = getarg("mismatch=", ac, av);
	inpat = getarg("ipat=", ac, av);
	outpat = getarg("opat=", ac, av);
	Bsize = getarg("bs=", ac, av);
	if (Bsize < 0)
		Bsize = 8192;
	Fsync = getarg("fsync=", ac, av);
	Sync = getarg("sync=", ac, av);
	Rand = getarg("rand=", ac, av);
	Print = getarg("print=", ac, av);
	Label = (char *)getarg("label=", ac, av);
	if (Rand != -1) {
		size = (Rand - 1) & ~511;
	}
#ifdef	RUSAGE
	ru = getarg("rusage=", ac, av);
#endif
	count = getarg("count=", ac, av);
	i = getarg("move=", ac, av);
	if (i > 0)
		count = i / Bsize;

#ifdef	FLUSH
	Flush = getarg("flush=", ac, av);
#endif
	if (count < 0)
		gotcnt = 0;
	else
		gotcnt = 1;
	c = 0;
	skip = getarg("skip=", ac, av);

	if ((inpat != -1 || outpat != -1) && (Bsize & 3)) {
		fprintf(stderr, "Block size 0x%x must be word aligned\n", Bsize);
		exit(1);
	}
	if ((Bsize >> 2) == 0) {
		fprintf(stderr, "Block size must be at least 4.\n");
		exit(1);
	}
#ifdef	AIO
	Aios = getarg("aios=", ac, av);
	if (Aios > AIO) {
		printf("Only %d aios supported\n", AIO);
		exit(1);
	}
	if (Aios <= 0) {
		Aios = 1;
	}
	if (Aios > 1) {
		aio_init();
	}
	bzero(aios, sizeof(aios));
	for (i = 0; i < Aios; ++i) {
#endif
	if (!(buf = (uint *) VALLOC((unsigned) Bsize))) {
		perror("VALLOC");
		exit(1);
	}
	bzero((char *) buf, Bsize);
#ifdef	AIO
	aios[i].aio_buf = buf;
	}
#endif

	start();
	/*
	 * We want this here because otherwise the accounting gets screwed up
	 */
	in = getfile("if=", ac, av);
	out = getfile("of=", ac, av);
	if (skip > 0) {
		lseek(in, skip * Bsize, 0);
	}
	for (;;) {
		register x;

		if (gotcnt && count-- <= 0) {
			done();
		}

#ifdef	AIO
		if (Aios > 1) {
			if (++next_aio == Aios) {
				next_aio = 0;
			}
			a = &aios[next_aio];
			buf = a->aio_buf;
			if (a->aio_nbytes) {
				int	n;

				aio_suspend(&a, 1, 0);
				n = aio_return(a);
				if (n != a->aio_nbytes) {
					printf("aio%d wanted %d got %d\n",
					   next_aio, a->aio_nbytes, n);
					done();
#if 0
				} else {
					printf("aio %x @ %d worked\n", 
					    a->aio_buf, a->aio_offset);
#endif
				}
			}
			a->aio_nbytes = 0;
		}
#endif

		/*
		 * Set the seek pointer if doing randoms
		 */
		if (Rand != -1) {
			off = lrand48() & size;
			if (in >= 0) {
				lseek(in, off, 0);
			}
			if (out >= 0) {
				lseek(out, off, 0);
			}
		}

#ifdef	AIO
		if (Aios > 1) {
			a->aio_offset = off;
		}
#endif

		if (in >= 0) {
			x = read(in, buf, Bsize);
		} else {
			x = Bsize;
		}
		if (x <= 0) {
			done();
		}
		if (inpat != -1) {
			register foo, cnt;

			for (foo = 0, cnt = x/sizeof(int); cnt--; foo++) {
				if (buf[foo] != (uint) (off + foo*sizeof(int))) {
					fprintf(stderr,
					    "off=%uK want=%x (%uK) got=%x (%uK)\n",
					    off >> 10,
					    off + foo*sizeof(int),
					    (off + foo*sizeof(int)) >> 10,
					    buf[foo], buf[foo] >> 10);
					if (mismatch != -1 && --misses == 0) {
						done();
					}
				}
			}
		}
		if (outpat != -1) {
			register foo, cnt;

			for (foo = 0, cnt = x/sizeof(int); cnt--; foo++) {
				buf[foo] = (uint)(off + foo*sizeof(int));
			}
		}
		if (out >= 0) {
#ifdef	AIO
			if (Aios > 1) {
				a->aio_nbytes = x;
				a->aio_fildes = out;
				if (aio_write(a)) {
					perror("aio_write");
					done();
				}
			} else {
				if (write(out, buf, x) != x) {
					done();
				}
			}
#else
			if (write(out, buf, x) != x) {
				done();
			}
#endif
		}
		off += x;
		c += (x >> 2);
	}
}

void
chkarg(arg)
	char	*arg;
{
	int	i;
	char	*a, *b;

	for (i = 0; cmds[i]; ++i) {
		for (a = arg, b = cmds[i]; *a && *b && *a == *b; a++, b++)
			;
		if (*a == '=')
			return;
	}
	fprintf(stderr, "Bad arg: %s\n", arg);
	exit(1);
	/*NOTREACHED*/
}

void
done()
{
#ifdef	AIO
	if (Aios > 1) {
		int	i, n;

		for (i = 0; i < Aios; ++i) {
			a = &aios[i];
			if (a->aio_nbytes) {
				aio_suspend(&a, 1, 0);
				n = aio_return(a);
				if (n != a->aio_nbytes) {
					printf("aio%d wanted %d got %d\n",
					   next_aio, a->aio_nbytes, n);
#if 0
				} else {
					printf("aio %d worked\n", n);
#endif
				}
			}
			a->aio_nbytes = 0;
		}
	}
#endif
	if (Sync > 0)
		sync();
	if (Fsync > 0)
		fsync(out);
#ifdef	FLUSH
	if (Flush > 0)
		flush();
#endif
	stop();
#ifdef	RUSAGE
	if (ru != -1)
		rusage();
#endif
	if ((long)Label != -1) {
		fprintf(stderr, "%s", Label);
	}
	switch (Print) {
	    case 0:	/* no print out */
	    	break;

	    case 1:	/* latency type print out */
		latency((c << 2) / Bsize, Bsize);
		break;

	    case 2:	/* microsecond per op print out */
		micro("", (c << 2) / Bsize);
		break;

	    case 3:	/* kb / sec print out */
		kb(c << 2);
		break;

	    case 4:	/* mb / sec print out */
		mb(c << 2);
		break;

	    case 5:	/* Xgraph output */
		bandwidth(c << 2, 0);
		break;

	    default:	/* bandwidth print out */
		bandwidth(c << 2, 1);
		break;
	}
	exit(0);
}

long
getarg(s, ac, av)
	char   *s;
	char  **av;
{
	register len, i;

	len = strlen(s);

	for (i = 1; i < ac; ++i)
		if (!strncmp(av[i], s, len)) {
			register bs = atoi(&av[i][len]);

			if (rindex(&av[i][len], 'k'))
				bs *= 1024;
			else if (rindex(&av[i][len], 'm'))
				bs *= (1024 * 1024);
			if (!strncmp(av[i], "label", 5)) {
				return (long)(&av[i][len]);	/* HACK */
			}
			return (bs);
		}
	return (-1);
}

char	*output;

getfile(s, ac, av)
	char   *s;
	char  **av;
{
	register ret, len, i;

	len = strlen(s);

	for (i = 1; i < ac; ++i) {
		if (!strncmp(av[i], s, len)) {
			if (av[i][0] == 'o') {
				if (!strcmp("of=internal", av[i]))
					return (-2);
				if (!strcmp("of=stdout", av[i]))
					return (1);
				if (!strcmp("of=1", av[i]))
					return (1);
				if (!strcmp("of=-", av[i]))
					return (1);
				if (!strcmp("of=stderr", av[i]))
					return (2);
				if (!strcmp("of=2", av[i]))
					return (2);
				ret = creat(&av[i][len], 0644);
#ifdef	sgi
				if (getarg("direct=", ac, av) != -1) {
					close(ret);
					ret = open(&av[i][len], O_WRONLY|O_DIRECT);
				}
#endif
				if (ret == -1)
					error(&av[i][len]);
				output = &av[i][len];
				return (ret);
			} else {
				if (!strcmp("if=internal", av[i]))
					return (-2);
				if (!strcmp("if=stdin", av[i]))
					return (0);
				if (!strcmp("if=0", av[i]))
					return (0);
				if (!strcmp("if=-", av[i]))
					return (0);
				ret = open(&av[i][len], 0);
#ifdef	sgi
				if (getarg("direct=", ac, av) != -1) {
					close(ret);
					ret = open(&av[i][len], O_RDONLY|O_DIRECT);
				}
#endif
				if (ret == -1)
					error(&av[i][len]);
				return (ret);
			}
		}
	}
	return (-2);
}

#ifdef	FLUSH
flush()
{
	int	fd;
	struct	stat sb;
	caddr_t	where;

	if (output == NULL || (fd = open(output, 2)) == -1) {
		warning("No output file");
		return;
	}
	if (fstat(fd, &sb) == -1 || sb.st_size == 0) {
		warning(output);
		return;
	}
	where = mmap(0, sb.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	msync(where, sb.st_size, MS_INVALIDATE);
	/* XXX - didn't unmap */
}
#endif

warning(s)
	char   *s;
{
	if ((long)Label != -1) {
		fprintf(stderr, "%s: ", Label);
	}
	perror(s);
	return (-1);
}

error(s)
	char   *s;
{
	if ((long)Label != -1) {
		fprintf(stderr, "%s: ", Label);
	}
	perror(s);
	exit(1);
}
