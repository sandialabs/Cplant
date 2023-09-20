    /*        Fast AXPY routine for Alpha 21164(A)         */
    /*         on  Linux, Digital UNIX and NT              */
    /*                         date : 98.11.05             */
    /*        by Kazushige Goto <goto@statabo.rim.or.jp>   */

	.set noat
	.set noreorder

#ifdef DOUBLE
#define NAME	daxpy_
#define SIZE	8
#define MUL	mult
#define ADD	addt
#define LD	ldt
#define ST	stt
#define SXADDQ	s8addq
#define SXSUBL	s8subl
#else
#define NAME	saxpy_
#define SIZE	4
#define MUL	muls
#define ADD	adds
#define LD	lds
#define ST	sts
#define SXADDQ	s4addq
#define SXSUBL	s4subl
#endif

	.globl NAME
	.ent   NAME

NAME:
	lda	$30, -16($30)
	ldl	$16,   0($16)		# n
	LD	$f30,  0($17)		# da
	ldl	$19,   0($19)		# incx

	ldl	$21,   0($21)		# incy
	sra	$16,  3,  $1
	stt	$f2,   0($30)
	cmpeq	$19,  1,  $3

	stt	$f3,   8($30)
	cmpeq	$21,  1, $4
	and	$16,  7,  $2
	ble	$16, $End

	and	$3,  $4,  $3
	fbeq	$f30, $End

	beq	$3,  $Sub
	ble	$1,  $Remain
	.align 4

	LD	$f10,  0*SIZE($18)
	LD	$f11,  1*SIZE($18)
	LD	$f12,  2*SIZE($18)
	LD	$f13,  3*SIZE($18)

	LD	$f18,  0*SIZE($20)
	LD	$f19,  1*SIZE($20)
	LD	$f20,  2*SIZE($20)
	LD	$f21,  3*SIZE($20)

	LD	$f14,  4*SIZE($18)
	LD	$f15,  5*SIZE($18)
	LD	$f16,  6*SIZE($18)
	LD	$f17,  7*SIZE($18)

	LD	$f22,  4*SIZE($20)
	LD	$f23,  5*SIZE($20)
	LD	$f24,  6*SIZE($20)
	LD	$f25,  7*SIZE($20)

	subq	$1,   1,  $1
	addq	$18, 8*SIZE, $18
	unop
	ble	$1,  $LoopEnd
	.align 4

$Loop:
	MUL	$f30, $f10, $f26		# ctemp1 = da * atemp1
	LD	$f10,  0*SIZE($18)
	MUL	$f30, $f11, $f27
	LD	$f11,  1*SIZE($18)

	MUL	$f30, $f12, $f28
	LD	$f12,  2*SIZE($18)
	MUL	$f30, $f13, $f29
	LD	$f13,  3*SIZE($18)

	ADD	$f18, $f26, $f0
	LD	$f18,  8*SIZE($20)
	MUL	$f30, $f14, $f26		# ctemp1 = da * atemp1
	LD	$f14,  4*SIZE($18)

	ADD	$f19, $f27, $f1
	LD	$f19,  9*SIZE($20)
	MUL	$f30, $f15, $f27
	LD	$f15,  5*SIZE($18)

	ADD	$f20, $f28, $f2
	LD	$f20, 10*SIZE($20)
	MUL	$f30, $f16, $f28
	LD	$f16,  6*SIZE($18)

	ADD	$f21, $f29, $f3
	LD	$f21, 11*SIZE($20)
	MUL	$f30, $f17, $f29
	LD	$f17, 7*SIZE($18)

	ST	$f0,   0*SIZE($20)
	ADD	$f22, $f26, $f0
	ST	$f1,   1*SIZE($20)
	ADD	$f23, $f27, $f1

	ST	$f2,   2*SIZE($20)
	ADD	$f24, $f28, $f2
	ST	$f3,   3*SIZE($20)
	ADD	$f25, $f29, $f3

	LD	$f22, 12*SIZE($20)
	LD	$f23, 13*SIZE($20)
	LD	$f24, 14*SIZE($20)
	LD	$f25, 15*SIZE($20)

	ST	$f0,  4*SIZE($20)
	ST	$f1,  5*SIZE($20)
	ST	$f2,  6*SIZE($20)
	ST	$f3,  7*SIZE($20)

	subq	$1,  1, $1
	addq	$20, 8*SIZE, $20
	addq	$18, 8*SIZE, $18
	bgt	$1, $Loop
	.align 4

$LoopEnd:
	MUL	$f30, $f10, $f26		# ctemp1 = da * atemp1
	MUL	$f30, $f11, $f27
	MUL	$f30, $f12, $f28
	MUL	$f30, $f13, $f29

	ADD	$f18, $f26, $f0
	MUL	$f30, $f14, $f26		# ctemp1 = da * atemp1
	ADD	$f19, $f27, $f1
	MUL	$f30, $f15, $f27

	ADD	$f20, $f28, $f2
	MUL	$f30, $f16, $f28
	ADD	$f21, $f29, $f3
	MUL	$f30, $f17, $f29

	ST	$f0,   0*SIZE($20)
	ADD	$f22, $f26, $f0
	ST	$f1,   1*SIZE($20)
	ADD	$f23, $f27, $f1

	ST	$f2,   2*SIZE($20)
	ADD	$f24, $f28, $f2
	ST	$f3,   3*SIZE($20)
	ADD	$f25, $f29, $f3

	ST	$f0,   4*SIZE($20)
	ST	$f1,   5*SIZE($20)
	ST	$f2,   6*SIZE($20)
	ST	$f3,   7*SIZE($20)
	addq	$20, 8*SIZE, $20
	.align 4

$Remain:
	ble	$2, $End
	.align 4

$RemainLoop:
	LD	$f10,  0*SIZE($18)
	LD	$f11,  0*SIZE($20)
	addq	$18, SIZE, $18
	addq	$20, SIZE, $20

	MUL	$f30, $f10, $f12
	subq	$2,  1,  $2
	ADD	$f11, $f12, $f13
	ST	$f13,  -1*SIZE($20)
	bgt	$2,  $RemainLoop
	.align 4

$End:
	ldt	$f2,   0($30)
	ldt	$f3,   8($30)
	lda	$30,  16($30)
	ret
	.align 4

$Sub:
	SXSUBL	$16,  SIZE, $22
	bge	$19, $IncX
	mull	$22, $19, $23
	subq	$18, $23, $18
	.align 4

$IncX:
	bge	$21, $IncY
	mull	$22, $21, $23
	subq	$20, $23, $20
	.align 4

$IncY:
	subq	$1,  1, $4
	ble	$1, $SubRemain

	LD	$f10,  0($18)
	SXADDQ	$19, $18, $18

	LD	$f11,  0($18)
	SXADDQ	$19, $18, $18
	LD	$f12,  0($18)
	SXADDQ	$19, $18, $18

	LD	$f13,  0($18)
	SXADDQ	$19, $18, $18
	LD	$f18,  0($20)
	SXADDQ	$21, $20, $22

	LD	$f19,  0($22)
	SXADDQ	$21, $22, $22
	LD	$f20,  0($22)
	SXADDQ	$21, $22, $22

	LD	$f21,  0($22)
	SXADDQ	$21, $22, $22
	LD	$f14,  0($18)
	SXADDQ	$19, $18, $18

	LD	$f15,  0($18)
	SXADDQ	$19, $18, $18
	LD	$f16,  0($18)
	SXADDQ	$19, $18, $18

	LD	$f17,  0($18)
	SXADDQ	$19, $18, $18
	LD	$f22,  0($22)
	SXADDQ	$21, $22, $22

	LD	$f23,  0($22)
	SXADDQ	$21, $22, $22
	LD	$f24,  0($22)
	SXADDQ	$21, $22, $22

	LD	$f25,  0($22)
	SXADDQ	$21, $22, $22
	unop
	ble	$4,  $SubLoopEnd
	.align 4

$SubLoop:
	MUL	$f30, $f10, $f26		# ctemp1 = da * atemp1
	LD	$f10,  0($18)
	unop
	SXADDQ	$19, $18, $18

	MUL	$f30, $f11, $f27
	LD	$f11,  0($18)
	unop
	SXADDQ	$19, $18, $18

	MUL	$f30, $f12, $f28
	LD	$f12,  0($18)
	unop
	SXADDQ	$19, $18, $18

	MUL	$f30, $f13, $f29
	LD	$f13,  0($18)
	unop
	SXADDQ	$19, $18, $18

	ADD	$f18, $f26, $f0
	MUL	$f30, $f14, $f26		# ctemp1 = da * atemp1
	LD	$f14,  0($18)
	SXADDQ	$19, $18, $18

	ADD	$f19, $f27, $f1
	MUL	$f30, $f15, $f27
	LD	$f15,  0($18)
	SXADDQ	$19, $18, $18

	ADD	$f20, $f28, $f2
	MUL	$f30, $f16, $f28
	LD	$f16,  0($18)
	SXADDQ	$19, $18, $18

	ADD	$f21, $f29, $f3
	MUL	$f30, $f17, $f29
	LD	$f17,  0($18)
	SXADDQ	$19, $18, $18

	ST	$f0,   0($20)
	SXADDQ	$21, $20, $20
	ADD	$f22, $f26, $f0
	unop

	ST	$f1,   0($20)
	SXADDQ	$21, $20, $20
	ADD	$f23, $f27, $f1
	unop

	ST	$f2,   0($20)
	SXADDQ	$21, $20, $20
	ADD	$f24, $f28, $f2
	unop

	ST	$f3,   0($20)
	SXADDQ	$21, $20, $20
	ADD	$f25, $f29, $f3
	unop

	LD	$f18,  0($22)
	SXADDQ	$21, $22, $22
	LD	$f19,  0($22)
	SXADDQ	$21, $22, $22

	LD	$f20,  0($22)
	SXADDQ	$21, $22, $22
	LD	$f21,  0($22)
	SXADDQ	$21, $22, $22

	LD	$f22,  0($22)
	SXADDQ	$21, $22, $22
	LD	$f23,  0($22)
	SXADDQ	$21, $22, $22

	LD	$f24,  0($22)
	SXADDQ	$21, $22, $22
	LD	$f25,  0($22)
	SXADDQ	$21, $22, $22

	ST	$f0,  0($20)
	SXADDQ	$21, $20, $20
	ST	$f1,  0($20)
	SXADDQ	$21, $20, $20
	ST	$f2,  0($20)
	SXADDQ	$21, $20, $20
	ST	$f3,  0($20)
	SXADDQ	$21, $20, $20

	subq	$4,   1,  $4
	bgt	$4, $SubLoop
	.align 4

$SubLoopEnd:
	MUL	$f30, $f10, $f26		# ctemp1 = da * atemp1
	MUL	$f30, $f11, $f27
	MUL	$f30, $f12, $f28
	MUL	$f30, $f13, $f29

	ADD	$f18, $f26, $f0
	MUL	$f30, $f14, $f26		# ctemp1 = da * atemp1
	ADD	$f19, $f27, $f1
	MUL	$f30, $f15, $f27

	ADD	$f20, $f28, $f2
	MUL	$f30, $f16, $f28
	ADD	$f21, $f29, $f3
	MUL	$f30, $f17, $f29

	ST	$f0,   0($20)
	SXADDQ	$21, $20, $20
	ST	$f1,   0($20)
	SXADDQ	$21, $20, $20

	ST	$f2,   0($20)
	SXADDQ	$21, $20, $20
	ST	$f3,   0($20)
	SXADDQ	$21, $20, $20

	ADD	$f22, $f26, $f0
	ADD	$f23, $f27, $f1
	ADD	$f24, $f28, $f2
	ADD	$f25, $f29, $f3

	ST	$f0,   0($20)
	SXADDQ	$21, $20, $20
	ST	$f1,   0($20)
	SXADDQ	$21, $20, $20

	ST	$f2,   0($20)
	SXADDQ	$21, $20, $20
	ST	$f3,   0($20)
	SXADDQ	$21, $20, $20
	.align 4

$SubRemain:
	ble	$2, $SubEnd
	.align 4

$SubRemainLoop:
	LD	$f10,  0($18)
	LD	$f11,  0($20)
	SXADDQ	$19, $18, $18

	MUL	$f30, $f10, $f12
	subq	$2,  1,  $2
	ADD	$f11, $f12, $f13
	ST	$f13,  0($20)
	SXADDQ	$21, $20, $20

	bgt	$2,  $SubRemainLoop
	.align 4

$SubEnd:
	ldt	$f2,   0($30)
	ldt	$f3,   8($30)
	lda	$30,  16($30)
	ret
	.end NAME


