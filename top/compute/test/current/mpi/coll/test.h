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
/* Header for testing procedures */

#ifndef _INCLUDED_TEST_H_
#define _INCLUDED_TEST_H_

#ifdef __STDC__
void Test_Init(char *, int);
void Test_Printf(char *, ...);
void Test_Message(char *);
void Test_Failed(char *);
void Test_Passed(char *);
int Summarize_Test_Results(void);
void Test_Finalize(void);
void Test_Waitforall(void);
#else
void Test_Init();
void Test_Message();
void Test_Printf();
void Test_Failed();
void Test_Passed();
int Summarize_Test_Results();
void Test_Finalize();
void Test_Waitforall();
#endif

#endif
