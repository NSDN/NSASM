#include "memman.h"

#include "base.h"

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
