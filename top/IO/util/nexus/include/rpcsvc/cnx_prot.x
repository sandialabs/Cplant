/*
 * Server control protocol.
 *
 * $Id: cnx_prot.x,v 1.1 1999/11/29 19:43:53 lward Exp $
 *
 */
const CNX_MAXTYPNAMLEN		= 32;
const CNX_MAXPATHLEN		= 1024;
const CNX_OPAQARGSSIZ		= 2048;

enum cnx_status {
	CNX_OK = 0,
	CNXERR_PERM = 1,
	CNXERR_NOENT = 2,
	CNXERR_IO = 5,
	CNXERR_NXIO = 6,
	CNXERR_NOMEM = 12,
	CNXERR_ACCES = 13,
	CNXERR_NOTBLK = 15,
	CNXERR_BUSY = 16,
	CNXERR_NODEV = 19,
	CNXERR_NOTDIR = 20,
	CNXERR_INVAL = 22,
	CNXERR_MFILE = 24,
	CNXERR_NAMETOOLONG = 36
};

typedef string cnx_typenam<CNX_MAXTYPNAMLEN>;
typedef string cnx_path<CNX_MAXPATHLEN>;
typedef opaque cnx_opaqarg[CNX_OPAQARGSSIZ];
typedef unsigned cnx_svcid;

/*
 * Args to the perform mount request.
 */
struct cnx_mountarg {
	cnx_typenam type;
	cnx_path path;
	cnx_opaqarg arg;
};

/*
 * Args to the offer service request.
 */
struct cnx_offerarg {
	cnx_typenam type;
	cnx_opaqarg arg;
};

/*
 * Status with service ID result.
 */
union cnx_svcidres switch (cnx_status status) {
case CNX_OK:
	cnx_svcid id;
default:
	void;
};

program CNX_PROGRAM {
	version CNX_VERSION {
		cnx_status
		CNXPROC_MOUNT(cnx_mountarg)		= 1;

		cnx_status
		CNXPROC_UNMOUNT(cnx_path)		= 2;

		cnx_svcidres
		CNXPROC_OFFER(cnx_offerarg)		= 3;

		cnx_status
		CNXPROC_STOP(cnx_svcid)			= 4;
	} = 1;
} = 400050;
