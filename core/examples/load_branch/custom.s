	.text
	.balign 4
	.global custom
	
done:
	ret
custom:
	li x1, 9
	li x2, 12
	li x4, 0x8068
	add x2, x2, x1
	addi x2, x2, 14
	sw x1, 0(x4)
	lw x5, 0(x4)
	bge x2, x5, done
