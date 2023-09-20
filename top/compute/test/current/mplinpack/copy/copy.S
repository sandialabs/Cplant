    /*        Fast COPY routine for Alpha 21164(A)         */
    /*         on  Linux, Digital UNIX and NT              */
    /*                         date : 98.11.15             */
    /*        by Kazushige Goto <goto@statabo.rim.or.jp>   */

	.set noat
	.set noreorder

#ifdef DCOPY
#define NAME	dcopy_
#define DOUBLE
#endif

#ifdef SCOPY
#define NAME	scopy_
#undef  DOUBLE
#endif

#ifdef ZCOPY
#define NAME	zcopy_
#define DOUBLE
#endif

#ifdef CCOPY
#define NAME	ccopy_
#define DOUBLE
#endif

#ifdef DOUBLE
#define SIZE	8
#define LD	ldt
#define ST	stt
#define SXADDQ	s8addq
#define SXSUBL	s8subl
#else
#define SIZE	4
#define LD	lds
#define ST	sts
#define SXADDQ	s4addq
#define SXSUBL	s4subl
#endif

	.globl	NAME
	.ent	NAME
NAME:
	.prologue 0

	ldl	$16,   0($16)
	ldl	$18,   0($18)
	ldl	$20,   0($20)
	nop

	cmpeq	$18,  1, $0
	ble	$16, $End
#ifndef ZCOPY
	sra	$16,  4, $4
#else
	sra	$16,  3, $4
#endif
	cmpeq	$20,  1, $1

	and	$0,  $1, $0
	beq	$0, $Sub
#ifndef ZCOPY
	and	$16, 15, $5
#else
	and	$16,  7, $5
#endif
	ble	$4,  $Remain

	LD	$f10,  0*SIZE($17)
	LD	$f11,  1*SIZE($17)
	LD	$f12,  2*SIZE($17)
	LD	$f13,  3*SIZE($17)
	LD	$f14,  4*SIZE($17)
	LD	$f15,  5*SIZE($17)
	LD	$f16,  6*SIZE($17)
	LD	$f17,  7*SIZE($17)

	LD	$f18,  8*SIZE($17)
	LD	$f19,  9*SIZE($17)
	LD	$f20, 10*SIZE($17)
	LD	$f21, 11*SIZE($17)
	LD	$f22, 12*SIZE($17)
	LD	$f23, 13*SIZE($17)
	LD	$f24, 14*SIZE($17)
	LD	$f25, 15*SIZE($17)

	subq	$4, 1, $4
	lda	$17, 16*SIZE($17)
	ble	$4, $MainLoopEnd
	.align 4

$MainLoop:
	ST	$f10,  0*SIZE($19)
	ST	$f11,  1*SIZE($19)
	ST	$f12,  2*SIZE($19)
	ST	$f13,  3*SIZE($19)

	LD	$f10,  0*SIZE($17)
	LD	$f11,  1*SIZE($17)
	LD	$f12,  2*SIZE($17)
	LD	$f13,  3*SIZE($17)

	ST	$f14,  4*SIZE($19)
	ST	$f15,  5*SIZE($19)
	ST	$f16,  6*SIZE($19)
	ST	$f17,  7*SIZE($19)

	LD	$f14,  4*SIZE($17)
	LD	$f15,  5*SIZE($17)
	LD	$f16,  6*SIZE($17)
	LD	$f17,  7*SIZE($17)

	ST	$f18,  8*SIZE($19)
	ST	$f19,  9*SIZE($19)
	ST	$f20, 10*SIZE($19)
	ST	$f21, 11*SIZE($19)

	LD	$f18,  8*SIZE($17)
	LD	$f19,  9*SIZE($17)
	LD	$f20, 10*SIZE($17)
	LD	$f21, 11*SIZE($17)

	ST	$f22, 12*SIZE($19)
	ST	$f23, 13*SIZE($19)
	ST	$f24, 14*SIZE($19)
	ST	$f25, 15*SIZE($19)

	LD	$f22, 12*SIZE($17)
	LD	$f23, 13*SIZE($17)
	LD	$f24, 14*SIZE($17)
	LD	$f25, 15*SIZE($17)

	subq	$4, 1, $4
	lda	$19, 16*SIZE($19)
	lda	$17, 16*SIZE($17)
	bgt	$4, $MainLoop
	.align 4

$MainLoopEnd:
	ST	$f10,  0*SIZE($19)
	ST	$f11,  1*SIZE($19)
	ST	$f12,  2*SIZE($19)
	ST	$f13,  3*SIZE($19)
	ST	$f14,  4*SIZE($19)
	ST	$f15,  5*SIZE($19)
	ST	$f16,  6*SIZE($19)
	ST	$f17,  7*SIZE($19)

	ST	$f18,  8*SIZE($19)
	ST	$f19,  9*SIZE($19)
	ST	$f20, 10*SIZE($19)
	ST	$f21, 11*SIZE($19)
	ST	$f22, 12*SIZE($19)
	ST	$f23, 13*SIZE($19)
	ST	$f24, 14*SIZE($19)
	ST	$f25, 15*SIZE($19)

	lda	$19, 16*SIZE($19)
	.align 4

$Remain:
	ble	$5, $End
#ifdef ZCOPY
	addq	$5, $5, $5
#endif
	.align 4

$RemainLoop:
	LD	$f10,  0*SIZE($17)
	lda	$17,   1*SIZE($17)
	ST	$f10,  0*SIZE($19)
	lda	$19,   1*SIZE($19)
	subq	$5, 1, $5
	bgt	$5, $RemainLoop
	.align 4
$End:
	ret
	.align 4

$Sub:
	SXSUBL	$16,  SIZE, $22
#ifdef ZCOPY
	addq	$22,  $22, $22
#endif
	bge	$18, $IncX
	mull	$22, $18, $23
	subq	$17, $23, $17
	.align 4

$IncX:
	bge	$20, $IncY
	mull	$22, $20, $23
	subq	$19, $23, $19
	.align 4
$IncY:
#ifdef ZCOPY
	addq	$18, $18, $18
	addq	$20, $20, $20
	and	$16,  7, $5
#else
	and	$16, 15, $5
#endif
	ble	$4, $SubRemain
	.align 4

$SubMainLoop:
#ifndef ZCOPY
	LD	$f10,  0($17)
	SXADDQ	$18, $17, $17
	LD	$f11,  0($17)
	SXADDQ	$18, $17, $17

	LD	$f12,  0($17)
	SXADDQ	$18, $17, $17
	LD	$f13,  0($17)
	SXADDQ	$18, $17, $17

	LD	$f14,  0($17)
	SXADDQ	$18, $17, $17
	LD	$f15,  0($17)
	SXADDQ	$18, $17, $17

	LD	$f16,  0($17)
	SXADDQ	$18, $17, $17
	LD	$f17,  0($17)
	SXADDQ	$18, $17, $17

	LD	$f18,  0($17)
	SXADDQ	$18, $17, $17
	LD	$f19,  0($17)
	SXADDQ	$18, $17, $17

	LD	$f20,  0($17)
	SXADDQ	$18, $17, $17
	LD	$f21,  0($17)
	SXADDQ	$18, $17, $17

	LD	$f22,  0($17)
	SXADDQ	$18, $17, $17
	LD	$f23,  0($17)
	SXADDQ	$18, $17, $17

	LD	$f24,  0($17)
	SXADDQ	$18, $17, $17
	LD	$f25,  0($17)
	SXADDQ	$18, $17, $17

	ST	$f10,  0($19)
	SXADDQ	$20, $19, $19
	ST	$f11,  0($19)
	SXADDQ	$20, $19, $19

	ST	$f12,  0($19)
	SXADDQ	$20, $19, $19
	ST	$f13,  0($19)
	SXADDQ	$20, $19, $19

	ST	$f14,  0($19)
	SXADDQ	$20, $19, $19
	ST	$f15,  0($19)
	SXADDQ	$20, $19, $19

	ST	$f16,  0($19)
	SXADDQ	$20, $19, $19
	ST	$f17,  0($19)
	SXADDQ	$20, $19, $19

	ST	$f18,  0($19)
	SXADDQ	$20, $19, $19
	ST	$f19,  0($19)
	SXADDQ	$20, $19, $19

	ST	$f20,  0($19)
	SXADDQ	$20, $19, $19
	ST	$f21,  0($19)
	SXADDQ	$20, $19, $19

	ST	$f22,  0($19)
	SXADDQ	$20, $19, $19
	ST	$f23,  0($19)
	SXADDQ	$20, $19, $19

	ST	$f24,  0($19)
	SXADDQ	$20, $19, $19
	ST	$f25,  0($19)
	SXADDQ	$20, $19, $19
#else
	LD	$f10,  0($17)
	LD	$f11,  8($17)
	SXADDQ	$18, $17, $17

	LD	$f12,  0($17)
	LD	$f13,  8($17)
	SXADDQ	$18, $17, $17

	LD	$f14,  0($17)
	LD	$f15,  8($17)
	SXADDQ	$18, $17, $17

	LD	$f16,  0($17)
	LD	$f17,  8($17)
	SXADDQ	$18, $17, $17

	LD	$f18,  0($17)
	LD	$f19,  8($17)
	SXADDQ	$18, $17, $17

	LD	$f20,  0($17)
	LD	$f21,  8($17)
	SXADDQ	$18, $17, $17

	LD	$f22,  0($17)
	LD	$f23,  8($17)
	SXADDQ	$18, $17, $17

	LD	$f24,  0($17)
	LD	$f25,  8($17)
	SXADDQ	$18, $17, $17

	ST	$f10,  0($19)
	ST	$f11,  8($19)
	SXADDQ	$20, $19, $19

	ST	$f12,  0($19)
	ST	$f13,  8($19)
	SXADDQ	$20, $19, $19

	ST	$f14,  0($19)
	ST	$f15,  8($19)
	SXADDQ	$20, $19, $19

	ST	$f16,  0($19)
	ST	$f17,  8($19)
	SXADDQ	$20, $19, $19

	ST	$f18,  0($19)
	ST	$f19,  8($19)
	SXADDQ	$20, $19, $19

	ST	$f20,  0($19)
	ST	$f21,  8($19)
	SXADDQ	$20, $19, $19

	ST	$f22,  0($19)
	ST	$f23,  8($19)
	SXADDQ	$20, $19, $19

	ST	$f24,  0($19)
	ST	$f25,  8($19)
	SXADDQ	$20, $19, $19
#endif
	subq	$4, 1, $4
	bgt	$4, $SubMainLoop
	.align 4

$SubRemain:
	ble	$5, $SubEnd
	.align 4

 $SubRemainLoop:
#ifndef ZCOPY
	LD	$f10,  0($17)
	SXADDQ	$18, $17, $17
	ST	$f10,  0($19)
	SXADDQ	$20, $19, $19
#else
	LD	$f10,  0($17)
	LD	$f11,  8($17)
	SXADDQ	$18, $17, $17
	ST	$f10,  0($19)
	ST	$f11,  8($19)
	SXADDQ	$20, $19, $19
#endif
	subq	$5, 1, $5
	bgt	$5, $SubRemainLoop
	.align 4

$SubEnd:
	ret
	.end NAME
