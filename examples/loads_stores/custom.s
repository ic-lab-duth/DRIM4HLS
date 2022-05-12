	.text
	.balign 4
	.global custom
	
custom:
	li x1, 9
	li x2, 12
	li x4, 0x8068
	sw x2, 0(x2)
	lw x3, 0(x2)
	add x2, x2, x1
	sw x2, 0(x1)
	addi x2, x2, 14
	lw x4, 0(x1)
	sw x4, 4(x1)
