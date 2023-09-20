/*
 * NFS FS-specific mount argument.
 *
 * $Id: nfsmnt.x,v 1.3 1999/12/20 18:17:48 lward Exp $
 */

const NFSMNT_MAXHOSTLEN	= 256;
const NFSMNT_MAXPATHLEN	= 1024;

typedef string nfsmnt_host<NFSMNT_MAXHOSTLEN>;
typedef string nfsmnt_path<NFSMNT_MAXPATHLEN>;

/*
 * NFS specific VFS mount arguments.
 */
struct nfsmnt {
	nfsmnt_host rhost;				/* remote host */
	nfsmnt_path rpath;				/* remote path */
	unsigned int attrtimeo;				/* attrs timeo (secs) */
	unsigned int rpctimeo;				/* call timeo (secs) */
};
