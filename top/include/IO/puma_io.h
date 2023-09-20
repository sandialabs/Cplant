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
** $Id: puma_io.h,v 1.2 1998/04/08 20:15:52 bright Exp $
*/
#ifndef PUMA_IO_H
#define PUMA_IO_H

/*
** IO modes inherited from Puma/nx
**
** These were defined in nx.h under Puma/Paragon.
*/
#define M_UNIX   0      /* Unshared file pointer, (default).
                         */
#define M_LOG    1      /* Shared file pointer, accessed in first come
                         * first served order.
                         */

/*
 * These may be AIX only values and should possibly be removed
 * (From Puma/paragon fcntl.h)
 */
 
/* Flag values accessible only to open(2) */
#define FOPEN           (-1)
#define FREAD           00001
#define FWRITE          00002
 
#define FMARK           00020           /* mark during gc() */
#define FSHLOCK         00200           /* shared lock present */
#define FEXLOCK         00400           /* exclusive lock present */


#endif
