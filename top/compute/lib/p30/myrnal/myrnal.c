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
** $Id: myrnal.c,v 1.31 2002/02/11 20:20:59 ktpedre Exp $
*/
#include <stdio.h>
#include <sys/param.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>		/* For mlock() munlock() */
#include <asm/page.h>		/* For PAGE_SIZE */
#include <asm/unistd.h>		/* For __NR_* */
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <p30.h>
#include <myrnal.h>
#include <sched.h>
#include <cache/cache.h>

extern int cache_fd;

#define P3DEV   "/dev/portals3"

#ifdef __osf__
#define ntv_sys	ntv_sys
#elif __linux__
#define ntv_sys	syscall
#else
#error "Need ntv_sys() system call\n"
#endif


static int forward(nal_t *nal, int id, void *args, size_t args_len, void *ret,
	size_t ret_len);
static int shutdown(nal_t *nal, int interface);
static int validate(nal_t *nal, void *base, size_t extent);
static void yield( nal_t *nal );
#ifdef __i386__
static void _touch_buf( void *ptr, ssize_t nbytes, int ro );
#endif


typedef struct {
    int p3fd;
} private_t;

private_t private= {-1};

static nal_t myrnal = {
    {0},	/* ni data */
    NULL,	/* nal_data */
    NULL,
    forward,
    shutdown,
    validate,
    NULL	/* yield is not defined for latency reasons */
};


nal_t *
PTL_IFACE_MYR(int interface, ptl_pt_index_t ptl_size, ptl_ac_index_t ac_size)
{

int p3fd;
int rc;
unsigned long args;
myrnal_forward_t mforward;

    if (__p30_myr_initialized == 1 ) {
	return &myrnal;
    }

    if (ac_size > MYRNAL_MAX_ACL_SIZE)   {
	fprintf(stderr, "ac_size %d > MYRNAL_MAX_ACL_SIZE %d\n", ac_size,
	    MYRNAL_MAX_ACL_SIZE);
	return NULL;
    }
    if (ptl_size > MYRNAL_MAX_PTL_SIZE)   {
	fprintf(stderr, "ptl_size %d > MYRNAL_MAX_PTL_SIZE %d\n", ptl_size,
	    MYRNAL_MAX_PTL_SIZE);
	return NULL;
    }
    if (private.p3fd >= 0)   {
	/* Device already open for this process */
	fprintf(stderr, "Device already open for this process!\n");
	return NULL;
    }
    myrnal.nal_data= (void *)&private;

    myrnal.timeout = &__p30_myr_timeout;

    p3fd= ntv_sys(__NR_open, P3DEV, O_RDWR);
    if (p3fd < 0)   {         
	perror("myrnal init: open()");
	return NULL;
    }

    ((private_t *)(myrnal.nal_data))->p3fd= p3fd;

    /* Set the close-on-exec flag */
    ntv_sys(__NR_fcntl, p3fd, F_SETFD, 1);

    /* Pack arguments */
    args= MCL_CURRENT | MCL_FUTURE;
    mforward.args= &args;
    mforward.args_len= sizeof(args);
    mforward.ret= NULL;
    mforward.ret_len= 0;
    mforward.p3cmd= PTL_MLOCKALL;

    rc= ntv_sys(__NR_ioctl, p3fd, P3SYSCALL, (unsigned long)(&mforward));
    if (rc)   {
	perror("Can't mlock all\n");
	return NULL;
    }
    __p30_myr_initialized=1;
    __myr_ni_handle =  (interface << 16);

    return &myrnal;

}

/*
**  This is where we actually cross the protection domain
**  from user code into the communication thread.  It would
**  also be the point where we would jump to the NIC or into
**  the kernel.
*/
static int
forward(nal_t *nal, int id, void *args, size_t args_len, void *ret,
	size_t ret_len)
{

ssize_t rc;
int p3fd;
myrnal_forward_t mforward;

    p3fd= ((private_t *)(nal->nal_data))->p3fd;
    if (p3fd < 0)   {
	return PTL_NOINIT;
    }

    /* Pack arguments */
    mforward.args= args;
    mforward.args_len= args_len;
    mforward.ret= ret;
    mforward.ret_len= ret_len;
    mforward.p3cmd= id;

    rc= ntv_sys(__NR_ioctl, p3fd, P3CMD, (unsigned long)(&mforward));

    return rc;

}


static int
shutdown(nal_t *nal, int interface)
{

int p3fd;


    p3fd= ((private_t *)(myrnal.nal_data))->p3fd;
    if (p3fd < 0)   {
	return -1;
    }
    close(p3fd);
    ((private_t *)(myrnal.nal_data))->p3fd= -1;
    return 0;
}


static int
validate(nal_t *nal, void *base, size_t extent)
{
#ifdef __i386__
    int rc;
#endif

    if (extent == 0)   {
	return 0;
    } else if ((base == NULL) || (extent < 0))   {
	return -1;
    }

#ifdef VERBOSE
    printf("validating %p of length %ld: %s\n", base, (long)extent,
	rc == 0 ? "OK" : "ERROR");
#endif VERBOSE

#ifdef __i386__
    /* rc = (1,0,-1) (RDONLY,WRITABLE,VMA_NOT_FOUND!) */
    rc = ntv_sys(__NR_ioctl, cache_fd, CACHE_USER_PAGE_IS_RDONLY, 
                                         (unsigned long)base);
    if (rc < 0) {
      /* vma not found */
      return -1;
    }

    if (rc == 0) {
      /* should be ok to write this buffer */
      _touch_buf(base, (ssize_t) extent, 0);
    }
#endif
    return 0;

}  /* end of validate() */


static void
yield(nal_t *nal)
{
#if defined( _POSIX_PRIORITY_SCHEDULING )
	sched_yield();
#else
	/* Do nothing */
#endif
}  /* end of yield() */


#ifdef __i386__
static void
_touch_buf( void *ptr, ssize_t nbytes, int ro )
{
        char *tp, *ep;
	char sum= 0, old_value;

	if( ptr == NULL || nbytes == 0 ) {
	  return;
        }

        tp = (char*) ptr;

	ep = (char*)( (unsigned long) ptr + nbytes );

	if( ro ) {
	  sum += *tp;
	} 
        else {
          old_value = *tp;
         *tp = old_value; 
        }

        /* first byte of next page */
	tp = (char*) ( ( (unsigned long)tp + PAGE_SIZE ) & PAGE_MASK );

	while( tp < ep ) {
          if( ro ) {
	    sum += *tp;
	  } 
          else {
	    old_value = *tp;
           *tp = old_value; 
          }
	  tp = (char*) ( (unsigned long)tp + PAGE_SIZE );
	}
	return;
}
#endif
