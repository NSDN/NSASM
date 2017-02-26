.conf {
    stack 16
    heap 32
}

.data {
    var plus = '+'
    var one = 0x01
    var sec = "s "
}

.code {
    mov r1, r7
[jump]
	out 0, plus
    out 0, one
    out 0, sec
	dec r1
	cmp r1, 0
	jnz [jump]
	
	out 0, '\n'
}
