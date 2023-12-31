/* $Header: /Net/pecos/proj/puma01/Cplant/top/compute/lib/mpich-1.2.0/src/infoexport/mpi_interface.h,v 1.1 2000/02/18 03:25:09 rbbrigh Exp $ */
/* $Locker:  $ */

/**********************************************************************
 * Copyright (C) 1997-1998 Dolphin Interconnect Solutions Inc.
 *
 * Permission is hereby granted to use, reproduce, prepare derivative
 * works, and to redistribute to others.
 *
 *				  DISCLAIMER
 *
 * Neither Dolphin Interconnect Solutions, nor any of their employees,
 * makes any warranty express or implied, or assumes any legal
 * liability or responsibility for the accuracy, completeness, or
 * usefulness of any information, apparatus, product, or process
 * disclosed, or represents that its use would not infringe privately
 * owned rights.
 *
 * This code was written by
 * James Cownie: Dolphin Interconnect Solutions. <jcownie@dolphinics.com>
 **********************************************************************/


/* Update log
 *
 * Oct  1 1998 JHC: Change MQS_INVALID_PROCESS to -1, TV would never generate
 *              the old value anyway.
 * May 26 1998 JHC: Change interface compatiblity, add extra strings in the
 *              mqs_pending_operation.
 * Apr 23 1998 JHC: Make mqs_tword_t and mqs_taddr_t into 64 bit entities on
 *                  SGI. Expand the comment above their definition.
 * Mar  9 1998 JHC: Added mqs_sizeof_ft. Of course we always needed this !
 * Nov  6 1997 JHC: Worked over somewhat to keep John happy :-)
 * Oct 27 1997 James Cownie <jcownie@dolphinics.com>: Created.
 */

/***********************************************************************
 * This header file defines the interface between a debugger and a
 * dynamically loaded library use to implement access to MPI message
 * queues.
 *
 * The interface is specified at the C level, to avoid C++ compiler issues.
 *
 * The interface allows code in the DLL to 
 * 1) find named types from the debugger's type system and look up fields in them
 * 2) find the address of named external variables 
 * 3) access objects at absolute addresses in the target process.
 * 4) convert objects from target format to host format.
 *
 * A number of different objects are passed backwards and forwards
 * between the debugger and the DLL :-
 *
 * executable images
 * processes
 * communicators
 *
 * Many of these are opaque to the DLL, in such cases they will be
 * defined in the interface as pointers to opaque structures, since
 * this provides type checking while maintaining information hiding.
 *
 * All named entities in here start with the prefix "mqs_" (for
 * Message Queue Support), all the debugger callbacks are made via 
 * callback tables, so the real (linkage) names of the functions are
 * not visible to the DLL.
 */

#ifndef _MPI_INTERFACE_INCLUDED
#define _MPI_INTERFACE_INCLUDED

#ifdef	__cplusplus
extern "C" {
#endif

/***********************************************************************
 * Version of the interface this header represents 
 */
enum
{
  MQS_INTERFACE_COMPATIBILITY = 2
};

/***********************************************************************
 * Type definitions.
 */

/* Opaque types are used here to provide a degree of type checking
 * through the prototypes of the interface functions.
 *
 * Only pointers to these types are ever passed across the interface.
 * Internally to the debugger, or the DLL you should immediately cast
 * these pointers to pointers to the concrete types that you actually
 * want to use.
 *
 * (An alternative would be to use void * for all of these arguments,
 * but that would remove a useful degree of type checking, assuming
 * that you are thinking while typing the casts :-)
 */

/* Types which will be (cast to) concrete types in the DLL */
typedef struct _mqs_image_info   mqs_image_info;
typedef struct _mqs_process_info mqs_process_info;

/* Types which will be (cast to) concrete types in the debugger */
typedef struct mqs_image_    mqs_image;
typedef struct mqs_process_  mqs_process;
typedef struct mqs_type_     mqs_type;

/* *** BEWARE ***
 * On machines with two pointer lengths (such as SGI -n32, -64 compilations,
 * and AIX and Solaris soon), it is quite likely that TotalView and the DLL
 * will have been compiled with the 32 bit model, *but* will need to debug
 * code compiled with the 64 bit one. The mqs_taddr_t and mqs_tword_t need
 * to be a type which even when compiled with the 32 bit model compiler can
 * hold the longer (64 bit) pointer.
 *
 * You may need to add your host to this #if, if you have a machine with
 * two compilation models.
 * *** END BEWARE ***
 *
 * It would be better not to have this target dependence in the
 * interface, but it is unreasonable to require a 64 bit interface on
 * 32 bit machines, and we need the 64 bit interface in some places.
 */

#if defined (__sgi)
typedef unsigned long long mqs_taddr_t;		/* Something long enough for a target address */
typedef long long          mqs_tword_t;		/* Something long enough for a word    */
#else
typedef unsigned long mqs_taddr_t;		/* Something long enough for a target address */
typedef long          mqs_tword_t;		/* Something long enough for a word    */
#endif

/***********************************************************************
 * Defined structures which form part of the interface.
 */

/* A structure for (target) architectural information */
typedef struct
{
  int short_size;				/* sizeof (short) */
  int int_size;					/* sizeof (int)   */
  int long_size;				/* sizeof (long)  */
  int long_long_size;				/* sizeof (long long) */
  int pointer_size;				/* sizeof (void *) */
} mqs_target_type_sizes;
  
/* Result codes. 
 * mqs_ok is returned for success. 
 * Anything else implies a failure of some sort. 
 *
 * Most of the functions actually return one of these, however to avoid
 * any potential issues with different compilers implementing enums as
 * different sized objects, we actually use int as the result type.
 *
 * Note that both the DLL and the debugger will use values starting at
 * mqs_first_user_code, since you always know which side you were calling,
 * this shouldn't be a problem.
 *
 * See below for functions to convert codes to strings.
 */
enum {
  mqs_ok = 0,
  mqs_no_information,
  mqs_end_of_list,
  mqs_first_user_code = 100			/* Allow for more pre-defines */
};

typedef enum {
  mqs_lang_c     = 'c',
  mqs_lang_cplus = 'C',
  mqs_lang_f77   = 'f',
  mqs_lang_f90   = 'F'
} mqs_lang_code;

/* Which queue are we interested in ? */
typedef enum
{
  mqs_pending_sends, 
  mqs_pending_receives, 
  mqs_unexpected_messages
} mqs_op_class;

/* A value to represent an invalid process index. */
enum
{
  MQS_INVALID_PROCESS = -1
};

/* A structure to represent a communicator */
typedef struct
{
  mqs_taddr_t unique_id;			/* A unique tag for the communicator */
  mqs_tword_t local_rank;			/* The rank of this process Comm_rank */
  mqs_tword_t size;				/* Comm_size  */
  char    name[64];				/* the name if it has one */
} mqs_communicator;

/*
 * We currently assume that all messages are flattened into contiguous buffers.
 * This is potentially incorrect, but let's leave that complication for a while.
 */
enum mqs_status 
{
  mqs_st_pending, mqs_st_matched, mqs_st_complete
};

typedef struct
{
  /* Fields for all messages */
  int status;					/* Status of the message (really enum mqs_status) */
  mqs_tword_t desired_local_rank;		/* Rank of target/source -1 for ANY */
  mqs_tword_t desired_global_rank;		/* As above but in COMM_WORLD  */
  int tag_wild;					/* Flag for wildcard receive  */
  mqs_tword_t desired_tag;			/* Only if !tag_wild */
  mqs_tword_t desired_length;			/* Length of the message buffer */
  int system_buffer;				/* Is it a system or user buffer ? */
  mqs_taddr_t buffer;				/* Where data is */

  /* Fields valid if status >= matched or it's a send */
  mqs_tword_t actual_local_rank;		/* Actual local rank */
  mqs_tword_t actual_global_rank;		/* As above but in COMM_WORLD */
  mqs_tword_t actual_tag;				
  mqs_tword_t actual_length;
  
  /* Additional strings which can be filled in if the DLL has more
   * info.  (Uninterpreted by the debugger, simply displayed to the
   * user).  
   *
   * Can be used to give the name of the function causing this request,
   * for instance.
   *
   * Up to five lines each of 64 characters.
   */
  char extra_text[5][64];
} mqs_pending_operation;

/***********************************************************************
 * Callbacks from the DLL into the debugger.
 ***********************************************************************
 * These are all made via a table of function pointers.
 */

/* Hang information on the image */
typedef void (*mqs_put_image_info_ft) (mqs_image *, mqs_image_info *);
/* Get it back */
typedef mqs_image_info * (*mqs_get_image_info_ft) (mqs_image *);

/* Given a process return the image it is an instance of */
typedef mqs_image * (*mqs_get_image_ft) (mqs_process *);

/* Given a process return its rank in comm_world */
typedef int (*mqs_get_global_rank_ft) (mqs_process *);

/* Given an image look up the specified function */
typedef int (*mqs_find_function_ft) (mqs_image *, char *, mqs_lang_code, mqs_taddr_t * );

/* Given an image look up the specified symbol */
typedef int (*mqs_find_symbol_ft) (mqs_image *, char *, mqs_taddr_t * );

/* Hang information on the process */
typedef void (*mqs_put_process_info_ft) (mqs_process *, mqs_process_info *);
/* Get it back */
typedef mqs_process_info * (*mqs_get_process_info_ft) (mqs_process *);

/* Allocate store */
typedef void * (*mqs_malloc_ft) (size_t);
/* Free it again */
typedef void   (*mqs_free_ft)   (void *);

/***********************************************************************
 * Type access functions 
 */

/* Given an executable image look up a named type in it.  
 * Returns a type handle, or the null pointer if the type could not be
 * found.  Since the debugger may load debug information lazily, the
 * MPI run time library should ensure that the type definitions
 * required occur in a file whose debug information will already have
 * been loaded, for instance by placing them in the same file as the
 * startup breakpoint function.
 */
typedef mqs_type * (*mqs_find_type_ft)(mqs_image *, char *, mqs_lang_code);

/* Given the handle for a type (assumed to be a structure) return the
 * byte offset of the named field. If the field cannot be found 
 * the result will be -1.
 */
typedef int (*mqs_field_offset_ft) (mqs_type *, char *);

/* Given the handle for a type return the size of the type in bytes.
 * (Just like sizeof ())
 */
typedef int (*mqs_sizeof_ft) (mqs_type *);

/* Fill in the sizes of target types for this process */
typedef void (*mqs_get_type_sizes_ft) (mqs_process *, mqs_target_type_sizes *);

/***********************************************************************
 * Target store access functions
 */

/* Fetch data from the process into a buffer into a specified buffer.
 * N.B. 
 * The data is the same as that in the target process when accessed
 * as a byte array. You *must* use mqs_target_to_host to do any
 * necessary byte flipping if you want to look at it at larger
 * granularity.
 */
typedef int (*mqs_fetch_data_ft) (mqs_process *, mqs_taddr_t, int, void *);

/* Convert data into host format */
typedef void (*mqs_target_to_host_ft) (mqs_process *, const void *, void *, int);

/***********************************************************************
 * Miscellaneous functions.
 */
/* Print a message (intended for debugging use *ONLY*). */
typedef void (*mqs_dprints_ft) (const char *);
/* Convert an error code from the debugger into an error message */
typedef char * (*mqs_errorstring_ft) (int);

/***********************************************************************
 * Call back tables
 */
typedef struct mqs_basic_callbacks
{
  mqs_malloc_ft           mqs_malloc_fp;
  mqs_free_ft             mqs_free_fp;             
  mqs_dprints_ft          mqs_dprints_fp;
  mqs_errorstring_ft      mqs_errorstring_fp;
  mqs_put_image_info_ft   mqs_put_image_info_fp;
  mqs_get_image_info_ft	  mqs_get_image_info_fp;
  mqs_put_process_info_ft mqs_put_process_info_fp;
  mqs_get_process_info_ft mqs_get_process_info_fp;
} mqs_basic_callbacks;

typedef struct mqs_image_callbacks
{
  mqs_get_type_sizes_ft	  mqs_get_type_sizes_fp;
  mqs_find_function_ft	  mqs_find_function_fp;
  mqs_find_symbol_ft      mqs_find_symbol_fp;
  mqs_find_type_ft        mqs_find_type_fp;
  mqs_field_offset_ft	  mqs_field_offset_fp;
  mqs_sizeof_ft	          mqs_sizeof_fp;
} mqs_image_callbacks;

typedef struct mqs_process_callbacks
{
  mqs_get_global_rank_ft  mqs_get_global_rank_fp;
  mqs_get_image_ft        mqs_get_image_fp;
  mqs_fetch_data_ft	  mqs_fetch_data_fp;
  mqs_target_to_host_ft   mqs_target_to_host_fp;
} mqs_process_callbacks;

/***********************************************************************
 * Calls from the debugger into the DLL.
 ***********************************************************************/

/* Provide the library with the pointers to the the debugger functions
 * it needs The DLL need only save the pointer, the debugger promises
 * to maintain the table of functions valid for as long as
 * needed. (The table remains the property of the debugger, and should
 * not be messed with, or deallocated by the DLL). This applies to
 * all of the callback tables.
 */
extern void mqs_setup_basic_callbacks (const mqs_basic_callbacks *);

/* Version handling */
extern char *mqs_version_string ( void );
extern int   mqs_version_compatibility( void );

/* Provide a text string for an error value */
extern char * mqs_dll_error_string (int);

/***********************************************************************
 * Calls related to an executable image.
 */

/* Setup debug information for a specific image, this must save
 * the callbacks (probably in the mqs_image_info), and use those
 * functions for accessing this image.
 *
 * The DLL should use the mqs_put_image_info and mqs_get_image_info functions
 * to associate whatever information it wants to keep with the image.
 * (For instance all of the type offsets it needs could be kept here).
 * the debugger will call mqs_destroy_image_info when it no longer wants to
 * keep information about the given executable.
 *
 * This will be called once for each executable image in the parallel
 * program.
 */
extern int mqs_setup_image (mqs_image *, const mqs_image_callbacks *);

/* Does this image have the necessary symbols to allow access to the message
 * queues ?
 *
 * This function will be called once for each image, and the information
 * cached inside the debugger.
 *
 * Returns an error enumeration to show whether the image has queues
 * or not, and an error string to be used in a pop-up complaint to the
 * user, as if in printf (error_string, name_of_image);
 *
 * The pop-up display is independent of the result. (So you can silently
 * disable things, or loudly enable them).
 */

extern int mqs_image_has_queues (mqs_image *, char **);

/* This will be called by the debugger to let you tidy up whatever is
 * required when the mqs_image_info is no longer needed.
 */
extern void mqs_destroy_image_info (mqs_image_info *);

/***********************************************************************
 * Calls related to a specific process. These will only be called if the 
 * image which this is an instance of passes the has_message_queues tests. 
 *
 * If you can't tell whether the process will have valid message queues
 * just by looking at the image, then you should return mqs_ok from 
 * mqs_image_has_queues and let mqs_process_has_queues handle it.
 */

/* Set up whatever process specific information we need. 
 * For instance addresses of global variables should be handled here,
 * rather than in the image information if anything is a dynamic library
 * which could end up mapped differently in different processes.
 */
extern int mqs_setup_process (mqs_process *, const mqs_process_callbacks *);
extern void mqs_destroy_process_info (mqs_process_info *);

/* Like the mqs_has_message_queues function, but will only be called
 * if the image claims to have message queues. This lets you actually
 * delve inside the process to look at variables before deciding if
 * the process really can support message queue extraction.
 */  
extern int mqs_process_has_queues (mqs_process *, char **);

/***********************************************************************
 * The functions which actually extract the information we need !
 *
 * The model here is that the debugger calls down to the library to initialise
 * an iteration over a specific class of things, and then keeps calling
 * the "next" function until it returns mqs_false. 
 *
 * For communicators we separate stepping from extracting information,
 * because we want to use the state of the communicator iterator to qualify
 * the selections of the operation iterator.
 *
 * Whenever mqs_true is returned the description has been updated,
 * mqs_false means there is no more information to return, and
 * therefore the description contains no useful information.
 *
 * We will only have one of each type of iteration running at once, so
 * the library should save the iteration state in the
 * mqs_process_info.
 */


/* Check that the DLL's model of the communicators in the process is
 * up to date, ideally by checking the sequence number.
 */
extern int mqs_update_communicator_list (mqs_process *);

/* Prepare to iterate over all of the communicators in the process. */
extern int mqs_setup_communicator_iterator (mqs_process *);

/* Extract information about the current communicator */
extern int mqs_get_communicator (mqs_process *, mqs_communicator *);

/* Move on to the next communicator in this process. */
extern int mqs_next_communicator (mqs_process *);

/* Prepare to iterate over the pending operations in the currently
 * active communicator in this process.
 *
 * The int is *really* mqs_op_class
 */
extern int mqs_setup_operation_iterator (mqs_process *, int);

/* Return information about the next appropriate pending operation in
 * the current communicator, mqs_false when we've seen them all.
 */
extern int mqs_next_operation (mqs_process *, mqs_pending_operation *);

#ifdef	__cplusplus
}
#endif
#endif /* defined (_MPI_INTERFACE_INCLUDED) */


