#include "inst.h"

#include "base.h"
#include "memman.h"

int _fun_mov(Instance* inst, Register* dst, Register* src) {
	if (src == 0) return ERR;
	if (dst->readOnly) return ERR;
	if (dst->type == RegChar && src->type == RegPtr) {
		dst->data.vChar = src->data.vPtr[0];
	} else if (dst->type == RegPtr && src->type == RegChar) {
		dst->data.vPtr[0] = src->data.vChar;
	} else {
		memcpy(dst, src, sizeof(Register));
		if (dst->readOnly) dst->readOnly = 0;
	}
	return OK;
}

int _fun_push(Instance* inst, Register* dst, Register* src) {
	return inst->mm->push(inst->mm->p, dst);
}

int _fun_pop(Instance* inst, Register* dst, Register* src) {
	if (dst->readOnly) return ERR;
	return inst->mm->pop(inst->mm->p, dst);
}

int _fun_in(Instance* inst, Register* dst, Register* src) {
	if (dst->readOnly) return ERR;
	char buf[IOBUF] = "";
	switch (dst->type) {
		case RegChar:
			fscan(buf, "%c", &(dst->data.vChar));
			break;
		case RegFloat:
			fscan(buf, "%f", &(dst->data.vFloat));
			break;
		case RegInt:
			fscan(buf, "%d", &(dst->data.vInt));
			break;
		case RegPtr:
			fscan(buf, "%s", dst->data.vPtr);
			break;
	}
	return OK;
}

int _fun_out(Instance* inst, Register* dst, Register* src) {
	if (src == 0) return ERR;
	switch (dst->data.vInt) {
		case 0x00:
			switch (src->type) {
				case RegChar:
					print("%c", src->data.vChar);
					break;
				case RegFloat:
					print("%f", src->data.vFloat);
					break;
				case RegInt:
					print("%d", src->data.vInt);
					break;
				case RegPtr:
					print("%s", src->data.vPtr);
					break;
			}
			break;
		case 0xFF:
			print("[DEBUG] ");
			switch (src->type) {
				case RegChar:
					print("%c", src->data.vChar);
					break;
				case RegFloat:
					print("%f", src->data.vFloat);
					break;
				case RegInt:
					print("%d", src->data.vInt);
					break;
				case RegPtr:
					print("%s", src->data.vPtr);
					break;
			}
			break;
		default:
			return ERR;
	}
	return OK;
}

int _fun_prt(Instance* inst, Register* dst, Register* src) {
	switch (dst->type) {
		case RegChar:
			print("%c", dst->data.vChar);
			break;
		case RegFloat:
			print("%f", dst->data.vFloat);
			break;
		case RegInt:
			print("%d", dst->data.vInt);
			break;
		case RegPtr:
			print("%s", dst->data.vPtr);
			break;
	}
	print("\n");
	return OK;
}
