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
 * $Id: nlm_prot.h,v 0.1 1999/08/17 05:42:21 lee Stab $
 */
#ifndef _NLM_PROT_
#define	_NLM_PROT_

#include "rpc/rpc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LM_MAXSTRLEN	1024
#define MAXNAMELEN	LM_MAXSTRLEN+1

enum nlm_stats {
	nlm_granted = 0,
	nlm_denied = 1,
	nlm_denied_nolocks = 2,
	nlm_blocked = 3,
	nlm_denied_grace_period = 4
};
typedef enum nlm_stats nlm_stats;

struct nlm_holder {
	bool_t exclusive;
	int svid;
	netobj oh;
	u_int l_offset;
	u_int l_len;
};
typedef struct nlm_holder nlm_holder;

struct nlm_testrply {
	nlm_stats stat;
	union {
		struct nlm_holder holder;
	} nlm_testrply_u;
};
typedef struct nlm_testrply nlm_testrply;

struct nlm_stat {
	nlm_stats stat;
};
typedef struct nlm_stat nlm_stat;

struct nlm_res {
	netobj cookie;
	nlm_stat stat;
};
typedef struct nlm_res nlm_res;

struct nlm_testres {
	netobj cookie;
	nlm_testrply stat;
};
typedef struct nlm_testres nlm_testres;

struct nlm_lock {
	char *caller_name;
	netobj fh;
	netobj oh;
	int svid;
	u_int l_offset;
	u_int l_len;
};
typedef struct nlm_lock nlm_lock;

struct nlm_lockargs {
	netobj cookie;
	bool_t block;
	bool_t exclusive;
	struct nlm_lock alock;
	bool_t reclaim;
	int state;
};
typedef struct nlm_lockargs nlm_lockargs;

struct nlm_cancargs {
	netobj cookie;
	bool_t block;
	bool_t exclusive;
	struct nlm_lock alock;
};
typedef struct nlm_cancargs nlm_cancargs;

struct nlm_testargs {
	netobj cookie;
	bool_t exclusive;
	struct nlm_lock alock;
};
typedef struct nlm_testargs nlm_testargs;

struct nlm_unlockargs {
	netobj cookie;
	struct nlm_lock alock;
};
typedef struct nlm_unlockargs nlm_unlockargs;
/*
 * The following enums are actually bit encoded for efficient
 * boolean algebra.... DON'T change them.....
 */

enum fsh_mode {
	fsm_DN = 0,
	fsm_DR = 1,
	fsm_DW = 2,
	fsm_DRW = 3
};
typedef enum fsh_mode fsh_mode;

enum fsh_access {
	fsa_NONE = 0,
	fsa_R = 1,
	fsa_W = 2,
	fsa_RW = 3
};
typedef enum fsh_access fsh_access;

struct nlm_share {
	char *caller_name;
	netobj fh;
	netobj oh;
	fsh_mode mode;
	fsh_access access;
};
typedef struct nlm_share nlm_share;

struct nlm_shareargs {
	netobj cookie;
	nlm_share share;
	bool_t reclaim;
};
typedef struct nlm_shareargs nlm_shareargs;

struct nlm_shareres {
	netobj cookie;
	nlm_stats stat;
	int sequence;
};
typedef struct nlm_shareres nlm_shareres;

struct nlm_notify {
	char *name;
	long state;
};
typedef struct nlm_notify nlm_notify;

#define	NLM_PROG ((unsigned long)(100021))
#define	NLM_VERS ((unsigned long)(1))

#if defined(__STDC__) || defined(__cplusplus)
#define	NLM_TEST ((unsigned long)(1))
extern  enum clnt_stat nlm_test_1(struct nlm_testargs *, nlm_testres *, CLIENT *);
extern  bool_t nlm_test_1_svc(struct nlm_testargs *, nlm_testres *, struct svc_req *);
#define	NLM_LOCK ((unsigned long)(2))
extern  enum clnt_stat nlm_lock_1(struct nlm_lockargs *, nlm_res *, CLIENT *);
extern  bool_t nlm_lock_1_svc(struct nlm_lockargs *, nlm_res *, struct svc_req *);
#define	NLM_CANCEL ((unsigned long)(3))
extern  enum clnt_stat nlm_cancel_1(struct nlm_cancargs *, nlm_res *, CLIENT *);
extern  bool_t nlm_cancel_1_svc(struct nlm_cancargs *, nlm_res *, struct svc_req *);
#define	NLM_UNLOCK ((unsigned long)(4))
extern  enum clnt_stat nlm_unlock_1(struct nlm_unlockargs *, nlm_res *, CLIENT *);
extern  bool_t nlm_unlock_1_svc(struct nlm_unlockargs *, nlm_res *, struct svc_req *);
#define	NLM_GRANTED ((unsigned long)(5))
extern  enum clnt_stat nlm_granted_1(struct nlm_testargs *, nlm_res *, CLIENT *);
extern  bool_t nlm_granted_1_svc(struct nlm_testargs *, nlm_res *, struct svc_req *);
#define	NLM_TEST_MSG ((unsigned long)(6))
extern  enum clnt_stat nlm_test_msg_1(struct nlm_testargs *, void *, CLIENT *);
extern  bool_t nlm_test_msg_1_svc(struct nlm_testargs *, void *, struct svc_req *);
#define	NLM_LOCK_MSG ((unsigned long)(7))
extern  enum clnt_stat nlm_lock_msg_1(struct nlm_lockargs *, void *, CLIENT *);
extern  bool_t nlm_lock_msg_1_svc(struct nlm_lockargs *, void *, struct svc_req *);
#define	NLM_CANCEL_MSG ((unsigned long)(8))
extern  enum clnt_stat nlm_cancel_msg_1(struct nlm_cancargs *, void *, CLIENT *);
extern  bool_t nlm_cancel_msg_1_svc(struct nlm_cancargs *, void *, struct svc_req *);
#define	NLM_UNLOCK_MSG ((unsigned long)(9))
extern  enum clnt_stat nlm_unlock_msg_1(struct nlm_unlockargs *, void *, CLIENT *);
extern  bool_t nlm_unlock_msg_1_svc(struct nlm_unlockargs *, void *, struct svc_req *);
#define	NLM_GRANTED_MSG ((unsigned long)(10))
extern  enum clnt_stat nlm_granted_msg_1(struct nlm_testargs *, void *, CLIENT *);
extern  bool_t nlm_granted_msg_1_svc(struct nlm_testargs *, void *, struct svc_req *);
#define	NLM_TEST_RES ((unsigned long)(11))
extern  enum clnt_stat nlm_test_res_1(nlm_testres *, void *, CLIENT *);
extern  bool_t nlm_test_res_1_svc(nlm_testres *, void *, struct svc_req *);
#define	NLM_LOCK_RES ((unsigned long)(12))
extern  enum clnt_stat nlm_lock_res_1(nlm_res *, void *, CLIENT *);
extern  bool_t nlm_lock_res_1_svc(nlm_res *, void *, struct svc_req *);
#define	NLM_CANCEL_RES ((unsigned long)(13))
extern  enum clnt_stat nlm_cancel_res_1(nlm_res *, void *, CLIENT *);
extern  bool_t nlm_cancel_res_1_svc(nlm_res *, void *, struct svc_req *);
#define	NLM_UNLOCK_RES ((unsigned long)(14))
extern  enum clnt_stat nlm_unlock_res_1(nlm_res *, void *, CLIENT *);
extern  bool_t nlm_unlock_res_1_svc(nlm_res *, void *, struct svc_req *);
#define	NLM_GRANTED_RES ((unsigned long)(15))
extern  enum clnt_stat nlm_granted_res_1(nlm_res *, void *, CLIENT *);
extern  bool_t nlm_granted_res_1_svc(nlm_res *, void *, struct svc_req *);
extern int nlm_prog_1_freeresult(SVCXPRT *, xdrproc_t, caddr_t);

#else /* K&R C */
#define	NLM_TEST ((unsigned long)(1))
extern  enum clnt_stat nlm_test_1();
extern  bool_t nlm_test_1_svc();
#define	NLM_LOCK ((unsigned long)(2))
extern  enum clnt_stat nlm_lock_1();
extern  bool_t nlm_lock_1_svc();
#define	NLM_CANCEL ((unsigned long)(3))
extern  enum clnt_stat nlm_cancel_1();
extern  bool_t nlm_cancel_1_svc();
#define	NLM_UNLOCK ((unsigned long)(4))
extern  enum clnt_stat nlm_unlock_1();
extern  bool_t nlm_unlock_1_svc();
#define	NLM_GRANTED ((unsigned long)(5))
extern  enum clnt_stat nlm_granted_1();
extern  bool_t nlm_granted_1_svc();
#define	NLM_TEST_MSG ((unsigned long)(6))
extern  enum clnt_stat nlm_test_msg_1();
extern  bool_t nlm_test_msg_1_svc();
#define	NLM_LOCK_MSG ((unsigned long)(7))
extern  enum clnt_stat nlm_lock_msg_1();
extern  bool_t nlm_lock_msg_1_svc();
#define	NLM_CANCEL_MSG ((unsigned long)(8))
extern  enum clnt_stat nlm_cancel_msg_1();
extern  bool_t nlm_cancel_msg_1_svc();
#define	NLM_UNLOCK_MSG ((unsigned long)(9))
extern  enum clnt_stat nlm_unlock_msg_1();
extern  bool_t nlm_unlock_msg_1_svc();
#define	NLM_GRANTED_MSG ((unsigned long)(10))
extern  enum clnt_stat nlm_granted_msg_1();
extern  bool_t nlm_granted_msg_1_svc();
#define	NLM_TEST_RES ((unsigned long)(11))
extern  enum clnt_stat nlm_test_res_1();
extern  bool_t nlm_test_res_1_svc();
#define	NLM_LOCK_RES ((unsigned long)(12))
extern  enum clnt_stat nlm_lock_res_1();
extern  bool_t nlm_lock_res_1_svc();
#define	NLM_CANCEL_RES ((unsigned long)(13))
extern  enum clnt_stat nlm_cancel_res_1();
extern  bool_t nlm_cancel_res_1_svc();
#define	NLM_UNLOCK_RES ((unsigned long)(14))
extern  enum clnt_stat nlm_unlock_res_1();
extern  bool_t nlm_unlock_res_1_svc();
#define	NLM_GRANTED_RES ((unsigned long)(15))
extern  enum clnt_stat nlm_granted_res_1();
extern  bool_t nlm_granted_res_1_svc();
extern int nlm_prog_1_freeresult();
#endif /* K&R C */
#define	NLM_VERSX ((unsigned long)(3))

#if defined(__STDC__) || defined(__cplusplus)
#define	NLM_SHARE ((unsigned long)(20))
extern  enum clnt_stat nlm_share_3(nlm_shareargs *, nlm_shareres *, CLIENT *);
extern  bool_t nlm_share_3_svc(nlm_shareargs *, nlm_shareres *, struct svc_req *);
#define	NLM_UNSHARE ((unsigned long)(21))
extern  enum clnt_stat nlm_unshare_3(nlm_shareargs *, nlm_shareres *, CLIENT *);
extern  bool_t nlm_unshare_3_svc(nlm_shareargs *, nlm_shareres *, struct svc_req *);
#define	NLM_NM_LOCK ((unsigned long)(22))
extern  enum clnt_stat nlm_nm_lock_3(nlm_lockargs *, nlm_res *, CLIENT *);
extern  bool_t nlm_nm_lock_3_svc(nlm_lockargs *, nlm_res *, struct svc_req *);
#define	NLM_FREE_ALL ((unsigned long)(23))
extern  enum clnt_stat nlm_free_all_3(nlm_notify *, void *, CLIENT *);
extern  bool_t nlm_free_all_3_svc(nlm_notify *, void *, struct svc_req *);
extern int nlm_prog_3_freeresult(SVCXPRT *, xdrproc_t, caddr_t);

#else /* K&R C */
#define	NLM_SHARE ((unsigned long)(20))
extern  enum clnt_stat nlm_share_3();
extern  bool_t nlm_share_3_svc();
#define	NLM_UNSHARE ((unsigned long)(21))
extern  enum clnt_stat nlm_unshare_3();
extern  bool_t nlm_unshare_3_svc();
#define	NLM_NM_LOCK ((unsigned long)(22))
extern  enum clnt_stat nlm_nm_lock_3();
extern  bool_t nlm_nm_lock_3_svc();
#define	NLM_FREE_ALL ((unsigned long)(23))
extern  enum clnt_stat nlm_free_all_3();
extern  bool_t nlm_free_all_3_svc();
extern int nlm_prog_3_freeresult();
#endif /* K&R C */

/* the xdr functions */

#if defined(__STDC__) || defined(__cplusplus)
extern  bool_t xdr_nlm_stats(XDR *, nlm_stats*);
extern  bool_t xdr_nlm_holder(XDR *, nlm_holder*);
extern  bool_t xdr_nlm_testrply(XDR *, nlm_testrply*);
extern  bool_t xdr_nlm_stat(XDR *, nlm_stat*);
extern  bool_t xdr_nlm_res(XDR *, nlm_res*);
extern  bool_t xdr_nlm_testres(XDR *, nlm_testres*);
extern  bool_t xdr_nlm_lock(XDR *, nlm_lock*);
extern  bool_t xdr_nlm_lockargs(XDR *, nlm_lockargs*);
extern  bool_t xdr_nlm_cancargs(XDR *, nlm_cancargs*);
extern  bool_t xdr_nlm_testargs(XDR *, nlm_testargs*);
extern  bool_t xdr_nlm_unlockargs(XDR *, nlm_unlockargs*);
extern  bool_t xdr_fsh_mode(XDR *, fsh_mode*);
extern  bool_t xdr_fsh_access(XDR *, fsh_access*);
extern  bool_t xdr_nlm_share(XDR *, nlm_share*);
extern  bool_t xdr_nlm_shareargs(XDR *, nlm_shareargs*);
extern  bool_t xdr_nlm_shareres(XDR *, nlm_shareres*);
extern  bool_t xdr_nlm_notify(XDR *, nlm_notify*);

#else /* K&R C */
extern bool_t xdr_nlm_stats();
extern bool_t xdr_nlm_holder();
extern bool_t xdr_nlm_testrply();
extern bool_t xdr_nlm_stat();
extern bool_t xdr_nlm_res();
extern bool_t xdr_nlm_testres();
extern bool_t xdr_nlm_lock();
extern bool_t xdr_nlm_lockargs();
extern bool_t xdr_nlm_cancargs();
extern bool_t xdr_nlm_testargs();
extern bool_t xdr_nlm_unlockargs();
extern bool_t xdr_fsh_mode();
extern bool_t xdr_fsh_access();
extern bool_t xdr_nlm_share();
extern bool_t xdr_nlm_shareargs();
extern bool_t xdr_nlm_shareres();
extern bool_t xdr_nlm_notify();

#endif /* K&R C */

#ifdef __cplusplus
}
#endif

#endif /* !_NLM_PROT_ */
