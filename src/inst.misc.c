#include "inst.h"

#include "base.h"
#include "memman.h"

int _rem_rem(Instance* inst, Register* dst, Register* src) {
	return OK;
}

int _dat_var(Instance* inst, Register* dst, Register* src) {
	if (src == 0) return ERR;
	return inst->mm->join(inst->mm->p, dst->data.vPtr, src);
}

int _dat_int(Instance* inst, Register* dst, Register* src) {
	return OK;
}

int _dat_char(Instance* inst, Register* dst, Register* src) {
	return OK;
}

int _dat_float(Instance* inst, Register* dst, Register* src) {
	return OK;
}

int _dat_str(Instance* inst, Register* dst, Register* src) {
	return OK;
}
