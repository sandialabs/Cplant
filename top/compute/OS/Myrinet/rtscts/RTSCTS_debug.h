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
** $Id: RTSCTS_debug.h,v 1.3 2002/02/26 23:15:52 jbogden Exp $
** Debugging trace of protocol messages
*/

#ifndef RTSCTS_DEBUG_H
#define RTSCTS_DEBUG_H

#include "RTSCTS_protocol.h"	/* For pkt_type_t */

#ifndef NO_PROTO_DEBUG
  #define protocol_debug_add      protocol_debug_add_func
#else
  #define protocol_debug_add
#endif /* NO_STATS */

    void protocol_debug_init(void);
    void protocol_debug_add_func(unsigned int msgID, pkt_type_t event,
	    unsigned int nid, unsigned int info, unsigned int info2, int sent);
    void protocol_debug_proc(char **pb_ptr, char *pb_end);

#endif /* RTSCTS_DEBUG_H */
