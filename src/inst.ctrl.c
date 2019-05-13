#include "inst.h"

#include "base.h"
#include "memman.h"

#include "util.h"

int _fun_cmp(Instance* inst, Register* dst, Register* src) {
	if (src == 0) return ERR;
	if (_fun_mov(inst, &(inst->state), dst) == ERR) return ERR;
	if (_fun_sub(inst, &(inst->state), src) == ERR) return ERR;
	switch (inst->state.type) {
		case RegChar:
			inst->state.data.vInt = inst->state.data.vChar;
			break;
		case RegFloat:
			inst->state.data.vInt = (int) inst->state.data.vFloat;
			break;
		case RegInt:
			break;
		case RegPtr:
			return ERR;
	}
	inst->state.type = RegInt;
	return OK;
}

int _fun_jmp(Instance* inst, Register* dst, Register* src) {
	if (dst->type != RegPtr) return ERR;
	if (verifyTag(dst->data.vPtr) == ERR) return ERR;
	strcpy(inst->tag, dst->data.vPtr);
	return OK;
}

int _fun_jz(Instance* inst, Register* dst, Register* src) {
	if (dst->type != RegPtr) return ERR;
	if (verifyTag(dst->data.vPtr) == ERR) return ERR;
	if (inst->state.data.vInt == 0) strcpy(inst->tag, dst->data.vPtr);
	return OK;
}

int _fun_jnz(Instance* inst, Register* dst, Register* src) {
	if (dst->type != RegPtr) return ERR;
	if (verifyTag(dst->data.vPtr) == ERR) return ERR;
	if (inst->state.data.vInt != 0) strcpy(inst->tag, dst->data.vPtr);
	return OK;
}

int _fun_jg(Instance* inst, Register* dst, Register* src) {
	if (dst->type != RegPtr) return ERR;
	if (verifyTag(dst->data.vPtr) == ERR) return ERR;
	if (inst->state.data.vInt > 0) strcpy(inst->tag, dst->data.vPtr);
	return OK;
}

int _fun_jl(Instance* inst, Register* dst, Register* src) {
	if (dst->type != RegPtr) return ERR;
	if (verifyTag(dst->data.vPtr) == ERR) return ERR;
	if (inst->state.data.vInt < 0) strcpy(inst->tag, dst->data.vPtr);
	return OK;
}

int _fun_run(Instance* inst, Register* dst, Register* src) {
	if (dst->type != RegPtr) return ERR;
	print("[+] %s\n\n", dst->data.vPtr);
	run(read(dst->data.vPtr));
	print("[-] %s\n\n", dst->data.vPtr);
	return OK;
}

int _fun_call(Instance* inst, Register* dst, Register* src) {
	if (dst->type != RegPtr) return ERR;
	call(dst->data.vPtr, inst);
	return OK;
}

int _fun_end(Instance* inst, Register* dst, Register* src) {
	return ETC;
}

int _fun_nop(Instance* inst, Register* dst, Register* src) {
	#ifdef WINDOWS
		__asm { 
			nop;
		}
	#else
		__asm__("nop");
	#endif
	return OK;
}

int _fun_rst(Instance* inst, Register* dst, Register* src) {
	int stackSiz = 0, heapSiz = 0;
	stackSiz = inst->mm->p->stackSiz;
	heapSiz = inst->mm->p->heapSiz;
	DisposeMemoryManager(inst->mm);
	inst->mm = InitMemoryManager(stackSiz, heapSiz);
	memset(inst->reg, 0, sizeof(Register) * REG_CNT);
	inst->tag[0] = '\0';
	inst->cnt = -1;
 	return OK;
}
