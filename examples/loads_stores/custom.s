	.text
	.balign 4
	.global custom
	
custom:
	li x1, 9
	li x2, 12
	li x4, 0x8068
	sw x1, 0(x2)
	sw x2, 0(x2)
	sw x4, 0(x2)
	lw x3, 0(x2)
	addi x3, x3, 1
	addi x3, x3, 2
	lw x5, 0(x2)
	lw x6, 0(x2)
	lw x7, 0(x2)
