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
	end
}
