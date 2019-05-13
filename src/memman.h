#ifndef __BASE_MM_H_
#define __BASE_MM_H_


#include "nsasm.pub.h"

#ifndef OK
#define OK 0
#endif

#ifndef ERR
#define ERR 1
#endif

#ifndef ETC
#define ETC -1
#endif

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

MemoryManager* InitMemoryManager(int stackSize, int heapSize);
void DisposeMemoryManager(MemoryManager* ptr);


#endif
