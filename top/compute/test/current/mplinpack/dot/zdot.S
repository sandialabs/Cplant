    /*        Fast DOT routine for Alpha 21164(A)          */
    /*         on  Linux, Digital UNIX( and NT)            */
    /*                         date : 98.10.25             */
    /*        by Kazushige Goto <goto@statabo.rim.or.jp>   */

	.set noat
	.set noreorder

#ifdef ZDOT

#ifndef CONJ
#define NAME zdotu_
#else
#define NAME zdotc_
#endif

#define SIZE	8
#define SXADDQ	s8addq
#define SXSUBL	s8subl
#define LD	ldt
#define ST	stt
#define ADD	addt
#define SUB	subt
#define MUL	mult

#else

#ifndef CONJ
#define NAME cdotu_
#else
#define NAME cdotc_
#endif

#define SIZE    4
#define SXADDQ  s4addq
#define SXSUBL	s4subl
#define LD      lds
#define ST	sts
#define ADD	adds
#define SUB	subs
#define MUL	muls
#endif

	.globl NAME
	.ent   NAME

NAME:
	lda	$30,-16($30)
	fclr	$f0
	ldl	$17,  0($17)
	fclr	$f1

	ldl	$19,  0($19)
	fclr	$f26
	ldl	$21,  0($21)
	fclr	$f30

	stt	$f2,  0($30)
	fclr	$f27
	cmpeq	$19, 1, $22
	fclr	$f2

	cmpeq	$21, 1, $23
	fclr	$f28
	srl	$17, 2, $5
	fclr	$f29

	fnop
	and	$17, 3, $6
	fnop
	ble	$17, $End

	and	$22, $23, $22
	beq	$22, $Continue
	beq	$5,  $Remain
	subq	$5,   1, $5

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

	addq	$18, 8*SIZE, $18
	addq	$20, 8*SIZE, $20
	nop
	beq	$5, $MainLoopEnd
	.align 4

$MainLoop:
	ADD	$f0,  $f26, $f0
	MUL	$f10, $f18, $f26
	ADD	$f1,  $f27, $f1
	MUL	$f11, $f19, $f27

	ADD	$f2,  $f28, $f2
	unop
	MUL	$f10, $f19, $f28
	LD	$f10,  0*SIZE($18)

	ADD	$f30, $f29, $f30
	MUL	$f11, $f18, $f29
	LD	$f11,  1*SIZE($18)
	LD	$f18,  0*SIZE($20)

	ADD	$f0,  $f26, $f0
	LD	$f19,  1*SIZE($20)
	MUL	$f12, $f20, $f26
	unop

	ADD	$f1,  $f27, $f1
	LD	$f31, 120($18)
	MUL	$f13, $f21, $f27
	unop

	ADD	$f2,  $f28, $f2
	unop
	MUL	$f12, $f21, $f28
	LD	$f12, 2*SIZE($18)

	ADD	$f30, $f29, $f30
	MUL	$f13, $f20, $f29
	LD	$f13, 3*SIZE($18)
	LD	$f20, 2*SIZE($20)

	ADD	$f0,  $f26, $f0
	LD	$f21, 3*SIZE($20)
	MUL	$f14, $f22, $f26
	unop

	ADD	$f1,  $f27, $f1
	unop
	MUL	$f15, $f23, $f27
	unop

	ADD	$f2,  $f28, $f2
	subq	$5,   1, $5
	MUL	$f14, $f23, $f28
	LD	$f14, 4*SIZE($18)

	ADD	$f30, $f29, $f30
	LD	$f23, 5*SIZE($20)
	MUL	$f15, $f22, $f29
	LD	$f22, 4*SIZE($20)

	ADD	$f0,  $f26, $f0
	LD	$f15, 5*SIZE($18)
	MUL	$f16, $f24, $f26
	addq	$18, 8*SIZE, $18

	ADD	$f1,  $f27, $f1
	unop
	MUL	$f17, $f25, $f27
	unop

	ADD	$f2,  $f28, $f2
	addq	$20, 8*SIZE, $20
	MUL	$f16, $f25, $f28
	LD	$f16,-2*SIZE($18)

	ADD	$f30, $f29, $f30
	MUL	$f17, $f24, $f29
	LD	$f17,-1*SIZE($18)
	LD	$f24,-2*SIZE($20)

	LD	$f25,-1*SIZE($20)
	bgt	$5, $MainLoop
	.align 4

$MainLoopEnd:
	ADD	$f0,  $f26, $f0
	MUL	$f10, $f18, $f26
	ADD	$f1,  $f27, $f1
	MUL	$f11, $f19, $f27

	ADD	$f2,  $f28, $f2
	MUL	$f10, $f19, $f28
	ADD	$f30, $f29, $f30
	MUL	$f11, $f18, $f29

	ADD	$f0,  $f26, $f0
	MUL	$f12, $f20, $f26
	ADD	$f1,  $f27, $f1
	MUL	$f13, $f21, $f27

	ADD	$f2,  $f28, $f2
	MUL	$f12, $f21, $f28
	ADD	$f30, $f29, $f30
	MUL	$f13, $f20, $f29

	ADD	$f0,  $f26, $f0
	MUL	$f14, $f22, $f26
	ADD	$f1,  $f27, $f1
	MUL	$f15, $f23, $f27

	ADD	$f2,  $f28, $f2
	MUL	$f14, $f23, $f28
	ADD	$f30, $f29, $f30
	MUL	$f15, $f22, $f29

	ADD	$f0,  $f26, $f0
	MUL	$f16, $f24, $f26
	ADD	$f1,  $f27, $f1
	MUL	$f17, $f25, $f27

	ADD	$f2,  $f28, $f2
	MUL	$f16, $f25, $f28
	ADD	$f30, $f29, $f30
	MUL	$f17, $f24, $f29

	ADD	$f0,  $f26, $f0
	ADD	$f1,  $f27, $f1
	ADD	$f2,  $f28, $f2
	ADD	$f30, $f29, $f30
	.align 4

$Remain:
	addq	$18, 2*SIZE, $18
	beq	$6, $End
	addq	$20, 2*SIZE, $20
	subq	$6,   1, $6

	LD	$f10, -2*SIZE($18)
	fclr	$f26
	LD	$f18, -2*SIZE($20)
	fclr	$f27

	LD	$f11, -1*SIZE($18)
	fclr	$f28
	LD	$f19, -1*SIZE($20)
	fclr	$f29

	beq	$6, $RemainLoopEnd
	.align 4

$RemainLoop:
	ADD	$f0,  $f26, $f0
	addq	$18, 2*SIZE, $18
	MUL	$f10, $f18, $f26
	addq	$20, 2*SIZE, $20

	ADD	$f1,  $f27, $f1
	subq	$6,   1, $6
	MUL	$f11, $f19, $f27
	unop

	ADD	$f2,  $f28, $f2
	MUL	$f10, $f19, $f28
	LD	$f10, -2*SIZE($18)
	LD	$f19, -1*SIZE($20)

	ADD	$f30, $f29, $f30
	MUL	$f11, $f18, $f29
	LD	$f11, -1*SIZE($18)
	LD	$f18, -2*SIZE($20)

	bgt	$6, $RemainLoop
	.align 4

$RemainLoopEnd:
	ADD	$f0,  $f26, $f0
	MUL	$f10, $f18, $f26
	ADD	$f1,  $f27, $f1
	MUL	$f11, $f19, $f27

	ADD	$f2,  $f28, $f2
	MUL	$f10, $f19, $f28

	ADD	$f30, $f29, $f30
	MUL	$f11, $f18, $f29

	ADD	$f0,  $f26, $f0
	ADD	$f1,  $f27, $f1
	ADD	$f2,  $f28, $f2
	ADD	$f30, $f29, $f30
	br	$End
	.align 4

$Continue:
	or	$19, $21, $22
	bne	$22, $Continue2
	stq	$17, 8($30)
	nop

	LD	$f10, 0*SIZE($18)
	LD	$f12, 0*SIZE($20)
	LD	$f11, 1*SIZE($18)
	LD	$f13, 1*SIZE($20)

	ldt	$f14, 8($30)
	MUL	$f10, $f12, $f0
	MUL	$f11, $f13, $f1
	cvtqt	$f14, $f14

	MUL	$f10, $f13, $f2
	MUL	$f11, $f12, $f30
	MUL	$f0 , $f14, $f0
	MUL	$f1 , $f14, $f1

	MUL	$f2 , $f14, $f2
	MUL	$f30, $f14, $f30 
	br	$End
	.align 4

$Continue2:
	addl	$19, $19, $19
	addl	$21, $21, $21
	SXSUBL	$17, SIZE, $22
	bge	$19, $IncX

	mull	$22, $19, $23
	subq	$18, $23, $18
	.align 4

$IncX:
	bge	$21, $IncY
	mull	$22, $21, $24
	subq	$20, $24, $20
	.align 4

$IncY:
	fclr	$f26
	beq	$5, $SubRemain
	LD	$f10,  0*SIZE($18)
	fclr	$f27

	LD	$f11,  1*SIZE($18)
	fclr	$f28
	SXADDQ	$19, $18, $18
	fclr	$f29

	LD	$f18,  0*SIZE($20)
	LD	$f19,  1*SIZE($20)
	unop
	SXADDQ	$21, $20, $20

	LD	$f12,  0*SIZE($18)
	LD	$f13,  1*SIZE($18)
	unop
	SXADDQ	$19, $18, $18

	LD	$f20,  0*SIZE($20)
	LD	$f21,  1*SIZE($20)
	unop
	SXADDQ	$21, $20, $20

	LD	$f14,  0*SIZE($18)
	LD	$f15,  1*SIZE($18)
	unop
	SXADDQ	$19, $18, $18

	LD	$f22,  0*SIZE($20)
	LD	$f23,  1*SIZE($20)
	subq	$5,   1, $5
	SXADDQ	$21, $20, $20

	LD	$f16,  0*SIZE($18)
	LD	$f17,  1*SIZE($18)
	unop
	SXADDQ	$19, $18, $18

	LD	$f24,  0*SIZE($20)
	LD	$f25,  1*SIZE($20)
	SXADDQ	$21, $20, $20
	beq	$5, $SubMainLoopEnd

	.align 4

$SubMainLoop:
	ADD	$f0,  $f26, $f0
	MUL	$f10, $f18, $f26
	ADD	$f1,  $f27, $f1
	MUL	$f11, $f19, $f27

	ADD	$f2,  $f28, $f2
	unop
	MUL	$f10, $f19, $f28
	LD	$f10,  0*SIZE($18)

	ADD	$f30, $f29, $f30
	MUL	$f11, $f18, $f29
	LD	$f11,  1*SIZE($18)
	LD	$f18,  0*SIZE($20)

	ADD	$f0,  $f26, $f0
	LD	$f19,  1*SIZE($20)
	MUL	$f12, $f20, $f26
	SXADDQ	$19, $18, $18

	ADD	$f1,  $f27, $f1
	SXADDQ	$21, $20, $20
	MUL	$f13, $f21, $f27
	unop

	ADD	$f2,  $f28, $f2
	unop
	MUL	$f12, $f21, $f28
	LD	$f12,  0*SIZE($18)

	ADD	$f30, $f29, $f30
	MUL	$f13, $f20, $f29
	LD	$f13,  1*SIZE($18)
	LD	$f20,  0*SIZE($20)

	ADD	$f0,  $f26, $f0
	LD	$f21,  1*SIZE($20)
	MUL	$f14, $f22, $f26
	SXADDQ	$19, $18, $18

	ADD	$f1,  $f27, $f1
	SXADDQ	$21, $20, $20
	MUL	$f15, $f23, $f27
	unop

	ADD	$f2,  $f28, $f2
	subq	$5,   1, $5
	MUL	$f14, $f23, $f28
	LD	$f14,  0*SIZE($18)

	ADD	$f30, $f29, $f30
	MUL	$f15, $f22, $f29
	LD	$f15,  1*SIZE($18)
	LD	$f22,  0*SIZE($20)

	ADD	$f0,  $f26, $f0
	LD	$f23,  1*SIZE($20)
	MUL	$f16, $f24, $f26
	SXADDQ	$19, $18, $18

	ADD	$f1,  $f27, $f1
	SXADDQ	$21, $20, $20
	MUL	$f17, $f25, $f27
	unop

	ADD	$f2,  $f28, $f2
	MUL	$f16, $f25, $f28
	LD	$f16,  0*SIZE($18)
	LD	$f25,  1*SIZE($20)

	ADD	$f30, $f29, $f30
	MUL	$f17, $f24, $f29
	LD	$f17,  1*SIZE($18)
	LD	$f24,  0*SIZE($20)

	SXADDQ	$19, $18, $18
	SXADDQ	$21, $20, $20
	nop
	bgt	$5, $SubMainLoop
	.align 4

$SubMainLoopEnd:
	ADD	$f0,  $f26, $f0
	MUL	$f10, $f18, $f26
	ADD	$f1,  $f27, $f1
	MUL	$f11, $f19, $f27

	ADD	$f2,  $f28, $f2
	MUL	$f10, $f19, $f28
	ADD	$f30, $f29, $f30
	MUL	$f11, $f18, $f29

	ADD	$f0,  $f26, $f0
	MUL	$f12, $f20, $f26
	ADD	$f1,  $f27, $f1
	MUL	$f13, $f21, $f27

	ADD	$f2,  $f28, $f2
	MUL	$f12, $f21, $f28
	ADD	$f30, $f29, $f30
	MUL	$f13, $f20, $f29

	ADD	$f0,  $f26, $f0
	MUL	$f14, $f22, $f26
	ADD	$f1,  $f27, $f1
	MUL	$f15, $f23, $f27

	ADD	$f2,  $f28, $f2
	MUL	$f14, $f23, $f28
	ADD	$f30, $f29, $f30
	MUL	$f15, $f22, $f29

	ADD	$f0,  $f26, $f0
	MUL	$f16, $f24, $f26
	ADD	$f1,  $f27, $f1
	MUL	$f17, $f25, $f27

	ADD	$f2,  $f28, $f2
	MUL	$f16, $f25, $f28
	ADD	$f30, $f29, $f30
	MUL	$f17, $f24, $f29

	ADD	$f0,  $f26, $f0
	ADD	$f1,  $f27, $f1
	ADD	$f2,  $f28, $f2
	ADD	$f30, $f29, $f30
	.align 4

$SubRemain:
	unop
	beq	$6, $End
	unop
	subq	$6,   1, $6

	LD	$f10,   0*SIZE($18)
	fclr	$f26
	LD	$f18,   0*SIZE($20)
	fclr	$f27

	LD	$f11,   1*SIZE($18)
	fclr	$f28
	LD	$f19,   1*SIZE($20)
	fclr	$f29

	SXADDQ	$19, $18, $18
	SXADDQ	$21, $20, $20
	nop
	beq	$6, $SubRemainLoopEnd
	.align 4

$SubRemainLoop:
	ADD	$f0,  $f26, $f0
	MUL	$f10, $f18, $f26
	ADD	$f1,  $f27, $f1
	MUL	$f11, $f19, $f27

	ADD	$f2,  $f28, $f2
	MUL	$f10, $f19, $f28
	LD	$f10,   0*SIZE($18)
	LD	$f19,   1*SIZE($20)

	ADD	$f30, $f29, $f30
	MUL	$f11, $f18, $f29
	LD	$f11,   1*SIZE($18)
	LD	$f18,   0*SIZE($20)

	subq	$6,   1, $6
	SXADDQ	$19, $18, $18
	SXADDQ	$21, $20, $20
	bgt	$6, $SubRemainLoop
	.align 4

$SubRemainLoopEnd:
	ADD	$f0,  $f26, $f0
	MUL	$f10, $f18, $f26
	ADD	$f1,  $f27, $f1
	MUL	$f11, $f19, $f27

	ADD	$f2,  $f28, $f2
	MUL	$f10, $f19, $f28
	ADD	$f30, $f29, $f30
	MUL	$f11, $f18, $f29

	ADD	$f0,  $f26, $f0
	ADD	$f1,  $f27, $f1
	ADD	$f2,  $f28, $f2
	ADD	$f30, $f29, $f30
	.align 4

$End:
#ifndef CONJ
	SUB	$f0, $f1,  $f0
	ADD	$f2, $f30, $f1
#else
	ADD	$f0, $f1,  $f0
	SUB	$f2, $f30, $f1
#endif
	ldt	$f2,   0($30)
	lda	$30,  16($30)

	ST	$f0, 0*SIZE($16)
	ST	$f1, 1*SIZE($16)

	ret
	.end NAME
