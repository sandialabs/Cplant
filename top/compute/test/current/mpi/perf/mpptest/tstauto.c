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
 *  $Id: tstauto.c,v 1.1 1997/10/29 20:31:58 bright Exp $
 *
 *  (C) 1993 by Argonne National Laboratory and Mississippi State University.
 *
 */

#ifndef lint
static char vcid[] = "$Id: tstauto.c,v 1.1 1997/10/29 20:31:58 bright Exp $";
#endif

/*
   This file contains routines that help automate the collection of data.

   The first routine, TSTAuto1d, adaptively calls a user-provided routine
   for parameter values that are chosen to control an error measure based
   on a piecewise linear model.
 */

#include <math.h>

/*@
    TSTAuto1d - Generates data for piecewise linear model

    Input Parameters:
.   xmin,xmax - limits of domain
.   dxmin     - minimum delta x
.   dxmax     - maximum delta x
.   rtol      - relative error tolerance
.   atol      - absolute error tolerance
.   results   - pointer to user- structure (see fcn)
.   rsize     - user-defined size of a single result
.   rmax      - rmax*rsize = allocated size of result
.   fcn       - user-defined function, returns a (double) value used
                in estimating error.
.   fcnctx    - user-defined context passed to fcn.

    Returns:
    Number of function evaluations performed.
    
    Notes:
    fcn is defined as double fcn( x, result, fcnctx ).  fcnctx should contain
    any data needed to evaluate the fcn; result contains any data that the
    user wants saved.  The simplest form of result would contain the tuple
    (x,f(x)).  Note that the result data is not sorted by x.

    It would be nice to have an open-ended test that could be satisfied
    by a separate condition, such as reaching some asymptotic (e.g., linear)
    behavior.
@*/    
int TSTAuto1d( xmin, xmax, dxmin, dxmax, 
               rtol, atol, results, rsize, rmax, fcn, fcnctx )
double xmin, xmax, dxmin, dxmax, rtol, atol, (*fcn)();
int    rsize, rmax;
char   *results;
void   *fcnctx;   
{
double fl, fr, xr;
int    cnt, nval;

if (rmax < 1) return 0;
if (dxmax <= 0.0) dxmax = xmax - xmin;

/* Evaluate the function at the endpoints */
fl       = (*fcn)( xmin, results, fcnctx );
results	 += rsize;
rmax     --;
xr       = xmin;
cnt      = 1;
while (xr < xmax) {
    xr += dxmax;
    if (xr > xmax) xr = xmax;
    if (rmax < 1) return cnt;    

    fr       = (*fcn)( xr, results, fcnctx );
    results  += rsize;
    rmax     --;

    nval     = TSTiAuto1d( xmin, fl, xr, fr, dxmin,
		           rtol, atol, results, rsize, rmax, fcn, fcnctx );
    results  += nval * rsize;
    rmax     -= nval;
    cnt      += 1 + nval;
    fl       = fr;
    xmin     = xr;
    }
return cnt;    
}	

int TSTiAuto1d( xleft, fleft, xright, fright, dxmin,
		rtol, atol, results, rsize, rmax, fcn, fcnctx )
double xleft, fleft, xright, fright, dxmin, rtol, atol, (*fcn)();
int    rsize, rmax;
char   *results;
void   *fcnctx;   
{
double center, fcenter, fdp, h, fmax, ferrEst;
int    nvals, nvalsl;

if (rmax < 1) return 0;	
/* Compute error and test for recursive subdivision */

center   = 0.5 * (xleft + xright);
h        = xright - center;
if (h < dxmin) return 0;

fcenter  = (*fcn)( center, results, fcnctx );
results	 += rsize;
rmax     --;

/* Compute f'' estimate (at center) */
fdp = 2.0 * (fleft   / ((xleft - center) * (xleft - xright)) +
             fcenter / ((center - xleft) * (center - xright)) +
             fright  / ((xright - xleft) * (xright - center)) );
ferrEst = 0.5 * fabs( fdp ) * h * h;
fmax    = fabs( fleft );
if (fabs( fcenter ) > fmax) fmax = fabs( fcenter );
if (fabs( fright ) > fmax)  fmax = fabs( fright );
/*
printf( "Error est = %f, fmax=%f, (%f,%f,%f)\n", 
        ferrEst, fmax, fleft, fcenter, fright );
printf( "test value = %f (rtol = %f, atol = %f)\n", 
       fmax * rtol + atol, rtol, atol );
 */
if (ferrEst < fmax * rtol + atol)
     return 1;

/* Error estimate exceeded; adaptively refine */
nvalsl  = TSTiAuto1d( xleft, fleft, center, fcenter, dxmin, rtol, atol,
		       results, rsize, rmax, fcn, fcnctx );
results += nvalsl * rsize;
rmax    -= nvalsl;
nvals   = TSTiAuto1d( center, fcenter, xright, fright,  dxmin, rtol, atol,
		       results, rsize, rmax, fcn, fcnctx );
return nvals + nvalsl + 1;		       
}

static int rcompare( a, b )
double *a, *b;
{
if (*a < *b) return -1;
if (*a > *b) return 1;
return 0;
}	

/*@
    TSTRSort - Sort a user-defined result array

    Input Parameters:
.   results   - pointer to user- structure (see fcn)
.   rsize     - user-defined size of a single result
.   rcnt      - number of elements in array

    Notes:
    This assumes that the "x" value is the first value, and that it is
    stored as a double.
@*/
void TSTRSort( results, rsize, rcnt )
char *results;
int  rsize, rcnt;
{
qsort( results, rcnt, rsize, (int (*)())rcompare );
}	
