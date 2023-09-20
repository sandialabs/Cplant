/*************************************************************************
Cplant Release Version 2.0.1.10
Release Date: Nov 5, 2002 
#############################################################################
#
#     This Cplant(TM) source code is the property of Sandia National
#     Laboratories.
#
#     This Cplant(TM) source code is copyrighted by Sandia National
#     Laboratories.
#
#     The redistribution of this Cplant(TM) source code is subject to the
#     terms of the GNU Lesser General Public License
#     (see cit/LGPL or http://www.gnu.org/licenses/lgpl.html)
#
#     Cplant(TM) Copyright 1998, 1999, 2000, 2001, 2002 Sandia Corporation. 
#     Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
#     license for use of this work by or on behalf of the US Government.
#     Export of this program may require a license from the United States
#     Government.
#
#############################################################################
**************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef linux
# include <sys/sysinfo.h>
# include <linux/version.h>  /* sysinfo() variations */
#endif
#ifndef USE_MALLOC
#include <sys/mman.h>
#endif

#include "cmn.h"
#include "rpc/rpc.h"
#include "rpc/pmap_clnt.h"
#include "queue.h"
#include "buffer.h"
#include "vfs.h"

#include "rpcsvc/cnx_prot.h"

IDENTIFY("$Id: main.c,v 1.9.4.1.4.1 2002/10/24 02:12:01 rklundt Exp $");

size_t	sys_pagesize = 0;				/* system page size */

static size_t heap_size = 0;				/* size of mem heap */
static void *heap = NULL;				/* memory heap */
#ifndef USE_MALLOC
static unsigned lock_heap = 0;				/* pin heap? */
#endif
#ifndef SINGLE_THREAD
unsigned	nservers = 0;				/* # service threads */
#else
unsigned	nservers = 1;				/* # service threads */
#endif

static const char *opts =
	"L:"
#ifndef USE_MALLOC
	"h:"
	"l"
#endif
#ifdef DEBUG
	"d:"
#endif
	"n:"
;
static const char *usgstr =
	"[-L log-spec] "
#ifndef USE_MALLOC
	"[-h heap-size] "
	"[-l] "
#endif
#ifdef DEBUG
	"[-d <var=lvl>] "
#endif
	"[-n nservers] "
;

#ifdef DEBUG
int	_rpcpmstart = 0;
#endif

const char *pgmname = NULL;				/* name at invocation */

/*
 * Internal super-user credentials.
 */
struct creds suser_creds = {
	SUSER_UID,
	0,
	{}
};

/*
 * Arguments to thread main.
 */
struct svc_arg {
	unsigned tno;					/* thread number */
};

static void svc_thread_run(struct svc_arg *);
static void initialize(void);
static int service_init(void);
static void usage(void);

int
main(int argc, char *const *argv)
{
	int	err;
	int	i;
	unsigned u;
#ifndef USE_MALLOC
	char	*cp;
#endif
#ifdef SINGLE_THREAD
	pid_t	pid;
#endif
	static char *logspec = "syslog:daemon";
#ifndef USE_MALLOC
	static long initial_heap[4096];
#endif
	struct svc_arg arg;

	pgmname = strrchr(argv[0], '/');
	pgmname = pgmname == NULL ? argv[0] : pgmname + 1;

#ifndef USE_MALLOC
	/*
	 * There is no heap from which to allocate memory yet. We
	 * need to set something up. It isn't the "real" heap though. Just
	 * enough to get us started. It's grown to the real size later on.
	 */
	if (heap_init() != 0) {
		(void )fprintf(stderr, "can't initialize heap allocator\n");
		exit(1);
	}
	if (heap_addToPool(initial_heap, sizeof(initial_heap)) != 0) {
		(void )fprintf(stderr,
			       "can't set initial pool for heap allocator\n");
		exit(1);
	}
#endif

	err = 0;
	while ((i = getopt(argc, argv, opts)) != EOF) {
		switch (i) {
		case 'L':				/* add log mechanism */
			if (log_to(optarg) != 0) {
				(void )fprintf(stderr,
					       "log spec \"%s\": %s\n",
					       optarg,
					       strerror(errno));
				exit(1);
			}
			logspec = NULL;
			break;
#ifndef USE_MALLOC
		case 'h':
			heap_size = strtoul(optarg, &cp, 0);
			if (*cp != '\0' || heap_size < (size_t )getpagesize()) {
				(void )fprintf(stderr,
					       "bad heap size\n");
				exit(1);
			}
			break;
		case 'l':
			lock_heap = 1;
			break;
#endif
#ifdef DEBUG
		case 'd':				/* debug */
			err = debug_set_option(optarg);
			break;
#endif
		case 'n':				/* # servers */
			nservers = atoi(optarg);
			if (nservers < 1) {
				(void )fprintf(stderr,
					       "Bad number of servers\n");
				err = 1;
			}
#ifdef SINGLE_THREAD
			(void )fprintf(stderr,
				       "Nservers ignored -- single threaded\n");
			nservers = 1;
#endif
			break;
		default:
			err = 1;
			break;
		}
		if (err)
			usage();
	}

	if (argc - optind != 0)
		usage();

	/*
	 * Add default log if set.
	 */
	if (logspec != NULL && log_to(logspec) != 0) {
		(void  )fprintf(stderr,
				"default log spec \"%s\": %s\n",
				logspec, strerror(errno));
		exit(1);
	}

	LOG(LOG_INFO, "%s; Built %s", revision, build_info);

	initialize();

	err = service_init();
	if (err)
		panic("can't initialize remote services offerring");

#ifndef SINGLE_THREAD
	for (u = 1; u < nservers; u++)
	{
		pthread_t tid;
		struct svc_arg *argp;

		argp = m_alloc(sizeof(struct svc_arg));
		if (argp == NULL) {
			my_perror("main: m_alloc(thread arg)");
			continue;
		}
		argp->tno = u;
		err =
                    pthread_create(&tid,
				   NULL,
				   (void *(*)(void *))svc_thread_run,
				   argp);
                if (err)
                        my_perror("main: can't create thread");
                else {
                        err = pthread_detach(tid);
                        if (err)
                                my_perror("main: can't detach new thread");
                }
	}
#else
	pid = getpid();
	for (u = 1; pid == getpid() && u < nservers; u++) {
		switch((err = fork())) {

		case -1:
			my_perror("fork");
			/*
			 * Fall into...
			 */
		default:
			break;
		}
	}
#endif
#ifndef USE_MALLOC
	/*
	 * Mlock() doesn't work across fork. Pin the heap pages now.
	 */
	if (lock_heap && mlock(heap, heap_size) != 0)
		LOG(LOG_WARNING, "main: can't pin heap (%s)", strerror(errno));
#endif

	LOG(LOG_INFO, "servicing requests");
	arg.tno = 0;
	svc_thread_run(&arg);

	return 0;
}

static void
svc_thread_run(struct svc_arg *argp)
{

#ifndef SINGLE_THREAD
	LOG(LOG_DEBUG,
	    "svc_thread_run: thread %u (tid " THREAD_T_FMT ", pid %lu) begun",
	    argp->tno,
	    thread_self(),
	    (unsigned long )getpid());
#else
	LOG(LOG_DEBUG,
	    "svc_thread_run: thread %u (pid %lu) begun",
	    argp->tno,
	    (unsigned long )getpid());
#endif

	svc_run();
	LOG(LOG_INFO, "svc_run returned");
}

static size_t
size_mem(void)
{
#ifndef linux
	int	i;
#else
	struct sysinfo si;
#endif
	size_t	n;

	/*
	 * If the heap size has already been set, use that. It better
	 * be right. We don't verify the value!
	 */
	if (heap_size)
		return heap_size;

#ifdef linux
	if (sysinfo(&si) < 0)
	    n = 0;
	else
	    n = si.freeram + si.bufferram;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,23))
	/*
	 * Units are reported in different units (pages) in modern kernels,
	 * and glibc doesn't seem to know about it.  Hope we aren't compiling
	 * on a 2.3.large and running on a 2.2 or vice-versa.  When 2.2 is
	 * dead and buried we'll make this unconditional.   --pw
	 */
	n *= si.mem_unit;
#endif
#else
	i = sysconf(_SC_AVPHYS_PAGES);
	n = i < 0 ? 0 : (size_t )i * VMPAGESIZE;
#endif
	if (!n)
		panic("size_mem: can't size memory");

	return n;
}

static void
setup_heap(void)
{
	size_t	n;
	size_t	mb;

	heap_size = size_mem();

	/*
	 * Populate the heap allocator.
	 */
	n = heap_size / 10;
	if (n > 10 * 1024 * 1024)
		n = 80 * 1024 * 1024;
	n = rndup(n, VMPAGESIZE);
#ifndef USE_MALLOC
	heap = valloc(heap_size);
#else
	heap = valloc(heap_size - n);
#endif
	if (heap == NULL)
		panic("setup_heap: no memory for heap?");
#ifndef USE_MALLOC
	if (heap_addToPool(heap, n) != 0)
		panic("setup_heap: can't create initial pool");
	heap += n;
#endif
	heap_size -= n;

	mb = heap_size / 1000000;
	LOG(LOG_INFO,
	    "%lu.%02lu MB memory reserved for buffers",
	    (unsigned long )mb,
	    (unsigned long )mb % mb);

#if 0
	/*
	 * Shouldn't buf_init adjust heap and heap_size according
	 * to how much it takes? We're assuming all but that
	 * may not be true in the future. Fix me!
	 */
	buf_init(heap, heap_size);
	heap = NULL;
	heap_size = 0;
#endif
}

/*
 * Remote services initialization.
 */
static int
service_init(void)
{
	int	err;
	extern void cnx_initialize_server(void);
	extern void cnx_program_1(struct svc_req, SVCXPRT *);
#if 0
	extern void mountprog_1(struct svc_req, SVCXPRT *);
	extern void nfs_program_2(struct svc_req, SVCXPRT *);
#endif


	cnx_initialize_server();
#if 0
	nfs_initialize_server();
	mount_initialize_server();
#endif

	(void )pmap_unset(CNX_PROGRAM, CNX_VERSION);
	err =
	    service_create(CNX_PROGRAM, CNX_VERSION,
			   cnx_program_1,
			   IPPROTO_UDP,
			   0);
	if (err) {
		LOG(LOG_ERR,
		    "couldn't create (CNX_PROGRAM, CNX_VERSION, udp) service");
		return err;
	}
#if 0
	err = service_create(MOUNTPROG, MOUNTVERS, mountprog_1, IPPROTO_UDP, 0);
	if (err) {
		LOG(LOG_ERR,
		    "couldn't create (MOUNTPROG, MOUNTVERS, udp) service");
		return err;
	}
	err =
	    service_create(NFS_PROGRAM, NFS_VERSION,
			   nfs_program_2,
			   IPPROTO_UDP,
			   NFS_PORT);
	if (err) {
		LOG(LOG_ERR,
		    "couldn't create (NFS_PROGRAM, NFS_VERSION, udp) service");
		return err;
	}
#endif
	return 0;
}

/*
 * Program initialization.
 */
static void
initialize(void)
{

	/*
	 * Let the RPC library create service threads as needed.
	 */
#ifndef SINGLE_THREAD
	svc_mt_mode = nservers ? RPC_SVC_MT_USER : RPC_SVC_MT_AUTO;
#endif

	sys_pagesize = getpagesize();

	//setup_heap();

	vfs_init();

#if 0
	nfs_initialize_server();
	mount_initialize_server();
#endif

}

/*
 * Print usage message.
 */
static void
usage()
{

	(void )fprintf(stderr, "Usage: %s %s\n", pgmname, usgstr);
#ifdef DEBUG
	debug_usage();
#endif
	exit(1);
}
