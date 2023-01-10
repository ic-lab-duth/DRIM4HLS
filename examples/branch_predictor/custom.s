	.text
	.balign 4
	.global custom
	
custom:
	li x1, 0
	li x2, 4
	li x3, 8
	li x4, 16
	li x5, 32
	li x6, 64
	bge x2, x1, 60
	add x4, x1, x4
	add x4, x1, x4
	addi x1, x1, 1
	bge x1, x2, 76
	bge x1, x2, 96
	add x4, x1, x4
	add x4, x1, x4
	j   56
	add x4, x1, x4
	add x4, x1, x4
	