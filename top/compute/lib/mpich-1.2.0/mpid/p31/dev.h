/*
 * This file defines the device structures.  There are several layers:
 *
 * The protocol: this indicates how a device sends and receives messages using
 * one particular approach
 *
 * The device: this indicates how a device choses particular protocols
 * 
 * The device_set: this is the container for all devices, and indicates
 * how to choose which device to use
 *
 */

#ifndef _MPID_DEV
#define _MPID_DEV

#ifndef ANSI_ARGS
#if defined(__STDC__) || defined(__cplusplus) || defined(HAVE_PROTOTYPES)
#define ANSI_ARGS(a) a
#else
#define ANSI_ARGS(a) ()
#endif
#endif

typedef struct _MPID_Protocol MPID_Protocol;
struct _MPID_Protocol { 
    int (*send)        ANSI_ARGS((void *, int, int, int, int, int, 
				  MPID_Msgrep_t, struct MPIR_COMMUNICATOR * ));
    int (*recv)        ANSI_ARGS((MPIR_RHANDLE *, int, void *));
    int (*isend)       ANSI_ARGS((void *, int, int, int, int, int,
				  MPID_Msgrep_t, MPIR_SHANDLE *, 
				  struct MPIR_COMMUNICATOR *));
    int (*ssend)       ANSI_ARGS((void *, int, int, int, int, int, int,
				  struct MPIR_COMMUNICATOR * ));
    int (*issend)      ANSI_ARGS((void *, int, int, int, int, int, int,
				  MPIR_SHANDLE *, struct MPIR_COMMUNICATOR * ));
    int (*wait_send)   ANSI_ARGS((MPIR_SHANDLE *));
    int (*push_send)   ANSI_ARGS((MPIR_SHANDLE *));
    int (*cancel_send) ANSI_ARGS((MPIR_SHANDLE *));
    int (*irecv)       ANSI_ARGS((MPIR_RHANDLE *, int, void *));
    int (*wait_recv)   ANSI_ARGS((MPIR_RHANDLE *, MPI_Status *));
    int (*push_recv)   ANSI_ARGS((MPIR_RHANDLE *));
    int (*cancel_recv) ANSI_ARGS((MPIR_RHANDLE *));
    int (*unex)        ANSI_ARGS((MPIR_RHANDLE *, int, void *));
    int (*do_ack)      ANSI_ARGS((void *, int));
    void (*delete)     ANSI_ARGS((MPID_Protocol *));
    };

/* 
 * The information on the data formats could be stored with each device,
 * but in some cases (particularly while receiving a packet) we may not want
 * to determine the device first.  In any event, this data is organized by
 * global rank of partner, and is stored separately.  
 * It could be in the MPID_DevSet, but I'm leaving it separate, at least for
 * the moment
 */

/* 
 * This is a particular form of device that allows for three protocol breaks 
 */
typedef struct _MPID_Device MPID_Device;
struct _MPID_Device {
    int           long_len, vlong_len;
    MPID_Protocol *short_msg, *long_msg, *vlong_msg;
    /* MPID_Protocol *eager, *rndv; */
    MPID_Protocol *ready;
    /* Mapping from global ranks to devices relative rank.  May be 
       null to use grank directly */
    int           *grank_to_devlrank;
    /* Routines to receive header (check/wait header) */
    int           (*check_device) ANSI_ARGS((MPID_Device*, 
					     MPID_BLOCKING_TYPE));
    /* Run down and abort - do these need self (device)? */
    int           (*terminate)        ANSI_ARGS((MPID_Device *));
    int           (*abort)            ANSI_ARGS((struct MPIR_COMMUNICATOR *, 
						 int, char *));

    /* This next field is used to link together all of the devices */
    struct _MPID_Device *next;
    };

/* This is the container for ALL devices */
typedef struct {
    /* mapping from global ranks to devices.  Many entries in this array will 
       point to the same device */
    int         ndev;
    MPID_Device *dev; /* make this **dev for more than 1 device */
    
    /* List of all DIFFERENT devices.  */
    int         ndev_list;
    MPID_Device *dev_list;

    /* These are freed but not completed requests.  We check them from 
       time to time.  This is here because it is global state related 
       to device processing.
     */
    MPI_Request req_pending;
    } MPID_DevSet;

/* This is the structure that is used to specifiy the configuration for
   multi-device systems.
 */
typedef struct _MPID_Config MPID_Config;
struct _MPID_Config {
    /* The routine to initialize the device */
    MPID_Device *(*device_init) ANSI_ARGS(( int *, char ***, int, int ));
    /* The name of the routine if device_init is null for dynamic loading */
    char *device_init_name; 
    /* Number of partners served by this device */
    int  num_served;
    /* Array of global ranks served by this device */
    int  *granks_served;
    /* Next device (Null if this is last) */
    MPID_Config *next;
    };

#endif
