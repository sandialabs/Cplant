/*
 * $Id: enfs_prot.x,v 1.1 2000/02/15 23:34:42 lward Exp $
 */
/* Derived from @(#)nfs_prot.x	2.1 88/08/01 4.0 RPCSRC */

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

%#include "cmn.h"
%#include "rpcsvc/nfs_prot.h"

const ENFS_PORT          = 19049;
const ENFS_MAXDATA       = 65536;

/*
 * Arguments to remote read
 */
struct ereadargs {
	nfs_fh file;		/* handle for file */
	unsigned offset;	/* byte offset in file */
	unsigned count;		/* immediate read count */
	unsigned totalcount;	/* total read count (from this offset)*/
};

/*
 * Status OK portion of remote read reply
 */
struct ereadokres {
	fattr	attributes;	/* attributes, need for pagin*/
	opaque data<ENFS_MAXDATA>;
};

union ereadres switch (nfsstat status) {
case NFS_OK:
	ereadokres reply;
default:
	void;
};

/*
 * Arguments to remote write
 */
struct ewriteargs {
	nfs_fh	file;		/* handle for file */
	unsigned beginoffset;	/* beginning byte offset in file */
	unsigned offset;	/* current byte offset in file */
	unsigned totalcount;	/* total write count (to this offset)*/
	opaque data<ENFS_MAXDATA>;
};

/*
 * Remote file service routines
 *
 * Note:
 * 	The program and version numbers are as for NFS version 2. This should
 * 	not be so. However, some client implementations (Linux at least) can't
 *	play with alternates.
 */
program ENFS_PROGRAM {
	version ENFS_VERSION {
		void
		ENFSPROC_NULL(void) = 0;

		attrstat
		ENFSPROC_GETATTR(nfs_fh) =	1;

		attrstat
		ENFSPROC_SETATTR(sattrargs) = 2;

		void
		ENFSPROC_ROOT(void) = 3;

		diropres
		ENFSPROC_LOOKUP(diropargs) = 4;

		readlinkres
		ENFSPROC_READLINK(nfs_fh) = 5;

		ereadres
		ENFSPROC_READ(ereadargs) = 6;

		void
		ENFSPROC_WRITECACHE(void) = 7;

		attrstat
		ENFSPROC_WRITE(ewriteargs) = 8;

		diropres
		ENFSPROC_CREATE(createargs) = 9;

		nfsstat
		ENFSPROC_REMOVE(diropargs) = 10;

		nfsstat
		ENFSPROC_RENAME(renameargs) = 11;

		nfsstat
		ENFSPROC_LINK(linkargs) = 12;

		nfsstat
		ENFSPROC_SYMLINK(symlinkargs) = 13;

		diropres
		ENFSPROC_MKDIR(createargs) = 14;

		nfsstat
		ENFSPROC_RMDIR(diropargs) = 15;

		readdirres
		ENFSPROC_READDIR(readdirargs) = 16;

		statfsres
		ENFSPROC_STATFS(nfs_fh) = 17;

		nfsstat
		ENFSPROC_SYNCHRONIZE(nfs_fh) = 18;
	} = 2;
} = 100003;
