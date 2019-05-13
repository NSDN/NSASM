#include "base.h"

#include "nsasm.priv.h"
#include "inst.h"

#include "util.h"

const static int FUN_NO_OPER_CNT = 4;
const static Function funList[] = {
	{ "rem", &_rem_rem },
	{ "end", &_fun_end },
	{ "nop", &_fun_nop },
	{ "rst", &_fun_rst },

	{ "var", &_dat_var },
	{ "int", &_dat_int },
	{ "char", &_dat_char },
	{ "float", &_dat_float },
	{ "str", &_dat_str },
	
	{ "mov", &_fun_mov },
	{ "push", &_fun_push },
	{ "pop", &_fun_pop },
	{ "in", &_fun_in },
	{ "out", &_fun_out },
	{ "add", &_fun_add },
	{ "inc", &_fun_inc },
	{ "sub", &_fun_sub },
	{ "dec", &_fun_dec },
	{ "mul", &_fun_mul },
	{ "div", &_fun_div },
	{ "cmp", &_fun_cmp },
	{ "jmp", &_fun_jmp },
	{ "jz", &_fun_jz },
	{ "jnz", &_fun_jnz },
	{ "jg", &_fun_jg },
	{ "jl", &_fun_jl },
	{ "and", &_fun_and },
	{ "or", &_fun_or },
	{ "xor", &_fun_xor },
	{ "not", &_fun_not },
	{ "shl", &_fun_shl },
	{ "shr", &_fun_shr },
	{ "run", &_fun_run },
	{ "call", &_fun_call },

	{ "prt", &_fun_prt },

	{ "\0", 0 }
};

int getNoperCount() {
	return FUN_NO_OPER_CNT;
}

Function* getFuncList() {
	return (Function*) &(funList[0]);
}

int nsasm(int argc, char* argv[]) {
	print("NyaSama Assembly Script Module\n");
	print("Version: %s\n\n", VERSION);
	if (argc < 2) {
		print("Usage: nsasm [c/r] [FILE]\n\n");
		return OK;
	} else {
		if (argc == 3) {
			if (strchr(argv[1], 'r') > 0) {
				run(read(argv[2]));
				return OK;
			}
		}
		if (strchr(argv[1], 'c') > 0) {
			console();
			return OK;
		}
		run(read(argv[1]));
		return OK;
	}
}
