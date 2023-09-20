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
/*
 * $Id: cmn.h,v 1.7 2001/07/18 18:57:43 rklundt Exp $
 */
#ifndef _CMN_H
#define _CMN_H

/*
 * Source file version identification.
 */
#if defined(__GNUC__)
#define IDENTIFY(s) \
	static const char __attribute__ ((__unused__)) rcsid[] = s
#else
#define IDENTIFY(s) \
	static const char rcsid[] = s
#endif

/*
 * Common (tie) include file.
 */
#ifdef USE_MALLOC
#include <stdlib.h>
#endif
#include <syslog.h>
#include <stdarg.h>
#include <errno.h>

#include <sys/types.h>

/*
 * GCC nicety to get rid of unused parameter warnings.
 */
#if !defined(IS_UNUSED) && defined(__GNUC__)
#define IS_UNUSED	__attribute__ ((unused))
#else
#define IS_UNUSED
#endif

/*
 * Aliasing symbols to another name.
 */
#if !defined(ALIAS_TO) && defined(__GNUC__)
#define ALIAS_TO(s)	__attribute__ ((alias(s)))
#else
#define ALIAS_TO(s)
#endif

/*
 * The system's page size, in bytes.
 */
#define VMPAGESIZE	(sys_pagesize)

/*
 * For systems without ENOTSUP.
 */
#ifndef ENOTSUP
#define ENOTSUP		ENOSYS
#endif

/*
 * GCC nicety to enable inline expansion of small functions.
 */
#if defined(__GNUC__)
#define	INLINE		inline
#else
#define	INLINE
#endif

#ifndef min
#define min(x, y)	((x) < (y) ? (x) : (y))
#endif

#ifndef max
#define max(x, y)	((x) > (y) ? (x) : (y))
#endif

#ifndef rndup
#define rndup(x, y)	((((x) + (y) - 1) / (y)) * (y))
#endif
#ifndef rnddwn
#define rnddwn(x, y)	(((x) / (y)) * (y))
#endif

/*
 * Support for our own assert(3) call.
 */
#ifdef NDEBUG
#define _assert(e)	((void )0)
#define assert(e)	((void )0)
#else
#define assert(e)       ((e) ? (void)0 : _assert(#e, __FILE__, __LINE__))

extern void _assert(const char *, const char *, unsigned);
#endif

#ifndef NDEBUG
#define BUG_CHECK(e)	assert(e)
#else
#define BUG_CHECK(e)	((void )0)
#endif

#define my_perror(s)	_my_perror(__FILE__, __LINE__, (s))

#define panic(args...)	_panic(__FILE__, __LINE__, ## args)

#define LOG(pri, args...) \
	logmsg((pri), __FILE__, __LINE__, ## args)

extern const char revision[];			/* program revision number */
extern const char *build_info;			/* program build info */

extern size_t sys_pagesize;			/* system paging unit */
extern size_t heap_size;			/* size in byes of the heap */
extern unsigned nservers;			/* # threads */

#ifndef USE_MALLOC
/*
 * Heap allocator wait indicators.
 */
#define HEAP_NOBLOCK    1
#define HEAP_BLOCK      2

#define m_alloc(siz)	heap_alloc((siz), HEAP_NOBLOCK)
#define m_free(p)	heap_free(p)
#else
#define m_alloc(siz)	malloc(siz)
#define m_free(p)	free(p)
#endif

/*
 * Size of the message string buffer.
 */
#define MSGSTR_MAX	4096

/*
 * Debugging
 */
#ifdef DEBUG
#define LOG_DBG(e, args...) \
	if (e) LOG(LOG_DEBUG, ## args)
#else
#define LOG_DBG(e, args...)
#endif

extern const char *pgmname;

extern void _my_perror(const char *, unsigned, const char *);

extern int log_to(const char *);
extern void logs(int, const char *);
extern void logmsg(int, const char *, unsigned, const char *, ...);
extern void vlogmsg(int, const char *, unsigned, const char *, va_list ap);
extern void _panic(const char *, unsigned, const char *, ...);

extern int heap_init();
extern int heap_addToPool(void *, size_t);
extern void *heap_alloc(size_t, int);
extern void heap_free(void *);

struct creds;

#ifdef DEBUG
#define DBG_EXE(e, stmt) \
	do { \
		if (e) \
			(void )(stmt); \
	} while (0)

extern void debug_usage(void);
extern int debug_set_option(const char *);
extern void dbg_creds(struct creds *, const char *);
#else
#define DBG_EXE(e, stmt)
#endif

extern char *savstr(const char *);

extern int imcompress(const char *, size_t, char *, size_t *);
extern int imuncompress(const char *, size_t, char *, size_t *);

struct svc_req;

extern int service_create(u_long program, u_long version,
			  void (*dispatch)(),
			  int proto,
			  u_short port);
extern int xlate_rpccreds(struct svc_req *rqstp, struct creds *crp);
#endif /* !defined(_CMN_H) */
