#ifndef MPID_CHHETERO
#define MPID_CHHETERO

/* Msgrep is simply OK for Homogeneous systems */
#define MPID_CH_Comm_msgrep( comm ) ((comm)->msgform = MPID_MSG_OK,MPI_SUCCESS)
#define MPID_Msgrep_from_comm( comm ) MPID_MSGREP_RECEIVER

#endif
