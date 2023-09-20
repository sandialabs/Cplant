/*
 *	@(#)test5a.c	1.5	99/02/04 Connectathon Testsuite
 *	1.3 Lachman ONC Test Suite source
 *
 * Test write
 *
 * Uses the following important system calls against the server:
 *
 *	chdir()
 *	mkdir()		(for initial directory creation if not -m)
 *	creat()
 *	open()
 *	read()
 *	write()
 *	stat()
 *	fstat()
 */

#if defined (DOS) || defined (WIN32)
/* If Dos, Windows or Win32 */
#define DOSorWIN32
/* Synchronous Write is Not supported in the Windows version yet.*/
#undef O_SYNC
#endif

#ifndef DOSorWIN32
#include <sys/param.h>
#include <unistd.h>
#endif

#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#ifdef MMAP
#include <sys/mman.h>
#endif

#include "../tests.h"

#if defined(O_SYNC) || defined(DOSorWIN32)
#define USE_OPEN
#endif

#ifndef MIN
#define MIN(a, b)	((a) < (b) ? (a) : (b))
#endif

#define	BUFSZ	8192
#define	DSIZE	1048576

static int Tflag = 0;	/* print timing */
static int Fflag = 0;	/* test function only;  set count to 1, negate -t */
static int Nflag = 0;	/* Suppress directory operations */
#ifdef O_SYNC
static int Sflag = 0;	/* use synchronous writes */
#endif

static void
usage()
{
#ifdef O_SYNC
	fprintf(stdout, "usage: %s [-htfns] [size count fname]\n", Myname);
#else
	fprintf(stdout, "usage: %s [-htfn] [size count fname]\n", Myname);
#endif
	fprintf(stdout, "  Flags:  h    Help - print this usage info\n");
	fprintf(stdout, "          t    Print execution time statistics\n");
	fprintf(stdout, "          f    Test function only (negate -t)\n");
	fprintf(stdout, "          n    Suppress test directory create operations\n");
#ifdef O_SYNC
	fprintf(stdout, "          s    Use synchronous writes\n");
#endif
}

main(argc, argv)
	int argc;
	char *argv[];
{
	int count = DCOUNT;	/* times to do each file */
	int ct;
	off_t size = DSIZE;
	off_t si;
	int i;
	int fd;
	off_t bytes = 0;
#ifdef USE_OPEN
	int roflags;			/* open read-only flags */
	int woflags;			/* write-only create flags */
#endif
	char *bigfile = "bigfile";
	struct timeval time;
	struct stat statb;
	char *opts;
	char buf[BUFSZ];
	double etime;
#ifdef MMAP
	caddr_t maddr;
#endif

	umask(0);
	setbuf(stdout, NULL);
	Myname = *argv++;
	argc--;
	while (argc && **argv == '-') {
		for (opts = &argv[0][1]; *opts; opts++) {
			switch (*opts) {
				case 'h':	/* help */
					usage();
					exit(1);
					break;

				case 't':	/* time */
					Tflag++;
					break;

				case 'f':	/* funtionality */
					Fflag++;
					break;

				case 'n':	/* No Test Directory create */
					Nflag++;
					break;

#ifdef O_SYNC
				case 's':	/* synchronous writes */
					Sflag++;
					break;
#endif

				default:
					error("unknown option '%c'", *opts);
					usage();
					exit(1);
			}
		}
		argc--;
		argv++;
	}

	if (argc) {
		size = getparm(*argv, 1, "size");
		if (size <= 0) {
			usage();
			exit(1);
		}
		argv++;
		argc--;
	}
	if (argc) {
		count = getparm(*argv, 1, "count");
		if (count <= 0) {
			usage();
			exit(1);
		}
		argv++;
		argc--;
	}
	if (argc) {
		bigfile = *argv;
		argv++;
		argc--;
	}
	if (argc) {
		usage();
		exit(1);
	}

	if (Fflag) {
		Tflag = 0;
		count = 1;
	}

#ifdef USE_OPEN
	woflags = O_WRONLY|O_CREAT|O_TRUNC;
	roflags = O_RDONLY;
#ifdef O_SYNC
	if (Sflag) {
		woflags |= O_SYNC;
	}
#endif
#ifdef DOSorWIN32
	woflags |= O_BINARY | O_RDWR;	/* create and open file */
	roflags |= O_BINARY;
#endif
#endif /* USE_OPEN */

	fprintf(stdout, "%s: write\n", Myname);

	if (!Nflag)
		testdir(NULL);
	else
		mtestdir(NULL);

	for (i = 0; i < BUFSZ / sizeof (int); i++) {
		((int *)buf)[i] = i;
	}

	if (Tflag) {
		starttime();
	}

	for (ct = 0; ct < count; ct++) {
#ifdef USE_OPEN
		if ((fd = open(bigfile, woflags, CHMOD_RW)) < 0) {
#else
		if ((fd = creat(bigfile, CHMOD_RW)) < 0) {
#endif
			error("can't create '%s'", bigfile);
			exit(1);
		}
		if (stat(bigfile, &statb) < 0) {
			error("can't stat '%s'", bigfile);
			exit(1);
		}
		if (statb.st_size != 0) {
			error("'%s' has size %ld, should be 0",
			      bigfile, (long)statb.st_size);
			exit(1);
		}
		for (si = size; si > 0; si -= bytes) {
			bytes = MIN(BUFSZ, si);
			if (write(fd, buf, bytes) != bytes) {
				error("'%s' write failed", bigfile);
				exit(1);
			}
		}
		if (close(fd) < 0) {
			error("can't close %s", bigfile);
			exit(1);
		}
		if (stat(bigfile, &statb) < 0) {
			error("can't stat '%s'", bigfile);
			exit(1);
		}
		if (statb.st_size != size) {
			error("'%s' has size %ld, should be %ld",
			      bigfile, (long)(statb.st_size), (long)size);
			exit(1);
		}
	}

	if (Tflag) {
		endtime(&time);
	}

	if ((fd = open(bigfile, roflags)) < 0) {
		error("can't open '%s'", bigfile);
		exit(1);
	}
#ifdef MMAP
	maddr = mmap((caddr_t)0, (size_t)size, PROT_READ,
			MAP_PRIVATE, fd, (off_t)0);
	if (maddr == NULL) {
		error("can't mmap '%s'", bigfile);
		exit(1);
	}
	if (msync(maddr, (size_t)size, MS_INVALIDATE) < 0) {
		error("can't invalidate pages for '%s'", bigfile);
		exit(1);
	}
	if (munmap(maddr, (size_t)size) < 0) {
		error("can't munmap '%s'", bigfile);
		exit(1);
	}
#endif
	for (si = size; si > 0; si -= bytes) {
		bytes = MIN(BUFSZ, si);
		if (read(fd, buf, bytes) != bytes) {
			error("'%s' read failed", bigfile);
			exit(1);
		}
		for (i = 0; i < bytes / sizeof (int); i++) {
			if (((int *)buf)[i] != i) {
				error("bad data in '%s'", bigfile);
				exit(1);
			}
		}
	}
	close(fd);

	fprintf(stdout, "\twrote %ld byte file %d times", (long)size, count);

	if (Tflag) {
		etime = (double)time.tv_sec + (double)time.tv_usec / 1000000.0;
		if (etime != 0.0) {
			fprintf(stdout, " in %ld.%-2ld seconds (%ld bytes/sec)",
				(long)time.tv_sec, (long)time.tv_usec / 10000,
				(long)((double)size * ((double)count / etime)));
		} else {
			fprintf(stdout, " in %ld.%-2ld seconds (> %ld bytes/sec)",
				(long)time.tv_sec, (long)time.tv_usec / 10000,
				(long)size * count);
		}
	}
	fprintf(stdout, "\n");

	complete();
}
