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
#include "rpc/rpc.h"

#include "cmn.h"
#include "creds.h"

IDENTIFY("$Id: rpc_creds.c,v 1.1 1999/12/13 23:45:50 lward Exp $");

/*
 * Internalize credentials.
 */
int
xlate_rpccreds(struct svc_req *rqstp, struct creds *crp)
{

	switch (rqstp->rq_cred.oa_flavor) {

	case AUTH_UNIX:
		{
			struct authunix_parms *unix_cred;
			size_t	agi;
			size_t	indx;

			unix_cred = (struct authunix_parms *)rqstp->rq_clntcred;
			crp->cr_uid = unix_cred->aup_uid;
			crp->cr_groups[0] = unix_cred->aup_gid;
			crp->cr_ngroups = (size_t )unix_cred->aup_len;
			agi = 1;
			if (unix_cred->aup_len &&
			    (short )crp->cr_groups[0] != unix_cred->aup_gids[0])
				agi = 0;
			indx = 1;
			while (agi < unix_cred->aup_len && indx < XNFS_NGROUPS)
				crp->cr_groups[indx++] =
				    unix_cred->aup_gids[agi++];
			crp->cr_ngroups = indx;
		}
		break;
	case AUTH_NULL:
	case AUTH_DES:
	default:
		LOG(LOG_ERR,
		    "nfs_getcreds: unsupported or unknown auth type (%d)",
		    rqstp->rq_cred.oa_flavor);
		return FALSE;
	}

	return TRUE;
}
