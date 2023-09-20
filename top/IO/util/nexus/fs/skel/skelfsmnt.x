/*
 * Skeletal file system specific mount argument.
 *
 * $Id: skelfsmnt.x,v 1.1 2000/05/23 15:50:31 smorgan Exp $
 */

const SKELFSMNT_MAXPATHLEN = 1024;

typedef string skelfsmnt_conf<SKELFSMNT_MAXPATHLEN>;

struct skelfsmnt {
	int		mount_internal;	/* flag: mount internal skelfs */
	skelfsmnt_conf	cpath;		/* configuration file path */
};

