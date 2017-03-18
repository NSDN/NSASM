/* ---- DEFINE ---- */
//#define WINDOWS
#define LINUX
//#define ARDUINO
//#define STM32
/* ---- DEFINE ---- */



int nsasm(int argc, char* argv[]);

#include <stdio.h>
#include <string.h>

#define APP_NAME nsasm

#if defined(WINDOWS)
#define _CRT_SECURE_NO_WARNINGS
#else
char* strlwr(char* s) {
	char* str;
	str = s;
	while(*str != '\0') {
		if(*str >= 'A' && *str <= 'Z') {
			*str += 'a'-'A';
		}
		str++;
	}
	return s; 
}
#endif

#if defined(WINDOWS)
#include <stdlib.h>
#include <vadefs.h>
#define clearScreen() system("cls")
int print(const char* format, ...) {
	va_list args = 0;
	__crt_va_start(args, format);
	int result = vprintf(format, args);
	__crt_va_end(args);
	return result;
}
int scan(char* buffer) { return gets(buffer); }
int fscan(char* buffer, const char* format, ...) {
	gets(buffer);
	va_list args = 0;
	__crt_va_start(args, format);
	int result = vsscanf(buffer, format, args);
	__crt_va_end(args);
	return result;
}

int main(int argc, char* argv[]) {
	int result;
	result = APP_NAME(argc, argv);
	system("pause");
	return result;
}

#elif defined(LINUX)
#include <stdlib.h>
#include <stdarg.h>
#define IOBUF 128
#define clearScreen() system("clear")
int print(const char* format, ...) {
	va_list args;
	va_start(args, format);
	int result = vprintf(format, args);
	va_end(args);
	return result;
}
char* scan(char* buffer) {
	char count = 0, tmp = 0;
	while (1) {
		tmp = getchar();
		if (tmp == '\n') break;
		buffer[count] = tmp;
		if (buffer[count] == 0x08 && count > 0) {
			count -= 1;
			print("%c", 0x08);
			continue;
		}
		else if (buffer[count] != 0x08) {
			print("%c", buffer[count]);
			count += 1;
		}
	}
	buffer[count] = '\0';
	print("\n");
}
int fscan(char* buffer, const char* format, ...) {
	scan(buffer);
	va_list args;
	va_start(args, format);
	int result = vsscanf(buffer, format, args);
	va_end(args);
	return result;
}

int main(int argc, char* argv[]) {
	int result;
	result = APP_NAME(argc, argv);
	return result;
}

#elif defined(ARDUINO)
#include <stdarg.h>
#define BACKCOLOR 0x0000
#define __print(buf) tft.print(buf)
#define clearScreen() { tft.setCursor(0, 0); tft.fillScreen(BACKCOLOR); }
#define IOBUF 128
int print(const char* format, ...) {
	char* iobuf = malloc(sizeof(char) * IOBUF);
	va_list args;
	va_start(args, format);
	int result = vsprintf(iobuf, format, args);
	va_end(args);
	__print(iobuf);
	free(iobuf);
	return result;
}
int scan(char* buffer) {
	char count = 0;
	while (true) {
		if (Serial.available() > 0) {
			if (Serial.peek() == '\n') break;
			buffer[count] = Serial.read();
			if (buffer[count] == 0x08 && count > 0) {
				count -= 1;
				print("%c", 0x08);
				continue;
			}
			else if (buffer[count] != 0x08) {
				print("%c", buffer[count]);
				count += 1;
			}
		}
		else if (Serial1.available() > 0) {
			if (Serial1.peek() == '\n') break;
			buffer[count] = Serial1.read();
			if (buffer[count] == 0x08 && count > 0) {
				count -= 1;
				print("%c", 0x08);
				continue;
			}
			else if (buffer[count] != 0x08) {
				print("%c", buffer[count]);
				count += 1;
			}
		}
	}
	buffer[count] = '\0';
	if (Serial.available() > 0) Serial.read();
	else if (Serial1.available() > 0) Serial1.read();
	print("\n");
}
int fscan(char* buffer, const char* format, ...) {
	scan(buffer);
	va_list args = 0;
	va_start(args, format);
	int result = vsscanf(buffer, format, args);
	va_end(args);
	return result;
}

#elif defined(STM32)
#include <stdarg.h>
#define HUART huart2
#define __print(buf) lcd->printfa(lcd->p, buf)
#define clearScreen() lcd->clear(lcd->p)
#define IOBUF 128
int print(const char* format, ...) {
	char* iobuf = malloc(sizeof(char) * IOBUF);
	va_list args;
	va_start(args, format);
	int result = vsprintf(iobuf, format, args);
	va_end(args);
	__print(iobuf);
	free(iobuf);
	return result;
}
int scan(char* buffer) {
	char count = 0, tmp = '\0';
	while (1) {
		if (HAL_UART_Receive(&HUART, &tmp, 1, 1) == HAL_OK) {
			if (tmp == '\n') break;
			buffer[count] = tmp;
			if (buffer[count] == 0x08 && count > 0) {
				count -= 1;
				print("%c", 0x08);
				continue;
			} else if (buffer[count] != 0x08) {
				print("%c", buffer[count]);
				count += 1;
			}
		}
	}
	buffer[count] = '\0';
	print("\n");
}
int fscan(char* buffer, const char* format, ...) {
	scan(buffer);
	va_list args = 0;
	va_start(args, format);
	int result = vsscanf(buffer, format, args);
	va_end(args);
	return result;
}

#endif

/* -------------------------------- */

#define VERSION 0.2

#define OK 0
#define ERR 1
#define ETC -1

typedef enum {
	RegInt,
	RegFloat,
	RegChar,
	RegPtr
} RegType;

typedef struct {
	char readOnly;
	RegType type;
	union {
		char vChar;
		int vInt;
		float vFloat;
		char* vPtr;
	} data;
} Register;

typedef Register MMBlock;
typedef struct _mmtype {
	MMBlock var;
	char* name;
	struct _mmtype* prev;
	struct _mmtype* next;
} MMType;

typedef struct {
	int stackSiz;
	int heapSiz;
	int stackCnt;
	int heapCnt;
	MMType* stackTop;
	MMType* heapStart;
	MMType* heapEnd;
} pMM;

typedef struct {
	pMM* p;
	int (*push)(pMM* p, MMBlock* blk);
	int (*pop)(pMM* p, MMBlock* blk);
	int (*join)(pMM* p, char* name, MMBlock* blk);
	MMBlock* (*get)(pMM* p, char* name);
	int (*exit)(pMM* p, char* name);
} MemoryManager;

int _mm_push(pMM* p, MMBlock* blk) {
	if (p->stackTop == 0) {
		p->stackTop = malloc(sizeof(MMType));
		p->stackTop->prev = 0;
	} else {
		p->stackTop->next = malloc(sizeof(MMType));
		p->stackTop->next->prev = p->stackTop;
		p->stackTop = p->stackTop->next;
	}
	if (p->stackCnt >= p->stackSiz) return ERR;
	memcpy(&(p->stackTop->var), blk, sizeof(MMBlock));
	p->stackCnt += 1;
	p->stackTop->next = 0;
	return OK;
}

int _mm_pop(pMM* p, MMBlock* blk) {
	if (p->stackTop->prev == 0) {
		memcpy(blk, &(p->stackTop->var), sizeof(MMBlock));
		free(p->stackTop);
		p->stackTop = 0;
		p->stackCnt = 0;
		return OK;
	}
	if (p->stackTop == 0) return ERR;
	memcpy(blk, &(p->stackTop->var), sizeof(MMBlock));
	p->stackCnt -= 1;
	p->stackTop = p->stackTop->prev;
	free(p->stackTop->next);
	return OK;
}

MMType* _mm_search(pMM* p, char* name) {
	MMType* tp = p->heapStart;
	if (p->heapCnt == 0) return OK;
	do {
		if (tp->name == 0) break;
		if (strcmp(tp->name, name) == 0) return tp;
		else tp = tp->next;
	} while (tp != 0);
	return OK;
}

int _mm_join(pMM* p, char* name, MMBlock* blk) {
	if (p->heapStart == 0) {
		p->heapStart = malloc(sizeof(MMType));
		p->heapStart->prev = 0;
		p->heapEnd = p->heapStart;
	} else {
		p->heapEnd->next = malloc(sizeof(MMType));
		p->heapEnd->next->prev = p->heapEnd;
		p->heapEnd = p->heapEnd->next;
	}
	p->heapEnd->name = 0;
	if (p->heapCnt >= p->heapSiz) return ERR;
	if (_mm_search(p, name) != 0) return ERR;
	p->heapEnd->name = malloc(sizeof(char) * (strlen(name) + 1));
	strcpy(p->heapEnd->name, name);
	memcpy(&(p->heapEnd->var), blk, sizeof(MMBlock));
	p->heapCnt += 1;
	p->heapEnd->next = 0;
	return OK;
}

MMBlock* _mm_get(pMM* p, char* name) {
	MMType* sp = 0;
	sp = _mm_search(p, name);
	if (sp == 0) return OK;
	return &(sp->var);
}

int _mm_exit(pMM* p, char* name) {
	MMType* sp = 0;
	sp = _mm_search(p, name);
	if (sp == 0) return ERR;
	if (sp == p->heapStart) {
		p->heapStart = sp->next;
		p->heapStart->prev = 0;
	} else if (sp == p->heapEnd) {
		p->heapEnd = sp->prev;
		p->heapEnd->next = 0;
	} else {
		sp->prev->next = sp->next;
		sp->next->prev = sp->prev;
	}
	p->heapCnt -= 1;
	free(sp->name);
	free(sp);
	return OK;
}

void _mm_clear(pMM* p) {
	MMType* ptr = p->stackTop;
	MMType* tmp = 0;

	while (ptr > 0) {
		tmp = ptr;
		ptr = ptr->prev;
		free(tmp);
	}
	
	ptr = p->heapStart;
	while (ptr > 0) {
		tmp = ptr;
		ptr = ptr->next;
		free(tmp);
	}
}

MemoryManager* InitMemoryManager(int stackSize, int heapSize) {
	pMM* p = malloc(sizeof(pMM));
	p->stackSiz = stackSize;
	p->heapSiz = heapSize;
	p->stackCnt = 0;
	p->heapCnt = 0;
	p->stackTop = 0;
	p->heapStart = 0;
	p->heapEnd = 0;
	
	MemoryManager* c = malloc(sizeof(MemoryManager));
	c->p = p;
	c->push = &_mm_push;
	c->pop = &_mm_pop;
	c->join = &_mm_join;
	c->get = &_mm_get;
	c->exit = &_mm_exit;
	
	return c;
}

void DisposeMemoryManager(MemoryManager* ptr) {
	_mm_clear(ptr->p);
	free(ptr->p);
	free(ptr);
}

#define REG_CNT 8
typedef struct {
	MemoryManager* mm;
	Register reg[REG_CNT];
	Register state;
	char tag[32];
	int cnt;
} Instance;

Instance* NewInstance(int stackSiz, int heapSiz) {
	Instance* ptr = malloc(sizeof(Instance));
	ptr->mm = InitMemoryManager(stackSiz, heapSiz);
	memset(ptr->reg, 0, sizeof(Register) * REG_CNT);
	memset(&ptr->state, 0, sizeof(Register));
	ptr->tag[0] = '\0';
	ptr->cnt = 0;
	return ptr;
}

void FreeInstance(Instance* ptr) {
	DisposeMemoryManager(ptr->mm);
	free(ptr);
}

/* -------------------------------- */

int _rem_rem(Instance* inst, Register* dst, Register* src);

int _dat_var(Instance* inst, Register* dst, Register* src);
int _dat_int(Instance* inst, Register* dst, Register* src);
int _dat_char(Instance* inst, Register* dst, Register* src);
int _dat_float(Instance* inst, Register* dst, Register* src);
int _dat_str(Instance* inst, Register* dst, Register* src);

int _fun_mov(Instance* inst, Register* dst, Register* src);
int _fun_push(Instance* inst, Register* dst, Register* src);
int _fun_pop(Instance* inst, Register* dst, Register* src);
int _fun_in(Instance* inst, Register* dst, Register* src);
int _fun_out(Instance* inst, Register* dst, Register* src);
int _fun_add(Instance* inst, Register* dst, Register* src);
int _fun_inc(Instance* inst, Register* dst, Register* src);
int _fun_sub(Instance* inst, Register* dst, Register* src);
int _fun_dec(Instance* inst, Register* dst, Register* src);
int _fun_mul(Instance* inst, Register* dst, Register* src);
int _fun_div(Instance* inst, Register* dst, Register* src);
int _fun_cmp(Instance* inst, Register* dst, Register* src);
int _fun_jmp(Instance* inst, Register* dst, Register* src);
int _fun_jz(Instance* inst, Register* dst, Register* src);
int _fun_jnz(Instance* inst, Register* dst, Register* src);
int _fun_jg(Instance* inst, Register* dst, Register* src);
int _fun_jl(Instance* inst, Register* dst, Register* src);
int _fun_and(Instance* inst, Register* dst, Register* src);
int _fun_or(Instance* inst, Register* dst, Register* src);
int _fun_xor(Instance* inst, Register* dst, Register* src);
int _fun_not(Instance* inst, Register* dst, Register* src);
int _fun_shl(Instance* inst, Register* dst, Register* src);
int _fun_shr(Instance* inst, Register* dst, Register* src);
int _fun_run(Instance* inst, Register* dst, Register* src);
int _fun_call(Instance* inst, Register* dst, Register* src);
int _fun_end(Instance* inst, Register* dst, Register* src);
int _fun_nop(Instance* inst, Register* dst, Register* src);
int _fun_rst(Instance* inst, Register* dst, Register* src);

typedef struct {
	char name[8];
	int (*fun)(Instance* inst, Register* dst, Register* src);
} Function;

#define FUN_NO_OPER_CNT 4
#if defined(ARDUINO)
static Function code funList[] = {
#else 
static Function funList[] = {
#endif
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

	{ "\0", 0 }
};

int getSymbolIndex(Function list[], char* var);
int verifyVarName(char* var);
int verifyTag(char* var);
int getRegister(Instance* inst, char* var, Register** ptr);

/* -------------------------------- */

int execute(Instance* inst, char* var, char type);
void console();
void run(char* var);
void call(char* var, Instance* prev);

char* read(char* path);
int lines(char* src);
char* line(char* src, int index);
char* cut(char* src, const char* head);
char* get(char* src, int start, char* buf, int size);

/* -------------------------------- */

int nsasm(int argc, char* argv[]) {
	print("NyaSama Assembly Script Module\n");
	print("Version: %1.2f\n\n", VERSION);
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

/* -------------------------------- */

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
	return inst->mm->push(inst->mm->p, dst);
}
int _fun_in(Instance* inst, Register* dst, Register* src) {
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
}
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

/* -------------------------------- */

int getSymbolIndex(Function list[], char* var) {
	for (int i = 0; list[i].name[0] != '\0'; i++) {
		if (strcmp(strlwr(var), list[i].name) == 0) {
			return i;
		}
	}
	return ETC;
}

int verifyVarName(char* var) {
	if (
		(var[0] >= '0' && var[0] <= '9') || 
		var[0] == 'r' || var[0] == 'R' || 
		var[0] == '-' || var[0] == '+' || 
		var[0] == '.' || var[0] == '['
	) return ERR;
	return OK;
}

int verifyTag(char* var) {
	if (var[0] == '[' && var[strlen(var) - 1] == ']') {
		return OK;
	}
	return ERR;
}

int getRegister(Instance* inst, char* var, Register** ptr) {
	if (var[0] == 'r' || var[0] == 'R') {
		int srn = -1;
		sscanf(var, "%*[rR]%d", &srn);
		if (srn >= 0 && srn < REG_CNT) {
			*ptr = &(inst->reg[srn]);
			return OK;
		} else return ERR;
	} else {
		if (var[0] == '\'') {
			if (var[strlen(var) - 1] != '\'') return ERR;
			char tmp = 0;
			if (sscanf(var, "%*[\'\\]%[^\']c", &tmp)) {
				*ptr = malloc(sizeof(Register));
				switch (tmp) {
					case 'n': (*ptr)->data.vChar = '\n'; break;
					case 'r': (*ptr)->data.vChar = '\r'; break;
					case 't': (*ptr)->data.vChar = '\t'; break;
					case '\\': (*ptr)->data.vChar = '\\'; break;
					default: (*ptr)->data.vChar = tmp; break;
				}
				(*ptr)->type = RegChar;
				(*ptr)->readOnly = 0;
				return ETC;
			} else if (sscanf(var, "%*[\']%[^\']c", &tmp)) {
				*ptr = malloc(sizeof(Register));
				(*ptr)->data.vChar = tmp;
				(*ptr)->type = RegChar;
				(*ptr)->readOnly = 0;
				return ETC;
			} else return ERR;
		} else if (var[0] == '\"') {
			int len = strlen(var), repeat = 0;
			if (var[len - 1] != '\"') {
				char buf[4];
				if (sscanf(var, "%*[^*]%*[* ]%[^\n]s", &buf) == 0) {
					return ERR;
				}
				if (sscanf(buf, "%d", &repeat) == 0) {
					return ERR;
				}
			}
			*ptr = malloc(sizeof(Register));
			if (repeat == 0) {
				(*ptr)->data.vPtr = malloc(sizeof(char) * (len - 1));
				memcpy((*ptr)->data.vPtr, var + 1, len - 2);
				(*ptr)->data.vPtr[len - 2] = '\0';
			} else {
				char* buf = malloc(sizeof(char) * len);
				if(sscanf(var, "%*[\"]%[^\"]d", buf)) {
					int bufLen = strlen(buf);
					(*ptr)->data.vPtr = malloc(sizeof(char) * (bufLen * (repeat + 1)));
					(*ptr)->data.vPtr[0] = '\0';
					for (int i = 0; i < repeat; i++) {
						strcat((*ptr)->data.vPtr, buf);
					}
				} else return ERR;
			}
			(*ptr)->type = RegPtr;
			(*ptr)->readOnly = 1;
			return ETC;
		} else if (var[0] >= '0' && var[0] <= '9' || var[0] == '-' || var[0] == '+') {
			if (var[strlen(var) - 1] == 'F' || var[strlen(var) - 1] == 'f' || 
				strchr(var, '.') > 0) {
				float tmp = 0;
				if (sscanf(var, "%f", &tmp)) {
					*ptr = malloc(sizeof(Register));
					(*ptr)->data.vFloat = tmp;
					(*ptr)->type = RegFloat;
					(*ptr)->readOnly = 0;
					return ETC;
				} else return ERR;
			} else {
				int tmp = 0;
				if (var[1] == 'x' || var[1] == 'X' || 
				var[strlen(var) - 1] == 'h' || var[strlen(var) - 1] == 'H') {
					sscanf(var, "%x", &tmp);
				} else {
					sscanf(var, "%d", &tmp);
				}
				*ptr = malloc(sizeof(Register));
				(*ptr)->data.vInt = tmp;
				(*ptr)->type = RegInt;
				(*ptr)->readOnly = 0;
				return ETC;
			}
		} else {
			Register* r = inst->mm->get(inst->mm->p, var);
			if (r == 0) return ERR;
			*ptr = r;
			return OK;
		}
	}
}

int execute(Instance* inst, char* var, char type) {
	char head[32] = "\0", dst[32] = "\0", src[64] = "\0";
	if (type == 'd') {
		sscanf(var, "%s %[^ \t=] %*[= \t]%[^\n]", head, dst, src);
		int index = getSymbolIndex(funList, head);
		if (index == ETC) return ERR;
		if (verifyVarName(dst)) return ERR;
		Register dr;
		dr.data.vPtr = malloc(sizeof(char) * (strlen(dst) + 1));
		dr.type = RegPtr;
		strcpy(dr.data.vPtr, dst);
		Register* sr;
		if (getRegister(inst, src, &sr) == ETC) {
			if (funList[index].fun(inst, &dr, sr)) {
				return ERR;
			}
			free(dr.data.vPtr);
			free(sr);
		} else return ERR;
	} else if (type == 'c') {
		sscanf(var, "%s %[^ \t,] %*[, \t]%[^\n]", head, dst, src);
		int index = getSymbolIndex(funList, head);
		if (index == ETC) {
			return verifyTag(head);
		}
		Register* dr = 0; Register* sr = 0; int dresult = 0, sresult = 0;
		dresult = getRegister(inst, dst, &dr);
		if (dresult != OK) {
			if (dresult == ETC) {
				dr->readOnly = 1;
			} else if (index < FUN_NO_OPER_CNT) {
				dr = 0;
			} else {
				if (verifyTag(dst) == OK) {
					dr = malloc(sizeof(Register));
					dr->data.vPtr = malloc(sizeof(char) * (strlen(dst) + 1));
					dr->type = RegPtr;
					dr->readOnly = 1;
					strcpy(dr->data.vPtr, dst);
				} else return ERR;
			}
		}
		sresult = getRegister(inst, src, &sr);
		//if (sresult) return ERR;
		int result = funList[index].fun(inst, dr, sr);
		if (result == ERR) return ERR;
		if (result == ETC) return ETC;
		if (dr != 0) {
			if (dr->type == RegPtr) {
				if (verifyTag(dr->data.vPtr) == OK) {
					free(dr->data.vPtr);
					free(dr);
				}
			}
			if (dresult == ETC) {
				if (dr->type == RegPtr) free(dr->data.vPtr);
				free(dr);
			}
		}
		if (sresult == ETC) {
			if (sr->type == RegPtr) free(sr->data.vPtr);
			free(sr);
		}
	}
	return OK;
}

void console() {
	print("Now in console mode.\n");
	char buf[IOBUF]; int lines = 1, result = 0;
	Instance* instance = NewInstance(16, 32);

	while (1) {
		print("\n%d >>> ", lines);
		scan(buf);
		
		if (execute(instance, buf, 'd') == ERR) {
			result = execute(instance, buf, 'c');
			if (result == ERR) {
				print("\nNSASM running error!\n");
				print("At line %d: %s\n\n", lines, buf);
				break;
			} else if (result == ETC) {
				break;
			}
		}
		lines++;
	}
}

void run(char* var) {
	if (var == 0) return;
	char* conf = cut(var, ".conf");
	char* data = cut(var, ".data");
	char* code = cut(var, ".code");
	
	int stackSiz = 0, heapSiz = 0;
	if (conf != 0) {
		int confLines = lines(conf);
		char type[8] = "", value[8] = "";
		print("CONF: %d line(s), init...\n", confLines);
		for (int i = 0; i < confLines; i++) {
			sscanf(line(conf, i), "%s %[^\n]", type, value);
			if (strcmp(strlwr(type), "stack") == 0) {
				if (sscanf(value, "%d", &stackSiz) == 0) {
					print("\nNSASM init error!\n");
					print("At [CONF] line %d: %s\n\n", i + 1, line(data, i));
					return;
				}
			} else if (strcmp(strlwr(type), "heap") == 0) {
				if (sscanf(value, "%d", &heapSiz) == 0) {
					print("\nNSASM init error!\n");
					print("At [CONF] line %d: %s\n\n", i + 1, line(data, i));
					return;
				}
			} else {
				print("\nNSASM init error!\n");
				print("At [CONF] line %d: %s\n\n", i + 1, line(data, i));
				return;
			}
		}
	} else {
		stackSiz = 32;
		heapSiz = 96;
	}
	
	Instance* instance = NewInstance(stackSiz, heapSiz);
	
	if (data != 0) {
		int dataLines = lines(data);
		print("DATA: %d line(s), loading...\n", dataLines);
		for (int i = 0; i < dataLines; i++)
			if (execute(instance, line(data, i), 'd')) {
				print("\nNSASM running error!\n");
				print("At [DATA] line %d: %s\n\n", i + 1, line(data, i));
				return;
			}
	}
	
	if (code != 0) {
		int prev = 0, result = 0, codeLines = lines(code);
		print("CODE: %d line(s), running...\n\n", codeLines);
		for (; instance->cnt < codeLines; instance->cnt++) {
			prev = instance->cnt;
			result = execute(instance, line(code, instance->cnt), 'c');
			if (result == ETC) break;
			if (result) {
				print("\nNSASM running error!\n");
				print("At line %d: %s\n\n", prev + 1, line(code, prev));
				return;
			}
			for (int i = 0; i < codeLines; i++) {
				if (strcmp(line(code, i), instance->tag) == 0) {
					for (int j = i + 1; j < codeLines; j++) {
						if (strcmp(line(code, j), instance->tag) == 0) {
							print("\nNSASM running error!\n");
							print("At  [CODE] line %d: %s\n", i, line(code, i));
							print("And [CODE] line %d: %s\n\n", j, line(code, j));
							return;
						}
					}
					instance->cnt = i;
					instance->tag[0] = '\0';
				}
			}
			if (instance->cnt >= codeLines) {
				print("\nNSASM running error!\n");
				print("At [CODE] line %d: %s\n\n", prev + 1, line(code, prev));
				return;
			}
		}
	}
	
	FreeInstance(instance);
	free(conf); free(data); free(code);
	print("\nNSASM running finished.\n\n");
}

void call(char* var, Instance* prev) {
	char* raw = 0;
	raw = read(var);
	if (raw == 0) return;
	char* conf = cut(raw, ".conf");
	char* data = cut(raw, ".data");
	char* code = cut(raw, ".code");
	
	int stackSiz = 0, heapSiz = 0;
	if (conf != 0) {
		int confLines = lines(conf);
		char type[8] = "", value[8] = "";
		for (int i = 0; i < confLines; i++) {
			sscanf(line(conf, i), "%s %[^\n]", type, value);
			if (strcmp(strlwr(type), "stack") == 0) {
				if (sscanf(value, "%d", &stackSiz) == 0) {
					print("\nNSASM init error in \"%s\"!\n", var);
					print("At [CONF] line %d: %s\n\n", i + 1, line(data, i));
					return;
				}
			} else if (strcmp(strlwr(type), "heap") == 0) {
				if (sscanf(value, "%d", &heapSiz) == 0) {
					print("\nNSASM init error in \"%s\"!\n", var);
					print("At [CONF] line %d: %s\n\n", i + 1, line(data, i));
					return;
				}
			} else {
				print("\nNSASM init error in \"%s\"!\n", var);
				print("At [CONF] line %d: %s\n\n", i + 1, line(data, i));
				return;
			}
		}
	} else {
		stackSiz = 32;
		heapSiz = 96;
	}
	
	Instance* instance = NewInstance(stackSiz, heapSiz);
	memcpy(instance->reg, prev->reg, sizeof(Register) * REG_CNT);
	memcpy(&instance->state, &prev->state, sizeof(Register));
	
	if (data != 0) {
		int dataLines = lines(data);
		for (int i = 0; i < dataLines; i++)
			if (execute(instance, line(data, i), 'd')) {
				print("\nNSASM running error in \"%s\"!\n", var);
				print("At [DATA] line %d: %s\n\n", i + 1, line(data, i));
				return;
			}
	}
	
	if (code != 0) {
		int prev = 0, result = 0, codeLines = lines(code);
		for (; instance->cnt < codeLines; instance->cnt++) {
			prev = instance->cnt;
			result = execute(instance, line(code, instance->cnt), 'c');
			if (result == ETC) break;
			if (result) {
				print("\nNSASM running error in \"%s\"!\n", var);
				print("At line %d: %s\n\n", prev + 1, line(code, prev));
				return;
			}
			for (int i = 0; i < codeLines; i++) {
				if (strcmp(line(code, i), instance->tag) == 0) {
					for (int j = i + 1; j < codeLines; j++) {
						if (strcmp(line(code, j), instance->tag) == 0) {
							print("\nNSASM running error!\n");
							print("At  [CODE] line %d: %s\n", i, line(code, i));
							print("And [CODE] line %d: %s\n\n", j, line(code, j));
							return;
						}
					}
					instance->cnt = i;
					instance->tag[0] = '\0';
				}
			}
			if (instance->cnt >= codeLines) {
				print("\nNSASM running error in \"%s\"!\n", var);
				print("At [CODE] line %d: %s\n\n", prev + 1, line(code, prev));
				return;
			}
		}
	}
	
	FreeInstance(instance);
	free(conf); free(data); free(code); free(raw);
}

char* read(char* path) {
	FILE* f = fopen(path, "r");
	if (f == 0) {
		print("File open failed.\n");
		print("At file: %s\n\n", path);
		return OK;
	}
	int length = 0; char tmp;
	while (feof(f) == 0) {
		tmp = fgetc(f);
		if (tmp != '\r')
			length += 1;
	}
	fclose(f);
	f = fopen(path, "r");
	if (f == 0) {
		print("File open failed.\n");
		print("At file: %s\n\n", path);
		return OK;
	}
	char* data = malloc(sizeof(char) * (length + 1));
	length = 0;
	while (feof(f) == 0) {
		tmp = fgetc(f);
		if (tmp != '\r') {
			data[length] = tmp;
			length += 1;
		}
	}
	data[length] = '\0';
	return data;
}

int lines(char* src) {
	if(src == 0) return OK;
	int cnt = 0, length = strlen(src);
	for (int i = 0; i < length; i++)
		if (src[i] == '\n') cnt += 1;
	return cnt;
}

char* line(char* src, int index) {
	if (index >= lines(src)) return OK;
	int srcLen = strlen(src), cnt = 0, pos = 0;
	char* buf = malloc(sizeof(char) * srcLen);
	char* result = 0;
	for (int i = 0; i < srcLen; i++) {
		if (index == 0) {
			for (i = 0; src[i] != '\n'; i++)
				buf[i] = src[i];
			pos = i + 1;
			result = malloc(sizeof(char) * (pos));
			for (i = 0; i < pos; i++) {
				if (i == pos - 1) {
					result[i] = '\0';
					break;
				}
				result[i] = buf[i];
			}
			free(buf);
			return result;
		}
		if (index == cnt) {
			pos = i;
			for (; src[i] != '\n'; i++)
				buf[i - pos] = src[i];
			pos = i - pos + 1;
			result = malloc(sizeof(char) * pos);
			for (i = 0; i < pos; i++) {
				if (i == pos - 1) {
					result[i] = '\0';
					break;
				}
				result[i] = buf[i];
			}
			free(buf);
			return result;
		}
		if (src[i] == '\n') cnt += 1;
	}
	return OK;
}

char* cut(char* src, const char* head) {
	int srcLen = strlen(src), headLen = strlen(head) + 1;
	char* headBuf = malloc(sizeof(char) * headLen);
	char* bodyBuf = malloc(sizeof(char) * srcLen);
	int start, size, cnt; char* buf = 0; char* blk = 0;
	for (int i = 0; i < srcLen - headLen; i++) {
		if (strcmp(get(src, i, headBuf, headLen), head) == 0) {
			for (; i < srcLen; i++) {
				if (src[i] == '{') {
					start = i += 1;
					for (; i < srcLen; i++) {
						if (src[i] == '}') break;
						if (i == srcLen) return OK;
						bodyBuf[i - start] = src[i];
					}
					break;
				}
			}
			size = i - start + 1;
			buf = malloc(sizeof(char) * size);
			for (cnt = i = 0; i < size; i++) {
				if (i == size - 1) {
					buf[i] = '\0';
					break;
				}
				if (i == 0 && bodyBuf[i] == '\n') {
					cnt += 1;
					continue;
				}
				if (bodyBuf[i] == ' ' && bodyBuf[i - 1] == ' ') {
					cnt += 1;
					continue;
				}
				if (bodyBuf[i] == '\n' && bodyBuf[i - 1] == '\n') {
					cnt += 1;
					continue;
				}
				if (bodyBuf[i] == '\n' && bodyBuf[i - 1] == '\t') {
					cnt += 1;
					continue;
				}
				if (bodyBuf[i] == '\t' && bodyBuf[i - 1] == '\t') {
					cnt += 1;
					continue;
				}
				if (bodyBuf[i] == '\t' && bodyBuf[i - 1] == '\n') {
					cnt += 1;
					continue;
				}
				buf[i - cnt] = bodyBuf[i];
			}
			size -= cnt;
			blk = malloc(sizeof(char) * size);
			for (i = 0; i < size; i++) {
				if (i == size - 1) {
					blk[i] = '\0';
					break;
				}
				blk[i] = buf[i];
			}
			free(buf);
			free(bodyBuf);
			free(headBuf);
			return blk;
		}
	}
	return OK;
}

char* get(char* src, int start, char* buf, int size) {
	for (int i = 0; i < size; i++) {
		if (i == size - 1) {
			buf[i] = '\0';
			break;
		}
		buf[i] = src[start + i];
	}
	return buf;
}
