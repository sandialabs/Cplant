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
** $Id: gen_host.h,v 1.2 2001/11/30 00:05:39 pumatst Exp $
*/

#ifndef GET_HOST_H
#define GET_HOST_H


/*     Siberia                                                                */
void gen_host_name_siberia(char *label, int sw, int port);
void gen_switch_name_siberia(char *label, int sw);

/*     Iceberg                                                                */
void gen_switch_name_iceberg(char *label, int sw);
void gen_host_name_iceberg(char *label, int pnid);

/*     Iceberg2                                                               */
void gen_switch_name_iceberg2(char *label, int sw);
void gen_host_name_iceberg2(char *label, int pnid);

/*     Alaska                                                                 */
void gen_host_name_alaska(char *label, int pnid);


#endif /* GET_HOST_H */
