.conf {
	stack 16
	heap 32
}

.data {
	var a = 1
	var b = 2.0F
	var c = 'c'
	var d = "gensokyo"
}

.code {
	mov r1, 1234
	mov r2, 1.5F
	mov r3, 0xFF
	mov r4, 'a'
	mov r5, 6.0F
	
	add r1, r3
	inc r4
	mul r2, 2.0F
	div r5, 3.0F
	
	add a, r1
	sub r5, b
	mul b, r4
	
	push r1
	xor r1, r1
	pop r1
	
	inc d
	
	out 0, "R1: "
	out 0, r1
	out 0, '\n'
	out 0, "R2: "
	out 0, r2
	out 0, '\n'
	out 0, "R4: "
	out 0, r4
	out 0, '\n'
	out 0, "R5: "
	out 0, r5
	out 0, '\n'
	out 0, '\n'
	out 0, a
	out 0, '\n'
	out 0, b
	out 0, '\n'
	out 0, c
	out 0, '\n'
	out 0, d
	out 0, '\n'
	
	mov r1, 10
	[jump]
	out 0, "+1s "
	dec r1
	cmp r1, 0
	jnz [jump]
	
	out 0, '\n'
	
	end
}
