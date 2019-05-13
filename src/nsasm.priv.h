#ifndef __NSASM_PRIV_H_
#define __NSASM_PRIV_H_


#include "nsasm.pub.h"

#include "memman.h"

#define REG_CNT 8

typedef struct {
	MemoryManager* mm;
	Register reg[REG_CNT];
	Register state;
	char tag[32];
	int cnt;
} Instance;

Instance* NewInstance(int stackSiz, int heapSiz);
void FreeInstance(Instance* ptr);

typedef struct {
	char name[8];
	int (*fun)(Instance* inst, Register* dst, Register* src);
} Function;

int getNoperCount();
Function* getFuncList();

int getSymbolIndex(Function list[], char* var);
int verifyVarName(char* var);
int verifyTag(char* var);
int getRegister(Instance* inst, char* var, Register** ptr);

int execute(Instance* inst, char* var, char type);
void console();
void run(char* var);
void call(char* var, Instance* prev);


#endif
