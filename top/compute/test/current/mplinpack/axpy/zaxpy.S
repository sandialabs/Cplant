    /*        Fast AXPY routine for Alpha 21164(A)         */
    /*         on  Linux, Digital UNIX and NT              */
    /*                         date : 98.11.14             */
    /*        by Kazushige Goto <goto@statabo.rim.or.jp>   */

	.set noat
	.set noreorder

#ifdef DOUBLE
#define NAME	zaxpy_
#define SIZE	8
#define MUL	mult
#define ADD	addt
#define SUB	subt
#define LD	ldt
#define ST	stt
#define SXADDQ	s8addq
#define SXSUBL	s8subl
#else
#define NAME	caxpy_
#define SIZE	4
#define MUL	muls
#define ADD	adds
#define SUB	subs
#define LD	lds
#define ST	sts
#define SXADDQ	s4addq
#define SXSUBL	s4subl
#endif

	.globl	NAME
	.ent	NAME

NAME:
	lda	$30, -64($30)
	ldl	$16,   0($16)

	ldl	$19,   0($19)
	ldl	$21,   0($21)

	LD	$f29,  0*SIZE($17)	# alpha_r
	LD	$f30,  1*SIZE($17)	# alpha_i
	stt	$f2,   0($30)
	cmpeq	$19, 1, $1

	stt	$f3,   8($30)
	cmpeq	$21, 1, $2
	stt	$f4,  16($30)
	and	$16, 3, $5

	stt	$f5,  24($30)
	stt	$f6,  32($30)
	stt	$f7,  40($30)
	stt	$f8,  48($30)
	.prologue 0

	and	$1, $2, $1
	ble	$16, $End
	sra	$16, 2, $4
	beq	$1, $Sub

	ble	$4,  $Remain
	subq	$4,  1,  $4

	LD	$f0,  0*SIZE($18)
	LD	$f1,  1*SIZE($18)
	LD	$f2,  2*SIZE($18)
	LD	$f3,  3*SIZE($18)
	LD	$f4,  4*SIZE($18)
	LD	$f5,  5*SIZE($18)
	LD	$f6,  6*SIZE($18)
	LD	$f7,  7*SIZE($18)

	LD	$f8,  0*SIZE($20)
	LD	$f28, 1*SIZE($20)
	LD	$f10, 2*SIZE($20)
	LD	$f11, 3*SIZE($20)
	LD	$f12, 4*SIZE($20)
	LD	$f13, 5*SIZE($20)
	LD	$f14, 6*SIZE($20)
	LD	$f15, 7*SIZE($20)

	addq	$18, 8*SIZE, $18
	ble	$4, $MainLoopEnd
	.align 4

$MainLoop:
	MUL	$f29, $f0,  $f20
	LD	$f31, 9*SIZE($18)
	MUL	$f30, $f1,  $f21
	unop

	MUL	$f30, $f0,  $f22
	LD	$f0,  0*SIZE($18)
	MUL	$f29, $f1,  $f23
	LD	$f1,  1*SIZE($18)
	
	MUL	$f29, $f2,  $f24
	unop
	MUL	$f30, $f3,  $f25
	nop

	MUL	$f30, $f2,  $f26
	LD	$f2,  2*SIZE($18)
	MUL	$f29, $f3,  $f27
	LD	$f3,  3*SIZE($18)

	SUB	$f20, $f21, $f16
	MUL	$f29, $f4,  $f20
	ADD	$f22, $f23, $f17
	MUL	$f30, $f5,  $f21

	SUB	$f24, $f25, $f18
	unop
	MUL	$f30, $f4,  $f22
	LD	$f4,  4*SIZE($18)

	ADD	$f26, $f27, $f19
	addq	$20, 8*SIZE, $20
	MUL	$f29, $f5,  $f23
	LD	$f5,  5*SIZE($18)
	
	ADD	$f16, $f8,  $f16
	LD	$f8,  0*SIZE($20)
	MUL	$f29, $f6,  $f24
	unop

	ADD	$f17, $f28, $f17
	LD	$f28, 1*SIZE($20)
	MUL	$f30, $f7,  $f25
	unop

	ADD	$f18, $f10, $f18
	LD	$f10, 2*SIZE($20)
	MUL	$f30, $f6,  $f26
	LD	$f6,  6*SIZE($18)

	ADD	$f19, $f11, $f19
	LD	$f11, 3*SIZE($20)
	MUL	$f29, $f7,  $f27
	LD	$f7,  7*SIZE($18)

	ST	$f16,-8*SIZE($20)
	SUB	$f20, $f21, $f16
	ST	$f17,-7*SIZE($20)
	ADD	$f22, $f23, $f17

	ST	$f18,-6*SIZE($20)
	SUB	$f24, $f25, $f18
	ST	$f19,-5*SIZE($20)
	ADD	$f26, $f27, $f19
	
	ADD	$f16, $f12, $f16
	LD	$f12, 4*SIZE($20)
	ADD	$f17, $f13, $f17
	LD	$f13, 5*SIZE($20)
	ADD	$f18, $f14, $f18
	LD	$f14, 6*SIZE($20)
	ADD	$f19, $f15, $f19
	LD	$f15, 7*SIZE($20)

	ST	$f16,-4*SIZE($20)
	addq	$18, 8*SIZE, $18
	ST	$f17,-3*SIZE($20)
	subq	$4, 1, $4

	ST	$f18,-2*SIZE($20)
	nop
	ST	$f19,-1*SIZE($20)
	bgt	$4, $MainLoop
	.align 4

$MainLoopEnd:
	MUL	$f29, $f0,  $f20
	MUL	$f30, $f1,  $f21
	MUL	$f30, $f0,  $f22
	MUL	$f29, $f1,  $f23
	
	MUL	$f29, $f2,  $f24
	MUL	$f30, $f3,  $f25
	MUL	$f30, $f2,  $f26
	MUL	$f29, $f3,  $f27

	SUB	$f20, $f21, $f16
	MUL	$f29, $f4,  $f20
	ADD	$f22, $f23, $f17
	MUL	$f30, $f5,  $f21

	SUB	$f24, $f25, $f18
	MUL	$f30, $f4,  $f22
	ADD	$f26, $f27, $f19
	MUL	$f29, $f5,  $f23
	
	ADD	$f16, $f8,  $f16
	MUL	$f29, $f6,  $f24
	ADD	$f17, $f28, $f17
	MUL	$f30, $f7,  $f25

	ADD	$f18, $f10, $f18
	MUL	$f30, $f6,  $f26
	ADD	$f19, $f11, $f19
	MUL	$f29, $f7,  $f27

	ST	$f16, 0*SIZE($20)
	SUB	$f20, $f21, $f16
	ST	$f17, 1*SIZE($20)
	ADD	$f22, $f23, $f17

	ST	$f18, 2*SIZE($20)
	SUB	$f24, $f25, $f18
	ST	$f19, 3*SIZE($20)
	ADD	$f26, $f27, $f19
	
	ADD	$f16, $f12, $f16
	ADD	$f17, $f13, $f17
	ADD	$f18, $f14, $f18
	ADD	$f19, $f15, $f19

	ST	$f16, 4*SIZE($20)
	ST	$f17, 5*SIZE($20)
	ST	$f18, 6*SIZE($20)
	ST	$f19, 7*SIZE($20)

	unop
	addq	$20, 8*SIZE, $20
	unop
	ble	$5,  $End
	.align 4

$Remain:
	subq	$5,  1,  $6
	ble	$5,  $End
	LD	$f0,  0*SIZE($18)
	LD	$f1,  1*SIZE($18)

	LD	$f8,  0*SIZE($20)
	LD	$f28, 1*SIZE($20)
	addq	$18, 2*SIZE, $18
	ble	$6, $RemainLoopEnd
	.align 4

$RemainLoop:
	MUL	$f29, $f0,  $f20
	subq	$6, 1, $6
	MUL	$f30, $f1,  $f21
	addq	$20, 2*SIZE, $20

	MUL	$f30, $f0,  $f22
	LD	$f0,  0*SIZE($18)
	MUL	$f29, $f1,  $f23
	LD	$f1,  1*SIZE($18)
	
	SUB	$f20, $f21, $f16
	ADD	$f22, $f23, $f17
	ADD	$f16, $f8,  $f16
	LD	$f8,  0*SIZE($20)
	ADD	$f17, $f28, $f17
	LD	$f28, 1*SIZE($20)

	ST	$f16,-2*SIZE($20)
	addq	$18, 2*SIZE, $18
	ST	$f17,-1*SIZE($20)
	bgt	$6, $RemainLoop
	.align 4

$RemainLoopEnd:
	MUL	$f29, $f0,  $f20
	MUL	$f30, $f1,  $f21
	MUL	$f30, $f0,  $f22
	MUL	$f29, $f1,  $f23
	
	SUB	$f20, $f21, $f16
	ADD	$f22, $f23, $f17
	ADD	$f16, $f8,  $f16
	ADD	$f17, $f28, $f17

	ST	$f16, 0*SIZE($20)
	nop
	ST	$f17, 1*SIZE($20)
	nop
	.align 4

$End:
	ldt	$f2,   0($30)
	ldt	$f3,   8($30)
	ldt	$f4,  16($30)
	ldt	$f5,  24($30)
	ldt	$f6,  32($30)
	ldt	$f7,  40($30)
	ldt	$f8,  48($30)
	lda	$30,  64($30)
	ret
	.align 4

$Sub:
	SXSUBL	$16,  SIZE, $22
	addq	$22,  $22,  $22		# Complex 

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
	addq	$19, $19, $19		# Complex 
	addq	$21, $21, $21		# Complex 

	ble	$4, $SubRemain

	LD	$f0,  0*SIZE($18)
	LD	$f1,  1*SIZE($18)
	SXADDQ	$19, $18, $18

	LD	$f2,  0*SIZE($18)
	LD	$f3,  1*SIZE($18)
	SXADDQ	$19, $18, $18

	LD	$f4,  0*SIZE($18)
	LD	$f5,  1*SIZE($18)
	SXADDQ	$19, $18, $18

	LD	$f6,  0*SIZE($18)
	LD	$f7,  1*SIZE($18)
	SXADDQ	$19, $18, $18

	LD	$f8,  0*SIZE($20)
	LD	$f28, 1*SIZE($20)
	SXADDQ	$21, $20, $24

	LD	$f10, 0*SIZE($24)
	LD	$f11, 1*SIZE($24)
	SXADDQ	$21, $24, $24

	LD	$f12, 0*SIZE($24)
	LD	$f13, 1*SIZE($24)
	SXADDQ	$21, $24, $24

	LD	$f14, 0*SIZE($24)
	LD	$f15, 1*SIZE($24)
	SXADDQ	$21, $24, $24

	subq	$4,  1,  $4
	ble	$4, $SubMainLoopEnd
	.align 4

$SubMainLoop:
	MUL	$f29, $f0,  $f20
	unop
	MUL	$f30, $f1,  $f21
	unop

	MUL	$f30, $f0,  $f22
	LD	$f0,  0*SIZE($18)
	MUL	$f29, $f1,  $f23
	LD	$f1,  1*SIZE($18)

	MUL	$f29, $f2,  $f24
	SXADDQ	$19, $18, $18
	MUL	$f30, $f3,  $f25
	unop

	MUL	$f30, $f2,  $f26
	LD	$f2,  0*SIZE($18)
	MUL	$f29, $f3,  $f27
	LD	$f3,  1*SIZE($18)

	SUB	$f20, $f21, $f16
	SXADDQ	$19, $18, $18
	MUL	$f29, $f4,  $f20
	unop

	ADD	$f22, $f23, $f17
	unop
	MUL	$f30, $f5,  $f21
	unop

	SUB	$f24, $f25, $f18
	unop
	MUL	$f30, $f4,  $f22
	LD	$f4,  0*SIZE($18)

	ADD	$f26, $f27, $f19
	unop
	MUL	$f29, $f5,  $f23
	LD	$f5,  1*SIZE($18)
	
	ADD	$f16, $f8,  $f16
	LD	$f8,  0*SIZE($24)
	MUL	$f29, $f6,  $f24
	SXADDQ	$19, $18, $18

	ADD	$f17, $f28, $f17
	LD	$f28, 1*SIZE($24)
	MUL	$f30, $f7,  $f25
	SXADDQ	$21, $24, $24

	ADD	$f18, $f10, $f18
	LD	$f10, 0*SIZE($24)
	MUL	$f30, $f6,  $f26
	LD	$f6,  0*SIZE($18)

	ADD	$f19, $f11, $f19
	LD	$f11, 1*SIZE($24)
	MUL	$f29, $f7,  $f27
	LD	$f7,  1*SIZE($18)

	ST	$f16, 0*SIZE($20)
	SXADDQ	$19, $18, $18
 	SUB	$f20, $f21, $f16
	unop

	ST	$f17, 1*SIZE($20)
	SXADDQ	$21, $20, $20
	ADD	$f22, $f23, $f17
	unop

	ST	$f18, 0*SIZE($20)
	SXADDQ	$21, $24, $24
	SUB	$f24, $f25, $f18
	unop

	ST	$f19, 1*SIZE($20)
	unop
	ADD	$f26, $f27, $f19
	SXADDQ	$21, $20, $20

	ADD	$f16, $f12, $f16
	unop
	LD	$f12, 0*SIZE($24)
	unop

	ADD	$f17, $f13, $f17
	unop
	LD	$f13, 1*SIZE($24)
	SXADDQ	$21, $24, $24

	ADD	$f18, $f14, $f18
	subq	$4, 1, $4
	LD	$f14, 0*SIZE($24)
	unop

	ADD	$f19, $f15, $f19
	unop
	LD	$f15, 1*SIZE($24)
	SXADDQ	$21, $24, $24

	ST	$f16, 0*SIZE($20)
	ST	$f17, 1*SIZE($20)
	SXADDQ	$21, $20, $20
	unop

	ST	$f18, 0*SIZE($20)
	ST	$f19, 1*SIZE($20)
	SXADDQ	$21, $20, $20
	bgt	$4, $SubMainLoop
	.align 4

$SubMainLoopEnd:
	MUL	$f29, $f0,  $f20
	MUL	$f30, $f1,  $f21
	MUL	$f30, $f0,  $f22
	MUL	$f29, $f1,  $f23
	
	MUL	$f29, $f2,  $f24
	MUL	$f30, $f3,  $f25
	MUL	$f30, $f2,  $f26
	MUL	$f29, $f3,  $f27

	SUB	$f20, $f21, $f16
	MUL	$f29, $f4,  $f20
	ADD	$f22, $f23, $f17
	MUL	$f30, $f5,  $f21

	SUB	$f24, $f25, $f18
	MUL	$f30, $f4,  $f22
	ADD	$f26, $f27, $f19
	MUL	$f29, $f5,  $f23
	
	ADD	$f16, $f8,  $f16
	MUL	$f29, $f6,  $f24
	ADD	$f17, $f28, $f17
	MUL	$f30, $f7,  $f25

	ADD	$f18, $f10, $f18
	MUL	$f30, $f6,  $f26
	ADD	$f19, $f11, $f19
	MUL	$f29, $f7,  $f27

	ST	$f16, 0*SIZE($20)
	SUB	$f20, $f21, $f16
	ST	$f17, 1*SIZE($20)
	ADD	$f22, $f23, $f17

	SXADDQ	$21, $20, $20
	nop
	ST	$f18, 0*SIZE($20)
	SUB	$f24, $f25, $f18

	ST	$f19, 1*SIZE($20)
	ADD	$f26, $f27, $f19
	SXADDQ	$21, $20, $20
	ADD	$f16, $f12, $f16

	ADD	$f17, $f13, $f17
	ADD	$f18, $f14, $f18
	ADD	$f19, $f15, $f19

	ST	$f16, 0*SIZE($20)
	ST	$f17, 1*SIZE($20)
	SXADDQ	$21, $20, $20

	ST	$f18, 0*SIZE($20)
	ST	$f19, 1*SIZE($20)
	SXADDQ	$21, $20, $20
	ble	$5,  $SubEnd
	.align 4

$SubRemain:
	subq	$5,  1,  $6
	ble	$5,  $SubEnd
	LD	$f0,  0*SIZE($18)
	LD	$f1,  1*SIZE($18)

	LD	$f8,  0*SIZE($20)
	LD	$f28, 1*SIZE($20)
	SXADDQ	$19, $18, $18
	SXADDQ	$21, $20, $24
	ble	$6, $SubRemainLoopEnd
	.align 4

$SubRemainLoop:
	MUL	$f29, $f0,  $f20
	MUL	$f30, $f1,  $f21
	MUL	$f30, $f0,  $f22
	LD	$f0,  0*SIZE($18)

	MUL	$f29, $f1,  $f23
	LD	$f1,  1*SIZE($18)
	SUB	$f20, $f21, $f16
	SXADDQ	$19, $18, $18

	ADD	$f22, $f23, $f17
	nop
	ADD	$f16, $f8,  $f16
	LD	$f8,  0*SIZE($24)

	ADD	$f17, $f28, $f17
	LD	$f28, 1*SIZE($24)
	SXADDQ	$21, $24, $24
	subq	$6, 1, $6

	ST	$f16, 0*SIZE($20)
	ST	$f17, 1*SIZE($20)
	SXADDQ	$21, $20, $20
	bgt	$6, $SubRemainLoop
	.align 4

$SubRemainLoopEnd:
	MUL	$f29, $f0,  $f20
	MUL	$f30, $f1,  $f21
	MUL	$f30, $f0,  $f22
	MUL	$f29, $f1,  $f23
	
	SUB	$f20, $f21, $f16
	ADD	$f22, $f23, $f17
	ADD	$f16, $f8,  $f16
	ADD	$f17, $f28, $f17

	ST	$f16, 0*SIZE($20)
	nop
	ST	$f17, 1*SIZE($20)
	nop
	.align 4

$SubEnd:
	ldt	$f2,   0($30)
	ldt	$f3,   8($30)
	ldt	$f4,  16($30)
	ldt	$f5,  24($30)
	ldt	$f6,  32($30)
	ldt	$f7,  40($30)
	ldt	$f8,  48($30)
	lda	$30,  64($30)
	ret
	.end NAME

