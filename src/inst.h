#ifndef __INST_H_
#define __INST_H_


#include "nsasm.priv.h"

// core instructions
int _fun_mov(Instance* inst, Register* dst, Register* src);
int _fun_push(Instance* inst, Register* dst, Register* src);
int _fun_pop(Instance* inst, Register* dst, Register* src);
int _fun_in(Instance* inst, Register* dst, Register* src);
int _fun_out(Instance* inst, Register* dst, Register* src);
int _fun_prt(Instance* inst, Register* dst, Register* src);

// control instructions
int _fun_cmp(Instance* inst, Register* dst, Register* src);
int _fun_jmp(Instance* inst, Register* dst, Register* src);
int _fun_jz(Instance* inst, Register* dst, Register* src);
int _fun_jnz(Instance* inst, Register* dst, Register* src);
int _fun_jg(Instance* inst, Register* dst, Register* src);
int _fun_jl(Instance* inst, Register* dst, Register* src);
int _fun_run(Instance* inst, Register* dst, Register* src);
int _fun_call(Instance* inst, Register* dst, Register* src);
int _fun_end(Instance* inst, Register* dst, Register* src);
int _fun_nop(Instance* inst, Register* dst, Register* src);
int _fun_rst(Instance* inst, Register* dst, Register* src);

// numeric instructions
int _fun_add(Instance* inst, Register* dst, Register* src);
int _fun_inc(Instance* inst, Register* dst, Register* src);
int _fun_sub(Instance* inst, Register* dst, Register* src);
int _fun_dec(Instance* inst, Register* dst, Register* src);
int _fun_mul(Instance* inst, Register* dst, Register* src);
int _fun_div(Instance* inst, Register* dst, Register* src);

// logic instructions
int _fun_and(Instance* inst, Register* dst, Register* src);
int _fun_or(Instance* inst, Register* dst, Register* src);
int _fun_xor(Instance* inst, Register* dst, Register* src);
int _fun_not(Instance* inst, Register* dst, Register* src);
int _fun_shl(Instance* inst, Register* dst, Register* src);
int _fun_shr(Instance* inst, Register* dst, Register* src);

// misc instructions
int _rem_rem(Instance* inst, Register* dst, Register* src);
int _dat_var(Instance* inst, Register* dst, Register* src);
int _dat_int(Instance* inst, Register* dst, Register* src);
int _dat_char(Instance* inst, Register* dst, Register* src);
int _dat_float(Instance* inst, Register* dst, Register* src);
int _dat_str(Instance* inst, Register* dst, Register* src);


#endif
