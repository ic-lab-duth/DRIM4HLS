	.text
	.balign 4
	.global custom
	
custom:
	li x1, 9
	li x2, 12
	li x4, 0x8068
more:
	add x1, x1, x3
	jal x5, nojump
nojump:
	sra x4, x4, x3
	add x1, x1, x3
	bge x1, x4, bigjump
	bge x1, x4, more
bigjump:
	bge x1, x3, done
done:		
	ret
