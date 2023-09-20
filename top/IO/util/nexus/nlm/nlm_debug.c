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
static void
print_line(struct mstr *mstrp, const char *fmt, ...)
{
	char	*buf;
	char	*scp, *dcp;

	/*
	 * Create the concatenated leader string.
	 */
	buf = malloc(MSGSTR_MAX);
	if (buf == NULL) {
		LOG(LOG_ERR, "can't allocate message buffer");
		return;
	}
	dcp = buf;
	for (sstrp = TAILQ_LAST(mstrp, mstr);
	     sstrp != NULL;
	     sstrp = TAILQ_PREV(sstrp, mstr, link)) {
		scp = sstrp->s;
		while (*scp != '\0' && dcp < ldr + MAXLDRLEN)
			*dcp++ = *scp++;
	}

	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);
	LOG(LOG_DEBUG, buf);

	free(buf);
}

static void
_nlm_dbgsvc_cancargs(struct mstr *mstrp, nlm_cancargs *arg)
{
	struct sstr sstr;

	print_line(mstrp,
		   "nlm_cancargs: block %s, exclusive %s",
		   arg->block,
		   arg->exclusive);
	sstr.s = " ";
	TAILQ_INSERT_HEAD(mstrp, &sstr, link);
	_nlm_dbgsvc_cookie(mstrp, &arg->cookie);
	_nlm_dbgsvc_lock(mstrp, &arg->alock);
	TAILQ_REMOVE(mstrp, &sstr);
}

void
nlm_dbgsvc_cancargs(nlm_cancargs *arg, const char *ldr)
{
	struct sstr sstr;
	struct mstr mstr;

	TAILQ_INIT(&mstr);
	sstr.s = ldr;
	TAILQ_INSERT_HEAD(&mstr, &sstr, link);
	nlm_dbgsvc_cancargs(&mstr, arg);
}

static void
_nlm_dbgsvc_lockargs(struct mstr *mstrp, nlm_lockargs *arg)
{

	LOG_DBG(NLM_SVCDBG_CHECK(1),
		"nlm_lockargs: block %s, exclusive %s",
		arg->block,
		arg->exclusive);
}

void
nlm_dbgsvc_lockargs(nlm_lockargs *arg, const char *ldr)
{
	struct sstr sstr;
	struct mstr mstr;

	TAILQ_INIT(&mstr);
	sstr.s = ldr;
	TAILQ_INSERT_HEAD(&mstr, &sstr, link);
	nlm_dbgsvc_cancargs(&mstr, arg);
}

static void
_nlm_dbgsvc_notify(struct mstr *mstrp, nlm_notify *arg)
{

}

void
nlm_dbgsvc_notify(nlm_notify *arg, const char *ldr)
{
	struct sstr sstr;
	struct mstr mstr;

	TAILQ_INIT(&mstr);
	sstr.s = ldr;
	TAILQ_INSERT_HEAD(&mstr, &sstr, link);
	nlm_dbgsvc_cancargs(&mstr, arg);
}

static void
_nlm_dbgsvc_res(struct mstr *mstrp, nlm_res *result)
{

}

void
nlm_dbgsvc_res(nlm_res *result, const char *ldr)
{
	struct sstr sstr;
	struct mstr mstr;

	TAILQ_INIT(&mstr);
	sstr.s = ldr;
	TAILQ_INSERT_HEAD(&mstr, &sstr, link);
	nlm_dbgsvc_cancargs(&mstr, arg);
}

static void
_nlm_dbgsvc_shareargs(struct mstr *mstrp, nlm_shareargs *arg)
{

}

void
nlm_dbgsvc_shareargs(nlm_shareargs *arg, const char *ldr)
{
	struct sstr sstr;
	struct mstr mstr;

	TAILQ_INIT(&mstr);
	sstr.s = ldr;
	TAILQ_INSERT_HEAD(&mstr, &sstr, link);
	nlm_dbgsvc_cancargs(&mstr, arg);
}

static void
_nlm_dbgsvc_shareres(struct mstr *mstrp, nlm_shareres *result)
{

}

void
nlm_dbgsvc_shareres(nlm_shareres *result, const char *ldr)
{
	struct sstr sstr;
	struct mstr mstr;

	TAILQ_INIT(&mstr);
	sstr.s = ldr;
	TAILQ_INSERT_HEAD(&mstr, &sstr, link);
	nlm_dbgsvc_cancargs(&mstr, arg);
}

static void
_nlm_dbgsvc_testargs(struct mstr *mstrp, nlm_testargs *arg)
{

}

void
nlm_dbgsvc_testargs(nlm_testargs *arg, const char *ldr)
{
	struct sstr sstr;
	struct mstr mstr;

	TAILQ_INIT(&mstr);
	sstr.s = ldr;
	TAILQ_INSERT_HEAD(&mstr, &sstr, link);
	nlm_dbgsvc_cancargs(&mstr, arg);
}

static void
_nlm_dbgsvc_testres(struct mstr *mstrp, nlm_testres *result)
{

}

void
nlm_dbgsvc_testres(nlm_testres *result, const char *ldr)
{
	struct sstr sstr;
	struct mstr mstr;

	TAILQ_INIT(&mstr);
	sstr.s = ldr;
	TAILQ_INSERT_HEAD(&mstr, &sstr, link);
	nlm_dbgsvc_cancargs(&mstr, arg);
}

static void
_nlm_dbgsvc_unlockargs(struct mstr *mstrp, nlm_unlockargs *arg)
{

}

void
nlm_dbgsvc_unlockargs(nlm_unlockargs *arg, const char *ldr)
{
	struct sstr sstr;
	struct mstr mstr;

	TAILQ_INIT(&mstr);
	sstr.s = ldr;
	TAILQ_INSERT_HEAD(&mstr, &sstr, link);
	nlm_dbgsvc_cancargs(&mstr, arg);
}
