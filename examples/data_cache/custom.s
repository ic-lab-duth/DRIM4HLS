	.text
	.balign 4
	.global custom
	
custom:
	li x1, 0
	li x2, 4
	li x3, 8
	li x4, 16
	li x5, 1024
	sw x2, 0(x1)
	sw x3, 4(x1)
	sw x3, 256(x1)
	sw x3, 264(x1)
	sw x3, 260(x1)
	lw x6, 4(x1)
	sw x3, 512(x1)
	sw x3, 524(x1)
	sw x4, 1024(x1)
	lw x7, 0(x1)
	sw x4, 1024(x5)
	sw x6, 32(x1)
	sw x2, 36(x1)
	sw x3, 40(x1)
	lw x6, 36(x1)
	lw x7, 256(x1)
	lw x7, 64(x1)