#include "inst.h"

#include "calc.h"

int _fun_and(Instance* inst, Register* dst, Register* src) {
	if (src == 0) return ERR;
	if (dst->readOnly) return ERR;
	return __calc__(dst, src, '&');
}

int _fun_or(Instance* inst, Register* dst, Register* src) {
	if (src == 0) return ERR;
	if (dst->readOnly) return ERR;
	return __calc__(dst, src, '|');
}
int _fun_xor(Instance* inst, Register* dst, Register* src) {
	if (src == 0) return ERR;
	if (dst->readOnly) return ERR;
	return __calc__(dst, src, '^');
}

int _fun_not(Instance* inst, Register* dst, Register* src) {
	if (dst->readOnly) return ERR;
	Register r;
	r.readOnly = 0;
	r.type = RegChar;
	r.data.vChar = 0;
	return __calc__(dst, &r, '~');
}

int _fun_shl(Instance* inst, Register* dst, Register* src) {
	if (src == 0) return ERR;
	if (dst->readOnly) return ERR;
	return __calc__(dst, src, '<');
}

int _fun_shr(Instance* inst, Register* dst, Register* src) {
	if (src == 0) return ERR;
	if (dst->readOnly) return ERR;
	return __calc__(dst, src, '>');
}
