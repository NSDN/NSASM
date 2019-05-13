#include "inst.h"

#include "calc.h"

int _fun_add(Instance* inst, Register* dst, Register* src) {
	if (src == 0) return ERR;
	if (dst->readOnly) return ERR;
	return __calc__(dst, src, '+');
}

int _fun_inc(Instance* inst, Register* dst, Register* src) {
	if (dst->readOnly) return ERR;
	Register r;
	r.readOnly = 0;
	r.type = RegChar;
	r.data.vChar = 1;
	return __calc__(dst, &r, '+');
}

int _fun_sub(Instance* inst, Register* dst, Register* src) {
	if (src == 0) return ERR;
	if (dst->readOnly) return ERR;
	return __calc__(dst, src, '-');
}

int _fun_dec(Instance* inst, Register* dst, Register* src) {
	if (dst->readOnly) return ERR;
	Register r;
	r.readOnly = 0;
	r.type = RegChar;
	r.data.vChar = 1;
	return __calc__(dst, &r, '-');
}

int _fun_mul(Instance* inst, Register* dst, Register* src) {
	if (src == 0) return ERR;
	if (dst->readOnly) return ERR;
	return __calc__(dst, src, '*');
}

int _fun_div(Instance* inst, Register* dst, Register* src) {
	if (src == 0) return ERR;
	if (dst->readOnly) return ERR;
	return __calc__(dst, src, '/');
}
