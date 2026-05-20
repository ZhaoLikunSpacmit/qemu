	.option	norvc

	.equ CSR_MTILEM, 0x803
	.equ CSR_MTILEN, 0x804
	.equ CSR_MTILEK, 0x805
	.equ EXCP_ILLEGAL_INST, 2

	.text
	.global _start
	.global _exit

_start:
	lla	t0, trap
	csrw	mtvec, t0

	li	t0, 7
	csrw	CSR_MTILEM, t0
	csrr	t1, CSR_MTILEM
	bne	t0, t1, fail

	li	t0, 9
	csrw	CSR_MTILEN, t0
	csrr	t1, CSR_MTILEN
	bne	t0, t1, fail

	li	t0, 2
	csrw	CSR_MTILEK, t0
	csrr	t1, CSR_MTILEK
	bne	t0, t1, fail

	li	t0, 33
	csrw	CSR_MTILEM, t0
	csrr	t1, CSR_MTILEM
	li	t2, 7
	bne	t1, t2, fail

	li	t0, 33
	csrw	CSR_MTILEN, t0
	csrr	t1, CSR_MTILEN
	li	t2, 9
	bne	t1, t2, fail

	li	t0, 5
	csrw	CSR_MTILEK, t0
	csrr	t1, CSR_MTILEK
	li	t2, 2
	bne	t1, t2, fail

	la	t0, trap_count
	ld	t1, 0(t0)
	li	t2, 3
	bne	t1, t2, fail

	li	a0, 0
	j	_exit

trap:
	csrr	t0, mcause
	li	t1, EXCP_ILLEGAL_INST
	bne	t0, t1, fail

	csrr	t0, mepc
	csrr	t1, mtval
	lwu	t2, 0(t0)
	bne	t1, t2, fail

	la	t3, trap_count
	ld	t4, 0(t3)
	addi	t4, t4, 1
	sd	t4, 0(t3)

	addi	t0, t0, 4
	csrw	mepc, t0
	mret

fail:
	li	a0, 1

_exit:
	lla	a1, semiargs
	li	t0, 0x20026
	sd	t0, 0(a1)
	sd	a0, 8(a1)
	li	a0, 0x20

	.balign	16
	slli	zero, zero, 0x1f
	ebreak
	srai	zero, zero, 0x7
	j	.

	.data
	.balign	16
semiargs:
	.space	16

	.balign	8
trap_count:
	.dword	0