#include <string.h>
#include "rpc/rpc.h"
#include <errno.h>

#include "queue.h"
#include "private.h"

IDENTIFY("$Id: svc_run.c,v 0.7 2001/07/18 18:57:50 rklundt Exp $");

/*
 * Thread args.
 */
struct targ {
	int	isauto;					/* internal thread? */
#ifdef INSTRUMENT
	u_long	requests;				/* rqsts serviced */
#endif
	cond_t	cond;					/* to sleep on */
	LIST_ENTRY(targ) link;				/* thread queue link */
};

#ifdef INSTRUMENT
#define TARG_SETUP(targp, autoflg) \
	do { \
		(targp)->isauto = (autoflg); \
		(targp)->requests = 0; \
		cond_init(&(targp)->cond); \
	} while(0)
#else
#define TARG_SETUP(targp, autoflg) \
	do { \
		(targp)->isauto = (autoflg); \
		cond_init(&(targp)->cond); \
	} while(0)
#endif

static mutex_t lock = MUTEX_INITIALIZER;

static volatile int proceed = 1;

/*
 * RISC processors may have a context cache. It can be beneficial to
 * reuse threads that have been used recently. We keep a LRU list of
 * threads to accomplish this.
 */
static LIST_HEAD(, targ) threads = LIST_HEAD_INITIALIZER(&threads);

#ifdef INSTRUMENT
static struct svc_stats {
	unsigned long hits;
	unsigned long awakenings;
	unsigned long autorun;
	unsigned long sleepers;
	unsigned long prunes;
} svc_stats = { 0, 0, 0, 0, 0 };
#endif

int	svc_mt_mode = RPC_SVC_MT_USER;			/* thread mode */
#ifndef linux
static unsigned auto_idle_lifetime = 30;		/* cleanup time */
#endif

static void t_svc_run(struct targ *);

#ifndef SINGLE_THREAD
static void
t_main(void)
{
	struct targ targ;

#ifdef INSTRUMENT
	mutex_lock(&lock);
	svc_stats.autorun++;
	mutex_unlock(&lock);
#endif
	TARG_SETUP(&targ, 1);
	t_svc_run(&targ);
	pthread_exit(NULL);
}
#endif

static int
wake_worker(void)
{
	struct targ *targp;
	int	running;

	proceed = 1;
	running = 0;
	targp = LIST_FIRST(&threads);
	if (targp != NULL) {
		LIST_REMOVE(targp, link);
		cond_signal(&targp->cond);
		running = 1;
#ifdef INSTRUMENT
		svc_stats.sleepers--;
#endif
	}

	return running;
}

static void
need_worker(void)
{
	int	running;
#ifndef SINGLE_THREAD
	pthread_t tid;
	int	err;
#endif

	mutex_lock(&lock);
	running = wake_worker();
	mutex_unlock(&lock);

#ifndef SINGLE_THREAD
	if (!running && svc_mt_mode == RPC_SVC_MT_AUTO) {
		err =
		    pthread_create(&tid, NULL, (void *(*)(void *))t_main, NULL);
		if (err)
			my_perror("svc_run: can't create thread");
		else {
			err = pthread_detach(tid);
			if (err)
				my_perror("svc_run: can't detach new thread");
		}
	}
#endif
}

/*
 * Idle a worker.
 *
 * NB: It is assumed that the caller holds the package mutex.
 */
static int
idle(struct targ *targp)
{
	int	err;

	/*
	 * Put self on the idle threads queue.
	 */
	LIST_INSERT_HEAD(&threads, targp, link);
#ifdef INSTRUMENT
	svc_stats.sleepers++;
#endif

#ifndef linux
	/*
	 * The timeout and cleanup thing doesn't work
	 * under Linux through, at least, RH6.0 w/ 2.2.10
	 * kernel. The system doesn't reap the zombie threads.
	 * Yes. We did detach. It was done immediately after
	 * the create. Perhaps we should pthread_create with
	 * a detached attribute?
	 *
	 * Might as well leave them hanging around then. They
	 * might get reused later. The alternative is to
	 * disallow auto mode or fill the system with zombies.
	 */
	if (targp->isauto) {
		struct timeval tv;
		struct timespec ts;

		assert(gettimeofday(&tv, NULL) == 0);
		TIMEVAL_TO_TIMESPEC(&tv, &ts);
		ts.tv_sec += auto_idle_lifetime;
		err = _cond_timedwait(&targp->cond, &lock, &ts);
	} else
#endif
		err = _cond_wait(&targp->cond, &lock);

	return err;
}

static void
t_svc_run(struct targ *targp)
{
	int	err;
	static volatile int nfds;
	static volatile fd_set read_fds;
	static volatile int fdgrp;
	static fd_mask *maskp;
	int	bit;
	extern int _rpc_dtablesize();
	extern void _t_svc_getrequest();

	while (1) {
		mutex_lock(&lock);
		/*
		 * Wait here until called for.
		 */
		while (!proceed) {
			err = idle(targp);
			if (err) {
				if (err != ETIMEDOUT)
					continue;
				/*
				 * Auto threads time out.
				 */
#ifdef INSTRUMENT
				svc_stats.prunes++;
#endif
				mutex_unlock(&lock);
				return;
			}
#ifdef INSTRUMENT
			svc_stats.awakenings++;
#endif
		}
		proceed = 0;
		mutex_unlock(&lock);

		/*
		 * Need to select?
		 */
		while (!nfds) {
			read_fds = svc_fdset;
			nfds =
			    _rpc_aware_select(_rpc_dtablesize(),
					      (fd_set *)&read_fds,
					      NULL,
					      NULL,
					      (struct timeval *)NULL);
#ifdef INSTRUMENT
			svc_stats.hits++;
#endif
			if (nfds <= 0) {
				if (nfds == 0)
					continue;
				nfds = 0;
				if (errno == EINTR)
					continue;
				mutex_lock(&lock);
				proceed = 1;
				(void )wake_worker();
				mutex_unlock(&lock);
				my_perror("svc_run: select failed");
				return;
			}
			fdgrp = 0;
			maskp = (fd_mask *)&read_fds;
		}
		nfds--;
		/*
		 * Find fildes.
		 */
		bit = 0;
		while (fdgrp < FD_SETSIZE) {
			bit = ffs(*maskp);
			if (bit) {
				*maskp ^= 1 << (bit - 1);
				break;
			}
			fdgrp += NFDBITS;
			maskp++;
		}
		if (bit) {
			/*
			 * Service the file descriptor.
			 */
			_t_svc_getrequest(fdgrp + bit - 1, need_worker);
#ifdef INSTRUMENT
			targp->requests++;
#endif
			continue;
		}
		assert(0);
	}
}

void
svc_run()
{
	struct targ targ;

	TARG_SETUP(&targ, 0);
	t_svc_run(&targ);
}
