#if !defined(__GNUC__)
	.extern _cerror

	.ent	ntv_sys
	.globl	ntv_sys
ntv_sys:
	.prologue 0

	/*
	 * Kernel expects syscall number in v0 and the args in a0 - a4.
	 * Need to shift them.
	 */
	mov	$16, $0
	mov	$17, $16
	mov	$18, $17
	mov	$19, $18
	mov	$20, $19
	mov	$21, $20

        /*
	**   mmap has 6 arguments...
	*/
	ldl    $21, ($sp)

	/*
	 * Make the syscall.
	 */
	call_pal 0x83
	bne	$19, $error
	ret

$error:
	br	$gp, 1f
1:	ldgp	$gp, 0($gp)
	jmp	$31, _cerror

	.end	ntv_sys
#endif
