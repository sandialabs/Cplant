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

#include "cnxsvc.h"

IDENTIFY("$Id");

#ifdef DEBUG
void
cnx_dbgsvc_typenam(cnx_typenam *arg, const char *ldr)
{

	LOG_DBG(CNX_SVCDBG_CHECK(1),
		"%stype: %.*s",
		ldr,
		CNX_MAXTYPNAMLEN,
		*arg);
}

void
cnx_dbgsvc_opaqarg(cnx_opaqarg *arg IS_UNUSED, const char *ldr)
{

	LOG_DBG(CNX_SVCDBG_CHECK(1), "%sopaqarg: <...>", ldr);
}

void
cnx_dbgsvc_mountarg(cnx_mountarg *arg, const char *ldr)
{
	char	*newldr;

	LOG_DBG(CNX_SVCDBG_CHECK(1), "%smountarg:", ldr);

	newldr = m_alloc(1 + strlen(ldr) + 1);
	if (newldr == NULL)
		panic("cnx_dbgsvc_mountarg: can't alloc");
	(void )sprintf(newldr, "%s ", ldr);
	cnx_dbgsvc_typenam(&arg->type, newldr);
	cnx_dbgsvc_path(&arg->path, newldr);
	cnx_dbgsvc_opaqarg(&arg->arg, newldr);
	free(newldr);
}

void
cnx_dbgsvc_status(cnx_status *arg, const char *ldr)
{
	const char *s;

	s = "<unknown status code>";
	switch (*arg) {

	case CNX_OK:
		s = "CNX_OK";
		break;
	case CNXERR_PERM:
		s = "CNXERR_PERM";
		break;
	case CNXERR_NOENT:
		s = "CNXERR_NOENT";
		break;
	case CNXERR_IO:
		s = "CNXERR_IO";
		break;
	case CNXERR_NXIO:
		s = "CNXERR_NXIO";
		break;
	case CNXERR_NOMEM:
		s = "CNXERR_NOMEM";
		break;
	case CNXERR_ACCES:
		s = "CNXERR_ACCES";
		break;
	case CNXERR_NOTBLK:
		s = "CNXERR_NOTBLK";
		break;
	case CNXERR_BUSY:
		s = "CNXERR_BUSY";
		break;
	case CNXERR_NODEV:
		s = "CNXERR_NODEV";
		break;
	case CNXERR_NOTDIR:
		s = "CNXERR_NOTDIR";
		break;
	case CNXERR_INVAL:
		s = "CNXERR_INVAL";
		break;
	case CNXERR_MFILE:
		s = "CNXERR_MFILE";
		break;
	case CNXERR_NAMETOOLONG:
		s = "CNXERR_NAMETOOLONG";
		break;
	default:
		break;
	}

	LOG_DBG(CNX_SVCDBG_CHECK(1), "%sstatus: %s", ldr, s);
}

void
cnx_dbgsvc_path(cnx_path *arg, const char *ldr)
{

	LOG_DBG(CNX_SVCDBG_CHECK(1), "%spath: %.*s", ldr, CNX_MAXPATHLEN, *arg);
}

void
cnx_dbgsvc_offerarg(cnx_offerarg *arg, const char *ldr)
{
	char	*newldr;

	LOG_DBG(CNX_SVCDBG_CHECK(1), "%sofferarg:", ldr);

	newldr = m_alloc(1 + strlen(ldr) + 1);
	if (newldr == NULL)
		panic("cnx_dbgsvc_offerarg: can't alloc");
	(void )sprintf(newldr, "%s ", ldr);
	cnx_dbgsvc_typenam(&arg->type, newldr);
	cnx_dbgsvc_opaqarg(&arg->arg, newldr);
	free(newldr);
}

void
cnx_dbgsvc_svcid(cnx_svcid *arg, const char *ldr)
{

	LOG_DBG(CNX_SVCDBG_CHECK(1),
		"%ssvcid: 0x%8lx",
		ldr,
		(unsigned long)*arg);
}

void
cnx_dbgsvc_svcidres(cnx_svcidres *result, const char *ldr)
{
	char	*newldr;

	LOG_DBG(CNX_SVCDBG_CHECK(1), "%ssvcidres:", ldr);

	newldr = m_alloc(1 + strlen(ldr) + 1);
	if (newldr == NULL)
		panic("cnx_dbgsvc_svcidres: can't alloc");
	(void )sprintf(newldr, "%s ", ldr);
	cnx_dbgsvc_status(&result->status, newldr);
	if (result->status == CNX_OK)
		cnx_dbgsvc_svcid(&result->cnx_svcidres_u.id, newldr);
	free(newldr);
}
#endif
