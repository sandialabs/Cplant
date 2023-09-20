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
** $Id: nal.h,v 1.7 2001/02/05 01:55:35 lafisk Exp $
*/
#ifndef _NAL_H_
#define _NAL_H_

/*
 * p30/nal.h
 *
 * The API side NAL declarations
 */

#include <p30/types.h>

typedef struct nal_t nal_t;

struct nal_t {
	ptl_ni_t	ni;
	void		*nal_data;
	int             *timeout;   /* for libp30api users */
	int		(*forward)(
		nal_t	*nal,
		int	index,	/* Function ID */
		void	*args,
		size_t	arg_len,
		void	*ret,
		size_t	ret_len
	);

	int		(*shutdown)(
			nal_t		*nal,
			int		interface
	);

	int		(*validate)(
			nal_t		*nal,
			void 		*base,
			size_t		extent
	);

	void		(*yield)(
			nal_t		*nal
	);
};
	
typedef nal_t *(ptl_interface_t)( int, ptl_pt_index_t, ptl_ac_index_t );
extern nal_t *PTL_IFACE_IP( int, ptl_pt_index_t, ptl_ac_index_t );
extern nal_t *PTL_IFACE_MYR( int, ptl_pt_index_t, ptl_ac_index_t );

#ifndef PTL_IFACE_DEFAULT
#define PTL_IFACE_DEFAULT (PTL_IFACE_IP)
#endif

#endif
