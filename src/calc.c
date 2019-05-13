#include "calc.h"

int __calc_char__(char* dst, char src, char fun) {
	switch (fun) {
		case '+': *dst += src; break;
		case '-': *dst -= src; break;
		case '*': *dst *= src; break;
		case '/': *dst /= src; break;
		case '&': *dst &= src; break;
		case '|': *dst |= src; break;
		case '~': *dst = ~(*dst); break;
		case '^': *dst ^= src; break;
		case '<': *dst = *dst << src; break;
		case '>': *dst = *dst >> src; break;
		default: return ERR;
	}
	return OK;
}

int __calc_int__(int* dst, int src, char fun) {
	switch (fun) {
		case '+': *dst += src; break;
		case '-': *dst -= src; break;
		case '*': *dst *= src; break;
		case '/': *dst /= src; break;
		case '&': *dst &= src; break;
		case '|': *dst |= src; break;
		case '~': *dst = ~(*dst); break;
		case '^': *dst ^= src; break;
		case '<': *dst = *dst << src; break;
		case '>': *dst = *dst >> src; break;
		default: return ERR;
	}
	return OK;
}

int __calc_float__(float* dst, float src, char fun) {
	switch (fun) {
		case '+': *dst += src; break;
		case '-': *dst -= src; break;
		case '*': *dst *= src; break;
		case '/': *dst /= src; break;
		case '&': return ERR;
		case '|': return ERR;
		case '~': return ERR;
		case '^': return ERR;
		case '<': return ERR;
		case '>': return ERR;
		default: return ERR;
	}
	return OK;
}

int __calc_ptr__(char** dst, int src, char fun) {
	switch (fun) {
		case '+': *dst += src; break;
		case '-': *dst -= src; break;
		case '*': return ERR;
		case '/': return ERR;
		case '&': return ERR;
		case '|': return ERR;
		case '~': return ERR;
		case '^': return ERR;
		case '<': return ERR;
		case '>': return ERR;
		default: return ERR;
	}
	return OK;
}

int __calc__(Register* dst, Register* src, char fun) {
	switch (dst->type) {
		case RegChar:
			switch (src->type) {
				case RegChar:
					return __calc_char__(&(dst->data.vChar), src->data.vChar, fun);
				case RegFloat:
					return __calc_char__(&(dst->data.vChar), src->data.vFloat, fun);
				case RegInt:
					return __calc_char__(&(dst->data.vChar), src->data.vInt, fun);
				case RegPtr:
					return ERR;
			}
			break;
		case RegFloat:
			switch (src->type) {
				case RegChar:
					return __calc_float__(&(dst->data.vFloat), src->data.vChar, fun);
				case RegFloat:
					return __calc_float__(&(dst->data.vFloat), src->data.vFloat, fun);
				case RegInt:
					return __calc_float__(&(dst->data.vFloat), src->data.vInt, fun);
				case RegPtr:
					return ERR;
			}
			break;
		case RegInt:
			switch (src->type) {
				case RegChar:
					return __calc_int__(&(dst->data.vInt), src->data.vChar, fun);
				case RegFloat:
					return __calc_int__(&(dst->data.vInt), src->data.vFloat, fun);
				case RegInt:
					return __calc_int__(&(dst->data.vInt), src->data.vInt, fun);
				case RegPtr:
					return ERR;
			}
			break;
		case RegPtr:
			switch (src->type) {
				case RegChar:
					return __calc_ptr__(&(dst->data.vPtr), src->data.vChar, fun);
				case RegFloat:
					return ERR;
				case RegInt:
					return __calc_ptr__(&(dst->data.vPtr), src->data.vInt, fun);
				case RegPtr:
					return ERR;
			}
	}
	return OK;
}
