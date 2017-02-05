.data {
	var a = 0
	var b = 1.414
	var c = "hello"
	str d = "world"
}

.code {
	mov r1, a
	inc r1
	mov a, r1
	mov r2, 0x32
	add r2, r1
	out 0x00, r2
	mov r2, '\n'
	out 0x00, r2
	mov r1, b
	mov r2, c
	dec r1
	inc r2
	out 0x00, r1
	mov r1, '\n'
	out 0x00, r1
	out 0x00, r2
	mov r2, ' '
	out 0x00, r2
	mov r2, d
	out 0x00, r2
	end
}

