#ifndef MPID_Wtime

#ifndef ANSI_ARGS
#if defined(__STDC__) || defined(__cplusplus)
#define ANSI_ARGS(a) a
#else
#define ANSI_ARGS(a) ()
#endif
#endif

#define MPID_Wtime(t) *(t)= dclock()
#define MPID_Wtick(t) MPID_P30_Wtick( t )

extern double dclock ANSI_ARGS((void));
extern void MPID_P30_Wtick ANSI_ARGS((double *));
#endif
