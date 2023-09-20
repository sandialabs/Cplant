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
 *  $Id: rate.c,v 1.1 1997/10/29 20:31:56 bright Exp $
 *
 *  (C) 1993 by Argonne National Laboratory and Mississippi State University.
 *
 */

#ifndef lint
static char vcid[] = "$Id: rate.c,v 1.1 1997/10/29 20:31:56 bright Exp $";
#endif

/*
    This file contains very simple code for estimating the parameters in the
    simple (s + rn) model of communication.  It does this using the NORMAL
    equations for the least-squares problem; this method has a number of
    disadvantages including numerical instability when the number of
    observations is large or the normal equation matrix nearly singular.
    Another problem is that the least squares problem gives different answers
    depending on how the system is weighted.  For example, another weighting
    (different than the one used here) weights the rows of the matrix by the
    inverse of the right-hand-side.

    The advantage of this approach is that it needs only a few, easily (if
    subject to numerical errors during accumulation) acquired values
 */    

/*@
    PIComputeRate - Computes the communication rate given timing information
 
    Input Parameters:
.   sumlen - Sum of the lengths of the messages sent
.   sumtime - Sum of the time to send the messages
.   sumlentime - Sum of the product of the message lengths and the times
                 to send those messages
.   sumlen2 - Sum of the squares of the lengths of the messages
.   ntest   - Number of messages sent

    Output Parameters:
.   s - latency
.   r - transfer rate

    Notes:
    This code computes a fit to the model (s + r n) for communications
    between two processors.  The method used is reasonable reliable for
    small values of ntest.

    If there is insufficient data to compute s and r, both are set to
    zero.  This code does not check that the assumed model (s + r n) is
    a good choice.

    The length sums are doubles rather than ints to provide a "long long"
    type (think of them as 53 bit integers).
@*/
void PIComputeRate( sumlen, sumtime, sumlentime, sumlen2, ntest, s, r )
int    ntest;
double sumlen, sumlen2, sumtime, sumlentime;
double *s, *r;
{
double R, S;

R = sumlen * sumlen - ntest * sumlen2;
if (R == 0.0) {
    *s = *r = 0.0;
    return;
    }
R = (sumlen * sumtime - ntest * sumlentime) / R;
S = (sumtime - R * sumlen) / ntest;

if (S < 0 || R < 0) {
    S = 0.0;
    R = sumlentime / sumlen2;
    }
*r = R;
*s = S;
}
