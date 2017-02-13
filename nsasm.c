/* ---- DEFINE ---- */
//#define WINDOWS
#define LINUX
//#define ARDUINO
//#define STM32
/* ---- DEFINE ---- */

#if defined(WINDOWS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <string.h>

int nsasm(int argc, char* argv[]);

#if defined(WINDOWS)
#include <stdlib.h>
#include <vadefs.h>
#define clear() system("cls")
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
	result = nsasm(argc, argv);
	system("pause");
	return result;
}

#elif defined(LINUX)
#include <stdlib.h>
#include <stdarg.h>
#define clear() system("clear")
int print(const char* format, ...) {
	va_list args = 0;
	va_start(args, format);
	int result = vprintf(format, args);
	va_end(args);
	return result;
}
char* scan(char* buffer) { return gets(buffer); }
int fscan(char* buffer, const char* format, ...) {
	gets(buffer);
	va_list args;
	va_start(args, format);
	int result = vsscanf(buffer, format, args);
	va_end(args);
	return result;
}

int main(int argc, char* argv[]) {
	int result;
	result = nsasm(argc, argv);
	return result;
}

#elif defined(ARDUINO)
#include <stdarg.h>
#define IOBUF 128
#define BACKCOLOR 0x0000
#define clear() { tft.setCursor(0, 0); tft.fillScreen(BACKCOLOR); }
int print(const char* format, ...) {
	char* iobuf = malloc(sizeof(char) * IOBUF);
	va_list args;
	va_start(args, format);
	int result = vsprintf(iobuf, format, args);
	va_end(args);
	tft.print(iobuf);
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
				tft.print((char)0x08);
				continue;
			}
			else if (buffer[count] != 0x08) {
				tft.print(buffer[count]);
				count += 1;
			}
		}
		else if (Serial1.available() > 0) {
			if (Serial1.peek() == '\n') break;
			buffer[count] = Serial1.read();
			if (buffer[count] == 0x08 && count > 0) {
				count -= 1;
				tft.print((char)0x08);
				continue;
			}
			else if (buffer[count] != 0x08) {
				tft.print(buffer[count]);
				count += 1;
			}
		}
	}
	buffer[count] = '\0';
	if (Serial.available() > 0) Serial.read();
	else if (Serial1.available() > 0) Serial1.read();
	tft.println();
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
#define IOBUF 128
#define BACKCOLOR 0x000000
#define clear() { tft.setCursor(0, 0); tft.fillScreen(BACKCOLOR); }
int print(const char* format, ...) {
	char* iobuf = malloc(sizeof(char) * IOBUF);
	va_list args;
	va_start(args, format);
	int result = vsprintf(iobuf, format, args);
	va_end(args);
	tft.print(iobuf);
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
				tft.print((char)0x08);
				continue;
			}
			else if (buffer[count] != 0x08) {
				tft.print(buffer[count]);
				count += 1;
			}
		}
		else if (Serial1.available() > 0) {
			if (Serial1.peek() == '\n') break;
			buffer[count] = Serial1.read();
			if (buffer[count] == 0x08 && count > 0) {
				count -= 1;
				tft.print((char)0x08);
				continue;
			}
			else if (buffer[count] != 0x08) {
				tft.print(buffer[count]);
				count += 1;
			}
		}
	}
	buffer[count] = '\0';
	if (Serial.available() > 0) Serial.read();
	else if (Serial1.available() > 0) Serial1.read();
	tft.println();
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

#define VERSION 0.1

typedef enum {
	RegChar,
    RegInt,
    RegFloat,
    RegPtr
} RegType;

typedef struct {
    RegType type;
    char vChar;
    int vInt;
    float vFloat;
    void* vPtr;
} Register;

#define REG_CNT 8
Register reg[REG_CNT];
Register* state;
char tagBuf[32];
int cnt;

/* -------------------------------- */

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
	if (p->stackCnt >= p->stackSiz) return 1;
	memcpy(&(p->stackTop->var), blk, sizeof(MMBlock));
	p->stackCnt += 1;
	p->stackTop->next = 0;
	return 0;
}

int _mm_pop(pMM* p, MMBlock* blk) {
	if (p->stackTop->prev == 0) {
		memcpy(blk, &(p->stackTop->var), sizeof(MMBlock));
		free(p->stackTop);
		p->stackTop = 0;
		p->stackCnt = 0;
		return 0;
	}
	if (p->stackTop == 0) return 1;
	memcpy(blk, &(p->stackTop->var), sizeof(MMBlock));
	p->stackCnt -= 1;
	p->stackTop = p->stackTop->prev;
	free(p->stackTop->next);
	return 0;
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
	if (p->heapCnt >= p->heapSiz) return 1;
	p->heapEnd->name = malloc(sizeof(char) * (strlen(name) + 1));
	strcpy(p->heapEnd->name, name);
	memcpy(&(p->heapEnd->var), blk, sizeof(MMBlock));
	p->heapCnt += 1;
	p->heapEnd->next = 0;
	return 0;
}

MMType* _mm_search(pMM* p, char* name) {
	MMType* lp = p->heapStart;
	MMType* rp = p->heapEnd;
	do {
		if (strcmp(lp->name, name) == 0) return lp;
		else lp = lp->next;
		if (strcmp(rp->name, name) == 0) return rp;
		else rp = rp->prev;
	} while (
		((p->heapCnt % 2 == 1) && lp != rp) || (
			(p->heapCnt % 2 == 0) && (lp != rp->next) && (rp != lp->prev)
			)
		);
	if (lp == rp) {
		if (strcmp(lp->name, name) == 0) return lp;
	}
	return 0;
}

MMBlock* _mm_get(pMM* p, char* name) {
	MMType* sp = 0;
	sp = _mm_search(p, name);
	if (sp == 0) return 0;
	return &(sp->var);
}

int _mm_exit(pMM* p, char* name) {
	MMType* sp = 0;
	sp = _mm_search(p, name);
	if (sp == 0) return 1;
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
	return 0;
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

MemoryManager* mm;

/* -------------------------------- */

int lines(char* src);
char* line(char* src, int index);
char* cut(char* src, const char* head);
char* get(char* src, int start, char* buf, int size);

int jump(char* src, char* tag);
int execute(char* var, char type);
void compile(char* var);
void run(char* var);

char* read(char* path) {
	FILE* f = fopen(path, "r");
	if (f == 0) {
		print("Error: File open failed.\n\n");
		return 0;
	}
	int length = 0;
	while (feof(f) == 0) {
		fgetc(f);
		length += 1;
	}
	fclose(f);
	f = fopen(path, "r");
	if (f == 0) {
		print("Error: File open failed.\n\n");
		return 0;
	}
	char* data = malloc(sizeof(char) * (length + 1));
	length = 0;
	while (feof(f) == 0) {
		data[length] = fgetc(f);
		length += 1;
	}
	data[length] = '\0';
	return data;
}

int nsasm(int argc, char* argv[]) {
    print("NyaSama Assembly Script Module\n");
    print("Version: %1.2f\n\n", VERSION);
    if (argc < 2) {
        print("Usage: nasm [c/r] [FILE]\n\n");
        return 0;
    } else {
        if (argc == 3) {
            if (strchr(argv[1], 'c') > 0) {
				compile(read(argv[2]));
                return 0;
            }
			if (strchr(argv[1], 'r') > 0) {
				run(read(argv[2]));
				return 0;
			}
        }
		run(read(argv[1]));
        return 0;
    }
}

int execute(char* var, char type) {
	char head[32] = "\0", dst[32] = "\0", src[64] = "\0";
	if (type == 'd') {
		sscanf(var, "%s %[^ \t=] %*[= \t]%[^\n]", head, dst, src);
		if (strcmp(strlwr(head), "var") == 0) {
			if (
				(dst[0] >= '0' && dst[0] <= '9') || 
				dst[0] == 'r' || dst[0] == 'R' || 
				dst[0] == '-' || dst[0] == '+' || 
				dst[0] == '.' || dst[0] == '['
			) return 1; 
			Register r;
			if (src[0] == '\'') {
				if (src[strlen(src) - 1] != '\'') return 1;
				char tmp = 0;
				if(sscanf(src, "%*[\']%[^\']c", &tmp)) {
					r.vChar = tmp;
					r.type = RegChar;
				} else {
					return 1;
				}
			} else if (src[0] >= '0' && src[0] <= '9' || src[0] == '-' || src[0] == '+') {
				if (src[strlen(src) - 1] == 'F' || src[strlen(src) - 1] == 'f' || 
					strchr(src, '.') > 0) {
					float tmp = 0;
					if (sscanf(src, "%f", &tmp)) {
						r.vFloat = tmp;
						r.type = RegFloat;
					} else {
						return 1;
					}
				} else {
					int tmp = 0;
					if (src[1] == 'x' || src[1] == 'X' || 
					src[strlen(src) - 1] == 'h' || src[strlen(src) - 1] == 'H') {
						sscanf(src, "%x", &tmp);
					} else {
						sscanf(src, "%d", &tmp);
					}
					r.vInt = tmp;
					r.type = RegInt;
				}
			} else if (src[0] == '\"') {
				int len = strlen(src);
				if (src[len - 1] != '\"') return 1;
				r.vPtr = malloc(sizeof(char) * (len - 1));
				memcpy(r.vPtr, src + 1, len - 2);
				r.type = RegPtr;
			}
			mm->join(mm->p, dst, &r);
		} else if (strcmp(strlwr(head), "int") == 0) {
			
		} else if (strcmp(strlwr(head), "char") == 0) {
			
		} else if (strcmp(strlwr(head), "float") == 0) {
			
		} else if (strcmp(strlwr(head), "str") == 0) {
			
		} else {
			return 1;
		}
	} else if (type == 'c') {
		sscanf(var, "%s %[^ \t,] %*[, \t]%[^\n]", head, dst, src);
		if (strcmp(strlwr(head), "mov") == 0) {
			if (dst[0] == 'r' || dst[0] == 'R') {
				int drn = -1;
				sscanf(dst, "%*[rR]%d", &drn);
				if (drn >= 0 && drn < REG_CNT) {
					if (src[0] == 'r' || src[0] == 'R') {
						int srn = -1;
						sscanf(src, "%*[rR]%d", &srn);
						if (srn >= 0 && srn < REG_CNT) {
							switch (reg[srn].type) {
								case RegChar:
									reg[drn].vChar = reg[srn].vChar;
									reg[drn].type = RegChar;
									break;
								case RegFloat:
									reg[drn].vFloat = reg[srn].vFloat;
									reg[drn].type = RegFloat;
									break;
								case RegInt:
									reg[drn].vInt = reg[srn].vInt;
									reg[drn].type = RegInt;
									break;
								case RegPtr:
									reg[drn].vPtr = reg[srn].vPtr;
									reg[drn].type = RegPtr;
									break;
								default:
									return 1;
							}
						} else return 1;
					} else {
						if (src[0] == '\'') {
							if (src[strlen(src) - 1] != '\'') return 1;
							char tmp = 0;
							if(sscanf(src, "%*[\']%[^\']c", &tmp)) {
								reg[drn].vChar = tmp;
								reg[drn].type = RegChar;
							} else {
								return 1;
							}
						} else if (src[0] >= '0' && src[0] <= '9' || src[0] == '-' || src[0] == '+') {
							if (src[strlen(src) - 1] == 'F' || src[strlen(src) - 1] == 'f' || 
								strchr(src, '.') > 0) {
								float tmp = 0;
								if (sscanf(src, "%f", &tmp)) {
									reg[drn].vFloat = tmp;
									reg[drn].type = RegFloat;
								} else {
									return 1;
								}
							} else {
								int tmp = 0;
								if (src[1] == 'x' || src[1] == 'X' || 
								src[strlen(src) - 1] == 'h' || src[strlen(src) - 1] == 'H') {
									sscanf(src, "%x", &tmp);
								} else {
									sscanf(src, "%d", &tmp);
								}
								reg[drn].vInt = tmp;
								reg[drn].type = RegInt;
							}
						} else {
							Register* r = mm->get(mm->p, src);
							if (r == 0) return 1;
							switch (r->type) {
								case RegChar:
									reg[drn].vChar = r->vChar;
									reg[drn].type = RegChar;
									break;
								case RegFloat:
									reg[drn].vFloat = r->vFloat;
									reg[drn].type = RegFloat;
									break;
								case RegInt:
									reg[drn].vInt = r->vInt;
									reg[drn].type = RegInt;
									break;
								case RegPtr:
									reg[drn].vPtr = r->vPtr;
									reg[drn].type = RegPtr;
									break;
								default:
									return 1;
							}
						}
					}
				} else return 1;
			} else {
				Register* dr = mm->get(mm->p, dst);
				if (dr == 0) return 1;
				if (src[0] == 'r' || src[0] == 'R') {
					int srn = -1;
					sscanf(src, "%*[rR]%d", &srn);
					if (srn >= 0 && srn < REG_CNT) {
						switch (reg[srn].type) {
							case RegChar:
								dr->vChar = reg[srn].vChar;
								dr->type = RegChar;
								break;
							case RegFloat:
								dr->vFloat = reg[srn].vFloat;
								dr->type = RegFloat;
								break;
							case RegInt:
								dr->vInt = reg[srn].vInt;
								dr->type = RegInt;
								break;
							case RegPtr:
								dr->vPtr = reg[srn].vPtr;
								dr->type = RegPtr;
								break;
							default:
								return 1;
						}
					} else return 1;
				} else {
					if (src[0] == '\'') {
						if (src[strlen(src) - 1] != '\'') return 1;
						char tmp = 0;
						if(sscanf(src, "%*[\']%[^\']c", &tmp)) {
							dr->vChar = tmp;
							dr->type = RegChar;
						} else {
							return 1;
						}
					} else if (src[0] >= '0' && src[0] <= '9' || src[0] == '-' || src[0] == '+') {
						if (src[strlen(src) - 1] == 'F' || src[strlen(src) - 1] == 'f' || 
							strchr(src, '.') > 0) {
							float tmp = 0;
							if (sscanf(src, "%f", &tmp)) {
								dr->vFloat = tmp;
								dr->type = RegFloat;
							} else {
								return 1;
							}
						} else {
							int tmp = 0;
							if (src[1] == 'x' || src[1] == 'X' || 
							src[strlen(src) - 1] == 'h' || src[strlen(src) - 1] == 'H') {
								sscanf(src, "%x", &tmp);
							} else {
								sscanf(src, "%d", &tmp);
							}
							dr->vInt = tmp;
							dr->type = RegInt;
						}
					} else {
						Register* r = mm->get(mm->p, src);
						if (r == 0) return 1;
						switch (r->type) {
							case RegChar:
								dr->vChar = r->vChar;
								dr->type = RegChar;
								break;
							case RegFloat:
								dr->vFloat = r->vFloat;
								dr->type = RegFloat;
								break;
							case RegInt:
								dr->vInt = r->vInt;
								dr->type = RegInt;
								break;
							case RegPtr:
								dr->vPtr = r->vPtr;
								dr->type = RegPtr;
								break;
							default:
								return 1;
						}
					}
				}
			}
		} else if (strcmp(strlwr(head), "push") == 0) {
			if (src[0] == 'r' || src[0] == 'R') {
				int srn = -1;
				sscanf(src, "%*[rR]%d", &srn);
				if (srn >= 0 && srn < REG_CNT) {
					mm->push(mm->p, &reg[srn]);
				} else return 1;
			} else {
				Register* r = mm->get(mm->p, src);
				if (r == 0) return 1;
				mm->push(mm->p, r);
			}
		} else if (strcmp(strlwr(head), "pop") == 0) {
			if (src[0] == 'r' || src[0] == 'R') {
				int srn = -1;
				sscanf(src, "%*[rR]%d", &srn);
				if (srn >= 0 && srn < REG_CNT) {
					mm->pop(mm->p, &reg[srn]);
				} else return 1;
			} else {
				Register* r = mm->get(mm->p, src);
				if (r == 0) return 1;
				mm->pop(mm->p, r);
			}
		} else if (strcmp(strlwr(head), "in") == 0) {
			
		} else if (strcmp(strlwr(head), "out") == 0) {
			int addr = -1;
			if (dst[1] == 'x' || dst[1] == 'X' || 
			dst[strlen(dst) - 1] == 'h' || dst[strlen(dst) - 1] == 'H') {
				sscanf(dst, "%x", &addr);
			} else {
				sscanf(dst, "%d", &addr);
			}
			switch (addr) {
				case 0x00:
					if (src[0] == '\"' || src[0] == '\'') {
						int len = strlen(src);
						if (src[len - 1] != '\"' && 
							src[len - 1] != '\'') return 1;
						for (int i = 1; i < len - 1; i++) {
							if (src[i] == '\\') {
								if (src[i + 1] == 'n') { print("\n"); i++; }
								else if (src[i + 1] == 't') { print("\t"); i++; }
								else if (src[i + 1] == 'r') { print("\r"); i++; }
								else if (src[i + 1] == '\\') { print("\\"); i++; }
								else print("%c", src[i]);
							} else 
								print("%c", src[i]);
						}	
					} else if (src[0] == 'r' || src[0] == 'R') {
						int srn = 0;
						sscanf(src, "%*[rR]%d", &srn);
						if (srn >= 0 && srn < REG_CNT) {
							switch (reg[srn].type) {
								case RegChar:
									print("%c", reg[srn].vChar);
									break;
								case RegFloat:
									print("%f", reg[srn].vFloat);
									break;
								case RegInt:
									print("%d", reg[srn].vInt);
									break;
								case RegPtr:
									print("%s", reg[srn].vPtr);
									break;
								default:
									return 1;
							}
						} else return 1;
					} else {
						if (src[0] == '\'') {
							if (src[strlen(src) - 1] != '\'') return 1;
							char tmp = 0;
							if(sscanf(src, "%*[\']%[^\']c", &tmp)) {
								print("%c", tmp);
							} else {
								return 1;
							}
						} else if (src[0] >= '0' && src[0] <= '9' || src[0] == '-' || src[0] == '+') {
							if (src[strlen(src) - 1] == 'F' || src[strlen(src) - 1] == 'f' || 
								strchr(src, '.') > 0) {
								float tmp = 0;
								if (sscanf(src, "%f", &tmp)) {
									print("%f", tmp);
								} else {
									return 1;
								}
							} else {
								int tmp = 0;
								if (src[1] == 'x' || src[1] == 'X' || 
								src[strlen(src) - 1] == 'h' || src[strlen(src) - 1] == 'H') {
									sscanf(src, "%x", &tmp);
								} else {
									sscanf(src, "%d", &tmp);
								}
								print("%d", tmp);
							}
						} else {
							Register* r = mm->get(mm->p, src);
							if (r == 0) return 1;
							switch (r->type) {
								case RegChar:
									print("%c", r->vChar);
									break;
								case RegFloat:
									print("%f", r->vFloat);
									break;
								case RegInt:
									print("%d", r->vInt);
									break;
								case RegPtr:
									print("%s", r->vPtr);
									break;
								default:
									return 1;
							}
						}
					}
					break;
				case 0xFF:
					if (src[0] == '\"' || src[0] == '\'') {
						int len = strlen(src);
						if (src[len - 1] != '\"' && 
							src[len - 1] != '\'') return 1;
						print("[DEBUG] ");
						for (int i = 1; i < len - 1; i++) {
							if (src[i] == '\\') {
								if (src[i + 1] == 'n') { print("\n"); i++; }
								else if (src[i + 1] == 't') { print("\t"); i++; }
								else if (src[i + 1] == 'r') { print("\r"); i++; }
								else if (src[i + 1] == '\\') { print("\\"); i++; }
								else print("%c", src[i]);
							} else 
								print("%c", src[i]);
						}	
					} else if (src[0] == 'r' || src[0] == 'R') {
						int srn = 0;
						sscanf(src, "%*[rR]%d", &srn);
						if (srn >= 0 && srn < REG_CNT) {
							switch (reg[srn].type) {
								case RegChar:
									print("[DEBUG] ");
									print("%c", reg[srn].vChar);
									break;
								case RegFloat:
									print("[DEBUG] ");
									print("%f", reg[srn].vFloat);
									break;
								case RegInt:
									print("[DEBUG] ");
									print("%d", reg[srn].vInt);
									break;
								case RegPtr:
									print("[DEBUG] ");
									print("%s", reg[srn].vPtr);
									break;
								default:
									return 1;
							}
						} else return 1;
					} else {
						if (src[0] == '\'') {
							if (src[strlen(src) - 1] != '\'') return 1;
							char tmp = 0;
							if(sscanf(src, "%*[\']%[^\']c", &tmp)) {
								print("[DEBUG] ");
								print("%c", tmp);
							} else {
								return 1;
							}
						} else if (src[0] >= '0' && src[0] <= '9' || src[0] == '-' || src[0] == '+') {
							if (src[strlen(src) - 1] == 'F' || src[strlen(src) - 1] == 'f' || 
								strchr(src, '.') > 0) {
								float tmp = 0;
								if (sscanf(src, "%f", &tmp)) {
									print("[DEBUG] ");
									print("%f", tmp);
								} else {
									return 1;
								}
							} else {
								int tmp = 0;
								if (src[1] == 'x' || src[1] == 'X' || 
								src[strlen(src) - 1] == 'h' || src[strlen(src) - 1] == 'H') {
									sscanf(src, "%x", &tmp);
								} else {
									sscanf(src, "%d", &tmp);
								}
								print("[DEBUG] ");
								print("%d", tmp);
							}
						} else {
							Register* r = mm->get(mm->p, src);
							if (r == 0) return 1;
							switch (r->type) {
								case RegChar:
									print("[DEBUG] ");
									print("%c", r->vChar);
									break;
								case RegFloat:
									print("[DEBUG] ");
									print("%f", r->vFloat);
									break;
								case RegInt:
									print("[DEBUG] ");
									print("%d", r->vInt);
									break;
								case RegPtr:
									print("[DEBUG] ");
									print("%s", r->vPtr);
									break;
								default:
									return 1;
							}
						}
					}
					break;
				default:
					return 1;
			}
		} else if (strcmp(strlwr(head), "add") == 0) {
			if (dst[0] == 'r' || dst[0] == 'R') {
				int drn = -1;
				sscanf(dst, "%*[rR]%d", &drn);
				if (drn >= 0 && drn < REG_CNT) {
					if (src[0] == 'r' || src[0] == 'R') {
						int srn = -1;
						sscanf(src, "%*[rR]%d", &srn);
						if (srn >= 0 && srn < REG_CNT) {
							switch (reg[drn].type) {
								case RegChar:
									switch (reg[srn].type) {
										case RegChar:
											reg[drn].vChar += (char) reg[srn].vChar;
											break;
										case RegFloat:
											reg[drn].vChar += (char) reg[srn].vFloat;
											break;
										case RegInt:
											reg[drn].vChar += (char) reg[srn].vInt;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegFloat:
									switch (reg[srn].type) {
										case RegChar:
											reg[drn].vFloat += (float) reg[srn].vChar;
											break;
										case RegFloat:
											reg[drn].vFloat += (float) reg[srn].vFloat;
											break;
										case RegInt:
											reg[drn].vFloat += (float) reg[srn].vInt;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegInt:
									switch (reg[srn].type) {
										case RegChar:
											reg[drn].vInt += (int) reg[srn].vChar;
											break;
										case RegFloat:
											reg[drn].vInt += (int) reg[srn].vFloat;
											break;
										case RegInt:
											reg[drn].vInt += (int) reg[srn].vInt;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegPtr:
								#ifndef WINDOWS
									switch (reg[srn].type) {
										case RegChar:
											reg[drn].vPtr += (int) reg[srn].vChar;
											break;
										case RegFloat:
											return 1;
										case RegInt:
											reg[drn].vPtr += (int) reg[srn].vInt;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
								#endif
									break;
								default:
									return 1;
							}
						} else return 1;
					} else {
						if (src[0] == '\'') {
							if (src[strlen(src) - 1] != '\'') return 1;
							char tmp = 0;
							if (sscanf(src, "%*[\']%[^\']c", &tmp)) {
								switch (reg[drn].type) {
									case RegChar:
										reg[drn].vChar += (char) tmp;
										break;
									case RegFloat:
										reg[drn].vFloat += (float) tmp;
										break;
									case RegInt:
										reg[drn].vInt += (int) tmp;
										break;
									case RegPtr:
									#ifndef WINDOWS
										reg[drn].vPtr += (int) tmp;
									#endif
										break;
									default:
										return 1;
								}
							} else {
								return 1;
							}
						} else if (src[0] >= '0' && src[0] <= '9' || src[0] == '-' || src[0] == '+') {
							if (src[strlen(src) - 1] == 'F' || src[strlen(src) - 1] == 'f' || 
								strchr(src, '.') > 0) {
								float tmp = 0;
								if (sscanf(src, "%f", &tmp)) {
									switch (reg[drn].type) {
										case RegChar:
											reg[drn].vChar += (char) tmp;
											break;
										case RegFloat:
											reg[drn].vFloat += (float) tmp;
											break;
										case RegInt:
											reg[drn].vInt += (int) tmp;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
								} else {
									return 1;
								}
							} else {
								int tmp = 0;
								if (src[1] == 'x' || src[1] == 'X' || 
								src[strlen(src) - 1] == 'h' || src[strlen(src) - 1] == 'H') {
									sscanf(src, "%x", &tmp);
								} else {
									sscanf(src, "%d", &tmp);
								}
								switch (reg[drn].type) {
									case RegChar:
										reg[drn].vChar += (char) tmp;
										break;
									case RegFloat:
										reg[drn].vFloat += (float) tmp;
										break;
									case RegInt:
										reg[drn].vInt += (int) tmp;
										break;
									case RegPtr:
									#ifndef WINDOWS
										reg[drn].vPtr += (int) tmp;
									#endif
										break;
									default:
										return 1;
								}
							}
						} else {
							Register* r = mm->get(mm->p, src);
							if (r == 0) return 1;
							switch (r->type) {
								case RegChar:
									switch (reg[drn].type) {
										case RegChar:
											reg[drn].vChar += (char) r->vChar;
											break;
										case RegFloat:
											reg[drn].vFloat += (float) r->vChar;
											break;
										case RegInt:
											reg[drn].vInt += (int) r->vChar;
											break;
										case RegPtr:
										#ifndef WINDOWS
											reg[drn].vPtr += (int) r->vChar;
										#endif
											break;
										default:
											return 1;
									}
									break;
								case RegFloat:
									switch (reg[drn].type) {
										case RegChar:
											reg[drn].vChar += (char) r->vFloat;
											break;
										case RegFloat:
											reg[drn].vFloat += (float) r->vFloat;
											break;
										case RegInt:
											reg[drn].vInt += (int) r->vFloat;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegInt:
									switch (reg[drn].type) {
										case RegChar:
											reg[drn].vChar += (char) r->vInt;
											break;
										case RegFloat:
											reg[drn].vFloat += (float) r->vInt;
											break;
										case RegInt:
											reg[drn].vInt += (int) r->vInt;
											break;
										case RegPtr:
										#ifndef WINDOWS
											reg[drn].vPtr += (int) r->vInt;
										#endif
											break;
										default:
											return 1;
									}
									break;
								case RegPtr:
									return 1;
								default:
									return 1;
							}
						}
					}
				} else return 1;
			} else {
				Register* dr = mm->get(mm->p, dst);
				if (dr == 0) return 1;
				if (src[0] == 'r' || src[0] == 'R') {
					int srn = -1;
					sscanf(src, "%*[rR]%d", &srn);
					if (srn >= 0 && srn < REG_CNT) {
						switch (dr->type) {
							case RegChar:
								switch (reg[srn].type) {
									case RegChar:
										dr->vChar += (char) reg[srn].vChar;
										break;
									case RegFloat:
										dr->vChar += (char) reg[srn].vFloat;
										break;
									case RegInt:
										dr->vChar += (char) reg[srn].vInt;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegFloat:
								switch (reg[srn].type) {
									case RegChar:
										dr->vFloat += (float) reg[srn].vChar;
										break;
									case RegFloat:
										dr->vFloat += (float) reg[srn].vFloat;
										break;
									case RegInt:
										dr->vFloat += (float) reg[srn].vInt;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegInt:
								switch (reg[srn].type) {
									case RegChar:
										dr->vInt += (int) reg[srn].vChar;
										break;
									case RegFloat:
										dr->vInt += (int) reg[srn].vFloat;
										break;
									case RegInt:
										dr->vInt += (int) reg[srn].vInt;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegPtr:
							#ifndef WINDOWS
								switch (reg[srn].type) {
									case RegChar:
										dr->vPtr += (int) reg[srn].vChar;
										break;
									case RegFloat:
										return 1;
									case RegInt:
										dr->vPtr += (int) reg[srn].vInt;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
							#endif
								break;
							default:
								return 1;
						}
					} else return 1;
				} else {
					if (src[0] == '\'') {
						if (src[strlen(src) - 1] != '\'') return 1;
						char tmp = 0;
						if (sscanf(src, "%*[\']%[^\']c", &tmp)) {
							switch (dr->type) {
								case RegChar:
									dr->vChar += (char) tmp;
									break;
								case RegFloat:
									dr->vFloat += (float) tmp;
									break;
								case RegInt:
									dr->vInt += (int) tmp;
									break;
								case RegPtr:
								#ifndef WINDOWS
									dr->vPtr += (int) tmp;
								#endif
									break;
								default:
									return 1;
							}
						} else {
							return 1;
						}
					} else if (src[0] >= '0' && src[0] <= '9' || src[0] == '-' || src[0] == '+') {
						if (src[strlen(src) - 1] == 'F' || src[strlen(src) - 1] == 'f' || 
							strchr(src, '.') > 0) {
							float tmp = 0;
							if (sscanf(src, "%f", &tmp)) {
								switch (dr->type) {
									case RegChar:
										dr->vChar += (char) tmp;
										break;
									case RegFloat:
										dr->vFloat += (float) tmp;
										break;
									case RegInt:
										dr->vInt += (int) tmp;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
							} else {
								return 1;
							}
						} else {
							int tmp = 0;
							if (src[1] == 'x' || src[1] == 'X' || 
							src[strlen(src) - 1] == 'h' || src[strlen(src) - 1] == 'H') {
								sscanf(src, "%x", &tmp);
							} else {
								sscanf(src, "%d", &tmp);
							}
							switch (dr->type) {
								case RegChar:
									dr->vChar += (char) tmp;
									break;
								case RegFloat:
									dr->vFloat += (float) tmp;
									break;
								case RegInt:
									dr->vInt += (int) tmp;
									break;
								case RegPtr:
								#ifndef WINDOWS
									dr->vPtr += (int) tmp;
								#endif
									break;
								default:
									return 1;
							}
						}
					} else {
						Register* r = mm->get(mm->p, src);
						if (r == 0) return 1;
						switch (r->type) {
							case RegChar:
								switch (dr->type) {
									case RegChar:
										dr->vChar += (char) r->vChar;
										break;
									case RegFloat:
										dr->vFloat += (float) r->vChar;
										break;
									case RegInt:
										dr->vInt += (int) r->vChar;
										break;
									case RegPtr:
									#ifndef WINDOWS
										dr->vPtr += (int) r->vChar;
									#endif
										break;
									default:
										return 1;
								}
								break;
							case RegFloat:
								switch (dr->type) {
									case RegChar:
										dr->vChar += (char) r->vFloat;
										break;
									case RegFloat:
										dr->vFloat += (float) r->vFloat;
										break;
									case RegInt:
										dr->vInt += (int) r->vFloat;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegInt:
								switch (dr->type) {
									case RegChar:
										dr->vChar += (char) r->vInt;
										break;
									case RegFloat:
										dr->vFloat += (float) r->vInt;
										break;
									case RegInt:
										dr->vInt += (int) r->vInt;
										break;
									case RegPtr:
									#ifndef WINDOWS
										dr->vPtr += (int) r->vInt;
									#endif
										break;
									default:
										return 1;
								}
								break;
							case RegPtr:
								return 1;
							default:
								return 1;
						}
					}
				}
			}
		} else if (strcmp(strlwr(head), "inc") == 0) {
			if (dst[0] == 'r' || dst[0] == 'R') {
				int drn = -1;
				sscanf(dst, "%*[rR]%d", &drn);
				if (drn >= 0 && drn < REG_CNT) {
					switch (reg[drn].type) {
						case RegChar:
							reg[drn].vChar += 1;
							break;
						case RegFloat:
							reg[drn].vFloat += 1.0F;
							break;
						case RegInt:
							reg[drn].vInt += 1;
							break;
						case RegPtr:
						#ifndef WINDOWS
							reg[drn].vPtr += 1;
						#endif
							break;
						default:
							return 1;
					}
				} else return 1;
			} else {
				Register* dr = mm->get(mm->p, dst);
				if (dr == 0) return 1;
				switch (dr->type) {
					case RegChar:
						dr->vChar += 1;
						break;
					case RegFloat:
						dr->vFloat += 1.0F;
						break;
					case RegInt:
						dr->vInt += 1;
						break;
					case RegPtr:
					#ifndef WINDOWS
						dr->vPtr += 1;
					#endif
						break;
					default:
						return 1;
				}
			}
		} else if (strcmp(strlwr(head), "sub") == 0) {
			if (dst[0] == 'r' || dst[0] == 'R') {
				int drn = -1;
				sscanf(dst, "%*[rR]%d", &drn);
				if (drn >= 0 && drn < REG_CNT) {
					if (src[0] == 'r' || src[0] == 'R') {
						int srn = -1;
						sscanf(src, "%*[rR]%d", &srn);
						if (srn >= 0 && srn < REG_CNT) {
							switch (reg[drn].type) {
								case RegChar:
									switch (reg[srn].type) {
										case RegChar:
											reg[drn].vChar -= (char) reg[srn].vChar;
											break;
										case RegFloat:
											reg[drn].vChar -= (char) reg[srn].vFloat;
											break;
										case RegInt:
											reg[drn].vChar -= (char) reg[srn].vInt;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegFloat:
									switch (reg[srn].type) {
										case RegChar:
											reg[drn].vFloat -= (float) reg[srn].vChar;
											break;
										case RegFloat:
											reg[drn].vFloat -= (float) reg[srn].vFloat;
											break;
										case RegInt:
											reg[drn].vFloat -= (float) reg[srn].vInt;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegInt:
									switch (reg[srn].type) {
										case RegChar:
											reg[drn].vInt -= (int) reg[srn].vChar;
											break;
										case RegFloat:
											reg[drn].vInt -= (int) reg[srn].vFloat;
											break;
										case RegInt:
											reg[drn].vInt -= (int) reg[srn].vInt;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegPtr:
								#ifndef WINDOWS
									switch (reg[srn].type) {
										case RegChar:
											reg[drn].vPtr -= (int) reg[srn].vChar;
											break;
										case RegFloat:
											return 1;
										case RegInt:
											reg[drn].vPtr -= (int) reg[srn].vInt;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
								#endif
									break;
								default:
									return 1;
							}
						} else return 1;
					} else {
						if (src[0] == '\'') {
							if (src[strlen(src) - 1] != '\'') return 1;
							char tmp = 0;
							if (sscanf(src, "%*[\']%[^\']c", &tmp)) {
								switch (reg[drn].type) {
									case RegChar:
										reg[drn].vChar -= (char) tmp;
										break;
									case RegFloat:
										reg[drn].vFloat -= (float) tmp;
										break;
									case RegInt:
										reg[drn].vInt -= (int) tmp;
										break;
									case RegPtr:
									#ifndef WINDOWS
										reg[drn].vPtr -= (int) tmp;
									#endif
										break;
									default:
										return 1;
								}
							} else {
								return 1;
							}
						} else if (src[0] >= '0' && src[0] <= '9' || src[0] == '-' || src[0] == '+') {
							if (src[strlen(src) - 1] == 'F' || src[strlen(src) - 1] == 'f' || 
								strchr(src, '.') > 0) {
								float tmp = 0;
								if (sscanf(src, "%f", &tmp)) {
									switch (reg[drn].type) {
										case RegChar:
											reg[drn].vChar -= (char) tmp;
											break;
										case RegFloat:
											reg[drn].vFloat -= (float) tmp;
											break;
										case RegInt:
											reg[drn].vInt -= (int) tmp;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
								} else {
									return 1;
								}
							} else {
								int tmp = 0;
								if (src[1] == 'x' || src[1] == 'X' || 
								src[strlen(src) - 1] == 'h' || src[strlen(src) - 1] == 'H') {
									sscanf(src, "%x", &tmp);
								} else {
									sscanf(src, "%d", &tmp);
								}
								switch (reg[drn].type) {
									case RegChar:
										reg[drn].vChar -= (char) tmp;
										break;
									case RegFloat:
										reg[drn].vFloat -= (float) tmp;
										break;
									case RegInt:
										reg[drn].vInt -= (int) tmp;
										break;
									case RegPtr:
									#ifndef WINDOWS
										reg[drn].vPtr -= (int) tmp;
									#endif
										break;
									default:
										return 1;
								}
							}
						} else {
							Register* r = mm->get(mm->p, src);
							if (r == 0) return 1;
							switch (r->type) {
								case RegChar:
									switch (reg[drn].type) {
										case RegChar:
											reg[drn].vChar -= (char) r->vChar;
											break;
										case RegFloat:
											reg[drn].vFloat -= (float) r->vChar;
											break;
										case RegInt:
											reg[drn].vInt -= (int) r->vChar;
											break;
										case RegPtr:
										#ifndef WINDOWS
											reg[drn].vPtr -= (int) r->vChar;
										#endif
											break;
										default:
											return 1;
									}
									break;
								case RegFloat:
									switch (reg[drn].type) {
										case RegChar:
											reg[drn].vChar -= (char) r->vFloat;
											break;
										case RegFloat:
											reg[drn].vFloat -= (float) r->vFloat;
											break;
										case RegInt:
											reg[drn].vInt -= (int) r->vFloat;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegInt:
									switch (reg[drn].type) {
										case RegChar:
											reg[drn].vChar -= (char) r->vInt;
											break;
										case RegFloat:
											reg[drn].vFloat -= (float) r->vInt;
											break;
										case RegInt:
											reg[drn].vInt -= (int) r->vInt;
											break;
										case RegPtr:
										#ifndef WINDOWS
											reg[drn].vPtr -= (int) r->vInt;
										#endif
											break;
										default:
											return 1;
									}
									break;
								case RegPtr:
									return 1;
								default:
									return 1;
							}
						}
					}
				} else return 1;
			} else {
				Register* dr = mm->get(mm->p, dst);
				if (dr == 0) return 1;
				if (src[0] == 'r' || src[0] == 'R') {
					int srn = -1;
					sscanf(src, "%*[rR]%d", &srn);
					if (srn >= 0 && srn < REG_CNT) {
						switch (dr->type) {
							case RegChar:
								switch (reg[srn].type) {
									case RegChar:
										dr->vChar -= (char) reg[srn].vChar;
										break;
									case RegFloat:
										dr->vChar -= (char) reg[srn].vFloat;
										break;
									case RegInt:
										dr->vChar -= (char) reg[srn].vInt;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegFloat:
								switch (reg[srn].type) {
									case RegChar:
										dr->vFloat -= (float) reg[srn].vChar;
										break;
									case RegFloat:
										dr->vFloat -= (float) reg[srn].vFloat;
										break;
									case RegInt:
										dr->vFloat -= (float) reg[srn].vInt;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegInt:
								switch (reg[srn].type) {
									case RegChar:
										dr->vInt -= (int) reg[srn].vChar;
										break;
									case RegFloat:
										dr->vInt -= (int) reg[srn].vFloat;
										break;
									case RegInt:
										dr->vInt -= (int) reg[srn].vInt;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegPtr:
							#ifndef WINDOWS
								switch (reg[srn].type) {
									case RegChar:
										dr->vPtr -= (int) reg[srn].vChar;
										break;
									case RegFloat:
										return 1;
									case RegInt:
										dr->vPtr -= (int) reg[srn].vInt;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
							#endif
								break;
							default:
								return 1;
						}
					} else return 1;
				} else {
					if (src[0] == '\'') {
						if (src[strlen(src) - 1] != '\'') return 1;
						char tmp = 0;
						if (sscanf(src, "%*[\']%[^\']c", &tmp)) {
							switch (dr->type) {
								case RegChar:
									dr->vChar -= (char) tmp;
									break;
								case RegFloat:
									dr->vFloat -= (float) tmp;
									break;
								case RegInt:
									dr->vInt -= (int) tmp;
									break;
								case RegPtr:
								#ifndef WINDOWS
									dr->vPtr -= (int) tmp;
								#endif
									break;
								default:
									return 1;
							}
						} else {
							return 1;
						}
					} else if (src[0] >= '0' && src[0] <= '9' || src[0] == '-' || src[0] == '+') {
						if (src[strlen(src) - 1] == 'F' || src[strlen(src) - 1] == 'f' || 
							strchr(src, '.') > 0) {
							float tmp = 0;
							if (sscanf(src, "%f", &tmp)) {
								switch (dr->type) {
									case RegChar:
										dr->vChar -= (char) tmp;
										break;
									case RegFloat:
										dr->vFloat -= (float) tmp;
										break;
									case RegInt:
										dr->vInt -= (int) tmp;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
							} else {
								return 1;
							}
						} else {
							int tmp = 0;
							if (src[1] == 'x' || src[1] == 'X' || 
							src[strlen(src) - 1] == 'h' || src[strlen(src) - 1] == 'H') {
								sscanf(src, "%x", &tmp);
							} else {
								sscanf(src, "%d", &tmp);
							}
							switch (dr->type) {
								case RegChar:
									dr->vChar -= (char) tmp;
									break;
								case RegFloat:
									dr->vFloat -= (float) tmp;
									break;
								case RegInt:
									dr->vInt -= (int) tmp;
									break;
								case RegPtr:
								#ifndef WINDOWS
									dr->vPtr -= (int) tmp;
								#endif
									break;
								default:
									return 1;
							}
						}
					} else {
						Register* r = mm->get(mm->p, src);
						if (r == 0) return 1;
						switch (r->type) {
							case RegChar:
								switch (dr->type) {
									case RegChar:
										dr->vChar -= (char) r->vChar;
										break;
									case RegFloat:
										dr->vFloat -= (float) r->vChar;
										break;
									case RegInt:
										dr->vInt -= (int) r->vChar;
										break;
									case RegPtr:
									#ifndef WINDOWS
										dr->vPtr -= (int) r->vChar;
									#endif
										break;
									default:
										return 1;
								}
								break;
							case RegFloat:
								switch (dr->type) {
									case RegChar:
										dr->vChar -= (char) r->vFloat;
										break;
									case RegFloat:
										dr->vFloat -= (float) r->vFloat;
										break;
									case RegInt:
										dr->vInt -= (int) r->vFloat;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegInt:
								switch (dr->type) {
									case RegChar:
										dr->vChar -= (char) r->vInt;
										break;
									case RegFloat:
										dr->vFloat -= (float) r->vInt;
										break;
									case RegInt:
										dr->vInt -= (int) r->vInt;
										break;
									case RegPtr:
									#ifndef WINDOWS
										dr->vPtr -= (int) r->vInt;
									#endif
										break;
									default:
										return 1;
								}
								break;
							case RegPtr:
								return 1;
							default:
								return 1;
						}
					}
				}
			}
		} else if (strcmp(strlwr(head), "dec") == 0) {
			if (dst[0] == 'r' || dst[0] == 'R') {
				int drn = -1;
				sscanf(dst, "%*[rR]%d", &drn);
				if (drn >= 0 && drn < REG_CNT) {
					switch (reg[drn].type) {
						case RegChar:
							reg[drn].vChar -= 1;
							break;
						case RegFloat:
							reg[drn].vFloat -= 1.0F;
							break;
						case RegInt:
							reg[drn].vInt -= 1;
							break;
						case RegPtr:
						#ifndef WINDOWS
							reg[drn].vPtr -= 1;
						#endif
							break;
						default:
							return 1;
					}
				} else return 1;
			} else {
				Register* dr = mm->get(mm->p, dst);
				if (dr == 0) return 1;
				switch (dr->type) {
					case RegChar:
						dr->vChar -= 1;
						break;
					case RegFloat:
						dr->vFloat -= 1.0F;
						break;
					case RegInt:
						dr->vInt -= 1;
						break;
					case RegPtr:
					#ifndef WINDOWS
						dr->vPtr -= 1;
					#endif
						break;
					default:
						return 1;
				}
			}
		} else if (strcmp(strlwr(head), "mul") == 0) {
			if (dst[0] == 'r' || dst[0] == 'R') {
				int drn = -1;
				sscanf(dst, "%*[rR]%d", &drn);
				if (drn >= 0 && drn < REG_CNT) {
					if (src[0] == 'r' || src[0] == 'R') {
						int srn = -1;
						sscanf(src, "%*[rR]%d", &srn);
						if (srn >= 0 && srn < REG_CNT) {
							switch (reg[drn].type) {
								case RegChar:
									switch (reg[srn].type) {
										case RegChar:
											reg[drn].vChar *= (char) reg[srn].vChar;
											break;
										case RegFloat:
											reg[drn].vChar *= (char) reg[srn].vFloat;
											break;
										case RegInt:
											reg[drn].vChar *= (char) reg[srn].vInt;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegFloat:
									switch (reg[srn].type) {
										case RegChar:
											reg[drn].vFloat *= (float) reg[srn].vChar;
											break;
										case RegFloat:
											reg[drn].vFloat *= (float) reg[srn].vFloat;
											break;
										case RegInt:
											reg[drn].vFloat *= (float) reg[srn].vInt;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegInt:
									switch (reg[srn].type) {
										case RegChar:
											reg[drn].vInt *= (int) reg[srn].vChar;
											break;
										case RegFloat:
											reg[drn].vInt *= (int) reg[srn].vFloat;
											break;
										case RegInt:
											reg[drn].vInt *= (int) reg[srn].vInt;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegPtr:
									return 1;
								default:
									return 1;
							}
						} else return 1;
					} else {
						if (src[0] == '\'') {
							if (src[strlen(src) - 1] != '\'') return 1;
							char tmp = 0;
							if (sscanf(src, "%*[\']%[^\']c", &tmp)) {
								switch (reg[drn].type) {
									case RegChar:
										reg[drn].vChar *= (char) tmp;
										break;
									case RegFloat:
										reg[drn].vFloat *= (float) tmp;
										break;
									case RegInt:
										reg[drn].vInt *= (int) tmp;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
							} else {
								return 1;
							}
						} else if (src[0] >= '0' && src[0] <= '9' || src[0] == '-' || src[0] == '+') {
							if (src[strlen(src) - 1] == 'F' || src[strlen(src) - 1] == 'f' || 
								strchr(src, '.') > 0) {
								float tmp = 0;
								if (sscanf(src, "%f", &tmp)) {
									switch (reg[drn].type) {
										case RegChar:
											reg[drn].vChar *= (char) tmp;
											break;
										case RegFloat:
											reg[drn].vFloat *= (float) tmp;
											break;
										case RegInt:
											reg[drn].vInt *= (int) tmp;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
								} else {
									return 1;
								}
							} else {
								int tmp = 0;
								if (src[1] == 'x' || src[1] == 'X' || 
								src[strlen(src) - 1] == 'h' || src[strlen(src) - 1] == 'H') {
									sscanf(src, "%x", &tmp);
								} else {
									sscanf(src, "%d", &tmp);
								}
								switch (reg[drn].type) {
									case RegChar:
										reg[drn].vChar *= (char) tmp;
										break;
									case RegFloat:
										reg[drn].vFloat *= (float) tmp;
										break;
									case RegInt:
										reg[drn].vInt *= (int) tmp;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
							}
						} else {
							Register* r = mm->get(mm->p, src);
							if (r == 0) return 1;
							switch (r->type) {
								case RegChar:
									switch (reg[drn].type) {
										case RegChar:
											reg[drn].vChar *= (char) r->vChar;
											break;
										case RegFloat:
											reg[drn].vFloat *= (float) r->vChar;
											break;
										case RegInt:
											reg[drn].vInt *= (int) r->vChar;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegFloat:
									switch (reg[drn].type) {
										case RegChar:
											reg[drn].vChar *= (char) r->vFloat;
											break;
										case RegFloat:
											reg[drn].vFloat *= (float) r->vFloat;
											break;
										case RegInt:
											reg[drn].vInt *= (int) r->vFloat;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegInt:
									switch (reg[drn].type) {
										case RegChar:
											reg[drn].vChar *= (char) r->vInt;
											break;
										case RegFloat:
											reg[drn].vFloat *= (float) r->vInt;
											break;
										case RegInt:
											reg[drn].vInt *= (int) r->vInt;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegPtr:
									return 1;
								default:
									return 1;
							}
						}
					}
				} else return 1;
			} else {
				Register* dr = mm->get(mm->p, dst);
				if (dr == 0) return 1;
				if (src[0] == 'r' || src[0] == 'R') {
					int srn = -1;
					sscanf(src, "%*[rR]%d", &srn);
					if (srn >= 0 && srn < REG_CNT) {
						switch (dr->type) {
							case RegChar:
								switch (reg[srn].type) {
									case RegChar:
										dr->vChar *= (char) reg[srn].vChar;
										break;
									case RegFloat:
										dr->vChar *= (char) reg[srn].vFloat;
										break;
									case RegInt:
										dr->vChar *= (char) reg[srn].vInt;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegFloat:
								switch (reg[srn].type) {
									case RegChar:
										dr->vFloat *= (float) reg[srn].vChar;
										break;
									case RegFloat:
										dr->vFloat *= (float) reg[srn].vFloat;
										break;
									case RegInt:
										dr->vFloat *= (float) reg[srn].vInt;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegInt:
								switch (reg[srn].type) {
									case RegChar:
										dr->vInt *= (int) reg[srn].vChar;
										break;
									case RegFloat:
										dr->vInt *= (int) reg[srn].vFloat;
										break;
									case RegInt:
										dr->vInt *= (int) reg[srn].vInt;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegPtr:
								return 1;
							default:
								return 1;
						}
					} else return 1;
				} else {
					if (src[0] == '\'') {
						if (src[strlen(src) - 1] != '\'') return 1;
						char tmp = 0;
						if (sscanf(src, "%*[\']%[^\']c", &tmp)) {
							switch (dr->type) {
								case RegChar:
									dr->vChar *= (char) tmp;
									break;
								case RegFloat:
									dr->vFloat *= (float) tmp;
									break;
								case RegInt:
									dr->vInt *= (int) tmp;
									break;
								case RegPtr:
									return 1;
								default:
									return 1;
							}
						} else {
							return 1;
						}
					} else if (src[0] >= '0' && src[0] <= '9' || src[0] == '-' || src[0] == '+') {
						if (src[strlen(src) - 1] == 'F' || src[strlen(src) - 1] == 'f' || 
							strchr(src, '.') > 0) {
							float tmp = 0;
							if (sscanf(src, "%f", &tmp)) {
								switch (dr->type) {
									case RegChar:
										dr->vChar *= (char) tmp;
										break;
									case RegFloat:
										dr->vFloat *= (float) tmp;
										break;
									case RegInt:
										dr->vInt *= (int) tmp;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
							} else {
								return 1;
							}
						} else {
							int tmp = 0;
							if (src[1] == 'x' || src[1] == 'X' || 
							src[strlen(src) - 1] == 'h' || src[strlen(src) - 1] == 'H') {
								sscanf(src, "%x", &tmp);
							} else {
								sscanf(src, "%d", &tmp);
							}
							switch (dr->type) {
								case RegChar:
									dr->vChar *= (char) tmp;
									break;
								case RegFloat:
									dr->vFloat *= (float) tmp;
									break;
								case RegInt:
									dr->vInt *= (int) tmp;
									break;
								case RegPtr:
									return 1;
								default:
									return 1;
							}
						}
					} else {
						Register* r = mm->get(mm->p, src);
						if (r == 0) return 1;
						switch (r->type) {
							case RegChar:
								switch (dr->type) {
									case RegChar:
										dr->vChar *= (char) r->vChar;
										break;
									case RegFloat:
										dr->vFloat *= (float) r->vChar;
										break;
									case RegInt:
										dr->vInt *= (int) r->vChar;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegFloat:
								switch (dr->type) {
									case RegChar:
										dr->vChar *= (char) r->vFloat;
										break;
									case RegFloat:
										dr->vFloat *= (float) r->vFloat;
										break;
									case RegInt:
										dr->vInt *= (int) r->vFloat;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegInt:
								switch (dr->type) {
									case RegChar:
										dr->vChar *= (char) r->vInt;
										break;
									case RegFloat:
										dr->vFloat *= (float) r->vInt;
										break;
									case RegInt:
										dr->vInt *= (int) r->vInt;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegPtr:
								return 1;
							default:
								return 1;
						}
					}
				}
			}
		} else if (strcmp(strlwr(head), "div") == 0) {
			if (dst[0] == 'r' || dst[0] == 'R') {
				int drn = -1;
				sscanf(dst, "%*[rR]%d", &drn);
				if (drn >= 0 && drn < REG_CNT) {
					if (src[0] == 'r' || src[0] == 'R') {
						int srn = -1;
						sscanf(src, "%*[rR]%d", &srn);
						if (srn >= 0 && srn < REG_CNT) {
							switch (reg[drn].type) {
								case RegChar:
									switch (reg[srn].type) {
										case RegChar:
											reg[drn].vChar /= (char) reg[srn].vChar;
											break;
										case RegFloat:
											reg[drn].vChar /= (char) reg[srn].vFloat;
											break;
										case RegInt:
											reg[drn].vChar /= (char) reg[srn].vInt;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegFloat:
									switch (reg[srn].type) {
										case RegChar:
											reg[drn].vFloat /= (float) reg[srn].vChar;
											break;
										case RegFloat:
											reg[drn].vFloat /= (float) reg[srn].vFloat;
											break;
										case RegInt:
											reg[drn].vFloat /= (float) reg[srn].vInt;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegInt:
									switch (reg[srn].type) {
										case RegChar:
											reg[drn].vInt /= (int) reg[srn].vChar;
											break;
										case RegFloat:
											reg[drn].vInt /= (int) reg[srn].vFloat;
											break;
										case RegInt:
											reg[drn].vInt /= (int) reg[srn].vInt;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegPtr:
									return 1;
								default:
									return 1;
							}
						} else return 1;
					} else {
						if (src[0] == '\'') {
							if (src[strlen(src) - 1] != '\'') return 1;
							char tmp = 0;
							if (sscanf(src, "%*[\']%[^\']c", &tmp)) {
								switch (reg[drn].type) {
									case RegChar:
										reg[drn].vChar /= (char) tmp;
										break;
									case RegFloat:
										reg[drn].vFloat /= (float) tmp;
										break;
									case RegInt:
										reg[drn].vInt /= (int) tmp;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
							} else {
								return 1;
							}
						} else if (src[0] >= '0' && src[0] <= '9' || src[0] == '-' || src[0] == '+') {
							if (src[strlen(src) - 1] == 'F' || src[strlen(src) - 1] == 'f' || 
								strchr(src, '.') > 0) {
								float tmp = 0;
								if (sscanf(src, "%f", &tmp)) {
									switch (reg[drn].type) {
										case RegChar:
											reg[drn].vChar /= (char) tmp;
											break;
										case RegFloat:
											reg[drn].vFloat /= (float) tmp;
											break;
										case RegInt:
											reg[drn].vInt /= (int) tmp;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
								} else {
									return 1;
								}
							} else {
								int tmp = 0;
								if (src[1] == 'x' || src[1] == 'X' || 
								src[strlen(src) - 1] == 'h' || src[strlen(src) - 1] == 'H') {
									sscanf(src, "%x", &tmp);
								} else {
									sscanf(src, "%d", &tmp);
								}
								switch (reg[drn].type) {
									case RegChar:
										reg[drn].vChar /= (char) tmp;
										break;
									case RegFloat:
										reg[drn].vFloat /= (float) tmp;
										break;
									case RegInt:
										reg[drn].vInt /= (int) tmp;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
							}
						} else {
							Register* r = mm->get(mm->p, src);
							if (r == 0) return 1;
							switch (r->type) {
								case RegChar:
									switch (reg[drn].type) {
										case RegChar:
											reg[drn].vChar /= (char) r->vChar;
											break;
										case RegFloat:
											reg[drn].vFloat /= (float) r->vChar;
											break;
										case RegInt:
											reg[drn].vInt /= (int) r->vChar;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegFloat:
									switch (reg[drn].type) {
										case RegChar:
											reg[drn].vChar /= (char) r->vFloat;
											break;
										case RegFloat:
											reg[drn].vFloat /= (float) r->vFloat;
											break;
										case RegInt:
											reg[drn].vInt /= (int) r->vFloat;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegInt:
									switch (reg[drn].type) {
										case RegChar:
											reg[drn].vChar /= (char) r->vInt;
											break;
										case RegFloat:
											reg[drn].vFloat /= (float) r->vInt;
											break;
										case RegInt:
											reg[drn].vInt /= (int) r->vInt;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegPtr:
									return 1;
								default:
									return 1;
							}
						}
					}
				} else return 1;
			} else {
				Register* dr = mm->get(mm->p, dst);
				if (dr == 0) return 1;
				if (src[0] == 'r' || src[0] == 'R') {
					int srn = -1;
					sscanf(src, "%*[rR]%d", &srn);
					if (srn >= 0 && srn < REG_CNT) {
						switch (dr->type) {
							case RegChar:
								switch (reg[srn].type) {
									case RegChar:
										dr->vChar /= (char) reg[srn].vChar;
										break;
									case RegFloat:
										dr->vChar /= (char) reg[srn].vFloat;
										break;
									case RegInt:
										dr->vChar /= (char) reg[srn].vInt;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegFloat:
								switch (reg[srn].type) {
									case RegChar:
										dr->vFloat /= (float) reg[srn].vChar;
										break;
									case RegFloat:
										dr->vFloat /= (float) reg[srn].vFloat;
										break;
									case RegInt:
										dr->vFloat /= (float) reg[srn].vInt;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegInt:
								switch (reg[srn].type) {
									case RegChar:
										dr->vInt /= (int) reg[srn].vChar;
										break;
									case RegFloat:
										dr->vInt /= (int) reg[srn].vFloat;
										break;
									case RegInt:
										dr->vInt /= (int) reg[srn].vInt;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegPtr:
								return 1;
							default:
								return 1;
						}
					}  else return 1;
				} else {
					if (src[0] == '\'') {
						if (src[strlen(src) - 1] != '\'') return 1;
						char tmp = 0;
						if (sscanf(src, "%*[\']%[^\']c", &tmp)) {
							switch (dr->type) {
								case RegChar:
									dr->vChar /= (char) tmp;
									break;
								case RegFloat:
									dr->vFloat /= (float) tmp;
									break;
								case RegInt:
									dr->vInt /= (int) tmp;
									break;
								case RegPtr:
									return 1;
								default:
									return 1;
							}
						} else {
							return 1;
						}
					} else if (src[0] >= '0' && src[0] <= '9' || src[0] == '-' || src[0] == '+') {
						if (src[strlen(src) - 1] == 'F' || src[strlen(src) - 1] == 'f' || 
							strchr(src, '.') > 0) {
							float tmp = 0;
							if (sscanf(src, "%f", &tmp)) {
								switch (dr->type) {
									case RegChar:
										dr->vChar /= (char) tmp;
										break;
									case RegFloat:
										dr->vFloat /= (float) tmp;
										break;
									case RegInt:
										dr->vInt /= (int) tmp;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
							} else {
								return 1;
							}
						} else {
							int tmp = 0;
							if (src[1] == 'x' || src[1] == 'X' || 
							src[strlen(src) - 1] == 'h' || src[strlen(src) - 1] == 'H') {
								sscanf(src, "%x", &tmp);
							} else {
								sscanf(src, "%d", &tmp);
							}
							switch (dr->type) {
								case RegChar:
									dr->vChar /= (char) tmp;
									break;
								case RegFloat:
									dr->vFloat /= (float) tmp;
									break;
								case RegInt:
									dr->vInt /= (int) tmp;
									break;
								case RegPtr:
									return 1;
								default:
									return 1;
							}
						}
					} else {
						Register* r = mm->get(mm->p, src);
						if (r == 0) return 1;
						switch (r->type) {
							case RegChar:
								switch (dr->type) {
									case RegChar:
										dr->vChar /= (char) r->vChar;
										break;
									case RegFloat:
										dr->vFloat /= (float) r->vChar;
										break;
									case RegInt:
										dr->vInt /= (int) r->vChar;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegFloat:
								switch (dr->type) {
									case RegChar:
										dr->vChar /= (char) r->vFloat;
										break;
									case RegFloat:
										dr->vFloat /= (float) r->vFloat;
										break;
									case RegInt:
										dr->vInt /= (int) r->vFloat;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegInt:
								switch (dr->type) {
									case RegChar:
										dr->vChar /= (char) r->vInt;
										break;
									case RegFloat:
										dr->vFloat /= (float) r->vInt;
										break;
									case RegInt:
										dr->vInt /= (int) r->vInt;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegPtr:
								return 1;
							default:
								return 1;
						}
					}
				}
			}
		} else if (strcmp(strlwr(head), "cmp") == 0) {
			if (dst[0] == 'r' || dst[0] == 'R') {
				int drn = -1;
				sscanf(dst, "%*[rR]%d", &drn);
				if (drn >= 0 && drn < REG_CNT) {
					if (src[0] == 'r' || src[0] == 'R') {
						int srn = -1;
						sscanf(src, "%*[rR]%d", &srn);
						if (srn >= 0 && srn < REG_CNT) {
							switch (reg[drn].type) {
								case RegChar:
									switch (reg[srn].type) {
										case RegChar:
											if (reg[drn].vChar > (char) reg[srn].vChar)
												state.vChar = 1;
											else if (reg[drn].vChar < (char) reg[srn].vChar)
												state.vChar = -1;
											else state.vChar = 0;
											state.type = RegChar;
											break;
										case RegFloat:
											if (reg[drn].vChar > (char) reg[srn].vFloat)
												state.vChar = 1;
											else if (reg[drn].vChar < (char) reg[srn].vFloat)
												state.vChar = -1;
											else state.vChar = 0;
											state.type = RegChar;
											break;
										case RegInt:
											if (reg[drn].vChar > (char) reg[srn].vInt)
												state.vChar = 1;
											else if (reg[drn].vChar < (char) reg[srn].vInt)
												state.vChar = -1;
											else state.vChar = 0;
											state.type = RegChar;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegFloat:
									switch (reg[srn].type) {
										case RegChar:
											if (reg[drn].vFloat > (float) reg[srn].vChar)
												state.vChar = 1;
											else if (reg[drn].vFloat < (float) reg[srn].vChar)
												state.vChar = -1;
											else state.vChar = 0;
											state.type = RegChar;
											break;
										case RegFloat:
											if (reg[drn].vFloat > (float) reg[srn].vFloat)
												state.vChar = 1;
											else if (reg[drn].vFloat < (float) reg[srn].vFloat)
												state.vChar = -1;
											else state.vChar = 0;
											state.type = RegChar;
											break;
										case RegInt:
											if (reg[drn].vFloat > (float) reg[srn].vInt)
												state.vChar = 1;
											else if (reg[drn].vFloat < (float) reg[srn].vInt)
												state.vChar = -1;
											else state.vChar = 0;
											state.type = RegChar;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegInt:
									switch (reg[srn].type) {
										case RegChar:
											if (reg[drn].vInt > (int) reg[srn].vChar)
												state.vChar = 1;
											else if (reg[drn].vInt < (int) reg[srn].vChar)
												state.vChar = -1;
											else state.vChar = 0;
											state.type = RegChar;
											break;
										case RegFloat:
											if (reg[drn].vInt > (int) reg[srn].vFloat)
												state.vChar = 1;
											else if (reg[drn].vInt < (int) reg[srn].vFloat)
												state.vChar = -1;
											else state.vChar = 0;
											state.type = RegChar;
											break;
										case RegInt:
											if (reg[drn].vInt > (int) reg[srn].vInt)
												state.vChar = 1;
											else if (reg[drn].vInt < (int) reg[srn].vInt)
												state.vChar = -1;
											else state.vChar = 0;
											state.type = RegChar;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegPtr:
									return 1;
								default:
									return 1;
							}
						} else return 1;
					} else {
						if (src[0] == '\'') {
							if (src[strlen(src) - 1] != '\'') return 1;
							char tmp = 0;
							if (sscanf(src, "%*[\']%[^\']c", &tmp)) {
								switch (reg[drn].type) {
									case RegChar:
										if (reg[drn].vChar > (char) tmp)
											state.vChar = 1;
										else if (reg[drn].vChar < (char) tmp)
											state.vChar = -1;
										else state.vChar = 0;
										state.type = RegChar;
										break;
									case RegFloat:
										if (reg[drn].vFloat > (float) tmp)
											state.vChar = 1;
										else if (reg[drn].vFloat < (float) tmp)
											state.vChar = -1;
										else state.vChar = 0;
										state.type = RegChar;
										break;
									case RegInt:
										if (reg[drn].vInt > (int) tmp)
											state.vChar = 1;
										else if (reg[drn].vInt < (int) tmp)
											state.vChar = -1;
										else state.vChar = 0;
										state.type = RegChar;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
							} else {
								return 1;
							}
						} else if (src[0] >= '0' && src[0] <= '9' || src[0] == '-' || src[0] == '+') {
							if (src[strlen(src) - 1] == 'F' || src[strlen(src) - 1] == 'f' || 
								strchr(src, '.') > 0) {
								float tmp = 0;
								if (sscanf(src, "%f", &tmp)) {
									switch (reg[drn].type) {
										case RegChar:
											if (reg[drn].vChar > (char) tmp)
												state.vChar = 1;
											else if (reg[drn].vChar < (char) tmp)
												state.vChar = -1;
											else state.vChar = 0;
											state.type = RegChar;
											break;
										case RegFloat:
											if (reg[drn].vFloat > (float) tmp)
												state.vChar = 1;
											else if (reg[drn].vFloat < (float) tmp)
												state.vChar = -1;
											else state.vChar = 0;
											state.type = RegChar;
											break;
										case RegInt:
											if (reg[drn].vInt > (int) tmp)
												state.vChar = 1;
											else if (reg[drn].vInt < (int) tmp)
												state.vChar = -1;
											else state.vChar = 0;
											state.type = RegChar;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
								} else {
									return 1;
								}
							} else {
								int tmp = 0;
								if (src[1] == 'x' || src[1] == 'X' || 
								src[strlen(src) - 1] == 'h' || src[strlen(src) - 1] == 'H') {
									sscanf(src, "%x", &tmp);
								} else {
									sscanf(src, "%d", &tmp);
								}
								switch (reg[drn].type) {
									case RegChar:
										if (reg[drn].vChar > (char) tmp)
											state.vChar = 1;
										else if (reg[drn].vChar < (char) tmp)
											state.vChar = -1;
										else state.vChar = 0;
										state.type = RegChar;
										break;
									case RegFloat:
										if (reg[drn].vFloat > (float) tmp)
											state.vChar = 1;
										else if (reg[drn].vFloat < (float) tmp)
											state.vChar = -1;
										else state.vChar = 0;
										state.type = RegChar;
										break;
									case RegInt:
										if (reg[drn].vInt > (int) tmp)
											state.vChar = 1;
										else if (reg[drn].vInt < (int) tmp)
											state.vChar = -1;
										else state.vChar = 0;
										state.type = RegChar;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
							}
						} else {
							Register* r = mm->get(mm->p, src);
							if (r == 0) return 1;
							switch (r->type) {
								case RegChar:
									switch (reg[drn].type) {
										case RegChar:
											if (reg[drn].vChar > (char) r->vChar)
												state.vChar = 1;
											else if (reg[drn].vChar < (char) r->vChar)
												state.vChar = -1;
											else state.vChar = 0;
											state.type = RegChar;
											break;
										case RegFloat:
											if (reg[drn].vFloat > (float) r->vChar)
												state.vChar = 1;
											else if (reg[drn].vFloat < (float) r->vChar)
												state.vChar = -1;
											else state.vChar = 0;
											state.type = RegChar;
											break;
										case RegInt:
											if (reg[drn].vInt > (int) r->vChar)
												state.vChar = 1;
											else if (reg[drn].vInt < (int) r->vChar)
												state.vChar = -1;
											else state.vChar = 0;
											state.type = RegChar;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegFloat:
									switch (reg[drn].type) {
										case RegChar:
											if (reg[drn].vChar > (char) r->vFloat)
												state.vChar = 1;
											else if (reg[drn].vChar < (char) r->vFloat)
												state.vChar = -1;
											else state.vChar = 0;
											state.type = RegChar;
											break;
										case RegFloat:
											if (reg[drn].vFloat > (float) r->vFloat)
												state.vChar = 1;
											else if (reg[drn].vFloat < (float) r->vFloat)
												state.vChar = -1;
											else state.vChar = 0;
											state.type = RegChar;
											break;
										case RegInt:
											if (reg[drn].vInt > (int) r->vFloat)
												state.vChar = 1;
											else if (reg[drn].vInt < (int) r->vFloat)
												state.vChar = -1;
											else state.vChar = 0;
											state.type = RegChar;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegInt:
									switch (reg[drn].type) {
										case RegChar:
											if (reg[drn].vChar > (char) r->vInt)
												state.vChar = 1;
											else if (reg[drn].vChar < (char) r->vInt)
												state.vChar = -1;
											else state.vChar = 0;
											state.type = RegChar;
											break;
										case RegFloat:
											if (reg[drn].vFloat > (float) r->vInt)
												state.vChar = 1;
											else if (reg[drn].vFloat < (float) r->vInt)
												state.vChar = -1;
											else state.vChar = 0;
											state.type = RegChar;
											break;
										case RegInt:
											if (reg[drn].vInt > (int) r->vInt)
												state.vChar = 1;
											else if (reg[drn].vInt < (int) r->vInt)
												state.vChar = -1;
											else state.vChar = 0;
											state.type = RegChar;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegPtr:
									return 1;
								default:
									return 1;
							}
						}
					}
				} else return 1;
			} else {
				Register* dr = mm->get(mm->p, dst);
				if (dr == 0) return 1;
				if (src[0] == 'r' || src[0] == 'R') {
					int srn = -1;
					sscanf(src, "%*[rR]%d", &srn);
					if (srn >= 0 && srn < REG_CNT) {
						switch (dr->type) {
							case RegChar:
								switch (reg[srn].type) {
									case RegChar:
										if (dr->vChar > (char) reg[srn].vChar)
											state.vChar = 1;
										else if (dr->vChar < (char) reg[srn].vChar)
											state.vChar = -1;
										else state.vChar = 0;
										state.type = RegChar;
										break;
									case RegFloat:
										if (dr->vChar > (char) reg[srn].vFloat)
											state.vChar = 1;
										else if (dr->vChar < (char) reg[srn].vFloat)
											state.vChar = -1;
										else state.vChar = 0;
										state.type = RegChar;
										break;
									case RegInt:
										if (dr->vChar > (char) reg[srn].vInt)
											state.vChar = 1;
										else if (dr->vChar < (char) reg[srn].vInt)
											state.vChar = -1;
										else state.vChar = 0;
										state.type = RegChar;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegFloat:
								switch (reg[srn].type) {
									case RegChar:
										if (dr->vFloat > (float) reg[srn].vChar)
											state.vChar = 1;
										else if (dr->vFloat < (float) reg[srn].vChar)
											state.vChar = -1;
										else state.vChar = 0;
										state.type = RegChar;
										break;
									case RegFloat:
										if (dr->vFloat > (float) reg[srn].vFloat)
											state.vChar = 1;
										else if (dr->vFloat < (float) reg[srn].vFloat)
											state.vChar = -1;
										else state.vChar = 0;
										state.type = RegChar;
										break;
									case RegInt:
										if (dr->vFloat > (float) reg[srn].vInt)
											state.vChar = 1;
										else if (dr->vFloat < (float) reg[srn].vInt)
											state.vChar = -1;
										else state.vChar = 0;
										state.type = RegChar;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegInt:
								switch (reg[srn].type) {
									case RegChar:
										if (dr->vInt > (int) reg[srn].vChar)
											state.vChar = 1;
										else if (dr->vInt < (int) reg[srn].vChar)
											state.vChar = -1;
										else state.vChar = 0;
										state.type = RegChar;
										break;
									case RegFloat:
										if (dr->vInt > (int) reg[srn].vFloat)
											state.vChar = 1;
										else if (dr->vInt < (int) reg[srn].vFloat)
											state.vChar = -1;
										else state.vChar = 0;
										state.type = RegChar;
										break;
									case RegInt:
										if (dr->vInt > (int) reg[srn].vInt)
											state.vChar = 1;
										else if (dr->vInt < (int) reg[srn].vInt)
											state.vChar = -1;
										else state.vChar = 0;
										state.type = RegChar;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegPtr:
								return 1;
							default:
								return 1;
						}
					} else return 1;
				} else {
					if (src[0] == '\'') {
						if (src[strlen(src) - 1] != '\'') return 1;
						char tmp = 0;
						if (sscanf(src, "%*[\']%[^\']c", &tmp)) {
							switch (dr->type) {
								case RegChar:
									if (dr->vChar > (char) tmp)
										state.vChar = 1;
									else if (dr->vChar < (char) tmp)
										state.vChar = -1;
									else state.vChar = 0;
									state.type = RegChar;
									break;
								case RegFloat:
									if (dr->vFloat > (float) tmp)
										state.vChar = 1;
									else if (dr->vFloat < (float) tmp)
										state.vChar = -1;
									else state.vChar = 0;
									state.type = RegChar;
									break;
								case RegInt:
									if (dr->vInt > (int) tmp)
										state.vChar = 1;
									else if (dr->vInt < (int) tmp)
										state.vChar = -1;
									else state.vChar = 0;
									state.type = RegChar;
									break;
								case RegPtr:
									return 1;
								default:
									return 1;
							}
						} else {
							return 1;
						}
					} else if (src[0] >= '0' && src[0] <= '9' || src[0] == '-' || src[0] == '+') {
						if (src[strlen(src) - 1] == 'F' || src[strlen(src) - 1] == 'f' || 
							strchr(src, '.') > 0) {
							float tmp = 0;
							if (sscanf(src, "%f", &tmp)) {
								switch (dr->type) {
									case RegChar:
										if (dr->vChar > (char) tmp)
											state.vChar = 1;
										else if (dr->vChar < (char) tmp)
											state.vChar = -1;
										else state.vChar = 0;
										state.type = RegChar;
										break;
									case RegFloat:
										if (dr->vFloat > (float) tmp)
											state.vChar = 1;
										else if (dr->vFloat < (float) tmp)
											state.vChar = -1;
										else state.vChar = 0;
										state.type = RegChar;
										break;
									case RegInt:
										if (dr->vInt > (int) tmp)
											state.vChar = 1;
										else if (dr->vInt < (int) tmp)
											state.vChar = -1;
										else state.vChar = 0;
										state.type = RegChar;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
							} else {
								return 1;
							}
						} else {
							int tmp = 0;
							if (src[1] == 'x' || src[1] == 'X' || 
							src[strlen(src) - 1] == 'h' || src[strlen(src) - 1] == 'H') {
								sscanf(src, "%x", &tmp);
							} else {
								sscanf(src, "%d", &tmp);
							}
							switch (dr->type) {
								case RegChar:
									if (dr->vChar > (char) tmp)
										state.vChar = 1;
									else if (dr->vChar < (char) tmp)
										state.vChar = -1;
									else state.vChar = 0;
									state.type = RegChar;
									break;
								case RegFloat:
									if (dr->vFloat > (float) tmp)
										state.vChar = 1;
									else if (dr->vFloat < (float) tmp)
										state.vChar = -1;
									else state.vChar = 0;
									state.type = RegChar;
									break;
								case RegInt:
									if (dr->vInt > (int) tmp)
										state.vChar = 1;
									else if (dr->vInt < (int) tmp)
										state.vChar = -1;
									else state.vChar = 0;
									state.type = RegChar;
									break;
								case RegPtr:
									return 1;
								default:
									return 1;
							}
						}
					} else {
						Register* r = mm->get(mm->p, src);
						if (r == 0) return 1;
						switch (r->type) {
							case RegChar:
								switch (dr->type) {
									case RegChar:
										if (dr->vChar > (char) r->vChar)
											state.vChar = 1;
										else if (dr->vChar < (char) r->vChar)
											state.vChar = -1;
										else state.vChar = 0;
										state.type = RegChar;
										break;
									case RegFloat:
										if (dr->vFloat > (float) r->vChar)
											state.vChar = 1;
										else if (dr->vFloat < (float) r->vChar)
											state.vChar = -1;
										else state.vChar = 0;
										state.type = RegChar;
										break;
									case RegInt:
										if (dr->vInt > (int) r->vChar)
											state.vChar = 1;
										else if (dr->vInt < (int) r->vChar)
											state.vChar = -1;
										else state.vChar = 0;
										state.type = RegChar;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegFloat:
								switch (dr->type) {
									case RegChar:
										if (dr->vChar > (char) r->vFloat)
											state.vChar = 1;
										else if (dr->vChar < (char) r->vFloat)
											state.vChar = -1;
										else state.vChar = 0;
										state.type = RegChar;
										break;
									case RegFloat:
										if (dr->vFloat > (float) r->vFloat)
											state.vChar = 1;
										else if (dr->vFloat < (float) r->vFloat)
											state.vChar = -1;
										else state.vChar = 0;
										state.type = RegChar;
										break;
									case RegInt:
										if (dr->vInt > (int) r->vFloat)
											state.vChar = 1;
										else if (dr->vInt < (int) r->vFloat)
											state.vChar = -1;
										else state.vChar = 0;
										state.type = RegChar;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegInt:
								switch (dr->type) {
									case RegChar:
										if (dr->vChar > (char) r->vInt)
											state.vChar = 1;
										else if (dr->vChar < (char) r->vInt)
											state.vChar = -1;
										else state.vChar = 0;
										state.type = RegChar;
										break;
									case RegFloat:
										if (dr->vFloat > (float) r->vInt)
											state.vChar = 1;
										else if (dr->vFloat < (float) r->vInt)
											state.vChar = -1;
										else state.vChar = 0;
										state.type = RegChar;
										break;
									case RegInt:
										if (dr->vInt > (int) r->vInt)
											state.vChar = 1;
										else if (dr->vInt < (int) r->vInt)
											state.vChar = -1;
										else state.vChar = 0;
										state.type = RegChar;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegPtr:
								return 1;
							default:
								return 1;
						}
					}
				}
			}
		} else if (strcmp(strlwr(head), "jmp") == 0) {
			if (dst[0] == '[' && dst[strlen(dst) - 1] == ']') {
				strcpy(tagBuf, dst);
			} else return 1;
		} else if (strcmp(strlwr(head), "jz") == 0) {
			if (dst[0] == '[' && dst[strlen(dst) - 1] == ']') {
				if (state.vChar == 0) strcpy(tagBuf, dst);
			} else return 1;
		} else if (strcmp(strlwr(head), "jnz") == 0) {
			if (dst[0] == '[' && dst[strlen(dst) - 1] == ']') {
				if (state.vChar != 0) strcpy(tagBuf, dst);
			} else return 1;
		} else if (strcmp(strlwr(head), "jg") == 0) {
			if (dst[0] == '[' && dst[strlen(dst) - 1] == ']') {
				if (state.vChar > 0) strcpy(tagBuf, dst);
			} else return 1;
		} else if (strcmp(strlwr(head), "jl") == 0) {
			if (dst[0] == '[' && dst[strlen(dst) - 1] == ']') {
				if (state.vChar < 0) strcpy(tagBuf, dst);
			} else return 1;
		} else if (strcmp(strlwr(head), "and") == 0) {
			if (dst[0] == 'r' || dst[0] == 'R') {
				int drn = -1;
				sscanf(dst, "%*[rR]%d", &drn);
				if (drn >= 0 && drn < REG_CNT) {
					if (src[0] == 'r' || src[0] == 'R') {
						int srn = -1;
						sscanf(src, "%*[rR]%d", &srn);
						if (srn >= 0 && srn < REG_CNT) {
							if (reg[drn].type == reg[srn].type) {
								switch (reg[drn].type) {
									case RegChar:
										reg[drn].vChar &= reg[srn].vChar;
										break;
									case RegFloat:
										return 1;
									case RegInt:
										reg[drn].vInt &= reg[srn].vInt;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
							} else return 1;
						} else return 1;
					} else {
						if (src[0] == '\'') {
							if (src[strlen(src) - 1] != '\'') return 1;
							char tmp = 0;
							if(sscanf(src, "%*[\']%[^\']c", &tmp) && reg[drn].type == RegChar) {
								reg[drn].vChar &= tmp;
							} else {
								return 1;
							}
						} else if (src[0] >= '0' && src[0] <= '9' || src[0] == '-' || src[0] == '+') {
							if (src[strlen(src) - 1] == 'F' || src[strlen(src) - 1] == 'f' || 
								strchr(src, '.') > 0) {
								float tmp = 0;
								return 1;
							} else {
								int tmp = 0;
								if (src[1] == 'x' || src[1] == 'X' || 
								src[strlen(src) - 1] == 'h' || src[strlen(src) - 1] == 'H') {
									sscanf(src, "%x", &tmp);
								} else {
									sscanf(src, "%d", &tmp);
								}
								if (reg[drn].type == RegInt) {
									reg[drn].vInt &= tmp;
								} else {
									return 1;
								}
							}
						} else {
							Register* r = mm->get(mm->p, src);
							if (r == 0) return 1;
							if (reg[drn].type == r->type) {
								switch (reg[drn].type) {
									case RegChar:
										reg[drn].vChar &= r->vChar;
										break;
									case RegFloat:
										return 1;
									case RegInt:
										reg[drn].vInt &= r->vInt;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
							} else return 1;
						}
					}
				} else return 1;
			} else {
				Register* dr = mm->get(mm->p, dst);
				if (dr == 0) return 1;
				if (src[0] == 'r' || src[0] == 'R') {
					int srn = -1;
					sscanf(src, "%*[rR]%d", &srn);
					if (srn >= 0 && srn < REG_CNT) {
						if (dr->type == reg[srn].type) {
							switch (dr->type) {
								case RegChar:
									dr->vChar &= reg[srn].vChar;
									break;
								case RegFloat:
									return 1;
								case RegInt:
									dr->vInt &= reg[srn].vInt;
									break;
								case RegPtr:
									return 1;
								default:
									return 1;
							}
						} else return 1;
					} else return 1;
				} else {
					if (src[0] == '\'') {
						if (src[strlen(src) - 1] != '\'') return 1;
						char tmp = 0;
						if(sscanf(src, "%*[\']%[^\']c", &tmp) && dr->type == RegChar) {
							dr->vChar &= tmp;
						} else {
							return 1;
						}
					} else if (src[0] >= '0' && src[0] <= '9' || src[0] == '-' || src[0] == '+') {
						if (src[strlen(src) - 1] == 'F' || src[strlen(src) - 1] == 'f' || 
							strchr(src, '.') > 0) {
							float tmp = 0;
							return 1;
						} else {
							int tmp = 0;
							if (src[1] == 'x' || src[1] == 'X' || 
							src[strlen(src) - 1] == 'h' || src[strlen(src) - 1] == 'H') {
								sscanf(src, "%x", &tmp);
							} else {
								sscanf(src, "%d", &tmp);
							}
							if (dr->type == RegInt) {
								dr->vInt &= tmp;
							} else {
								return 1;
							}
						}
					} else {
						Register* r = mm->get(mm->p, src);
						if (r == 0) return 1;
						if (dr->type == r->type) {
							switch (dr->type) {
								case RegChar:
									dr->vChar &= r->vChar;
									break;
								case RegFloat:
									return 1;
								case RegInt:
									dr->vInt &= r->vInt;
									break;
								case RegPtr:
									return 1;
								default:
									return 1;
							}
						} else return 1;
					}
				}
			}
		} else if (strcmp(strlwr(head), "or") == 0) {
			if (dst[0] == 'r' || dst[0] == 'R') {
				int drn = -1;
				sscanf(dst, "%*[rR]%d", &drn);
				if (drn >= 0 && drn < REG_CNT) {
					if (src[0] == 'r' || src[0] == 'R') {
						int srn = -1;
						sscanf(src, "%*[rR]%d", &srn);
						if (srn >= 0 && srn < REG_CNT) {
							if (reg[drn].type == reg[srn].type) {
								switch (reg[drn].type) {
									case RegChar:
										reg[drn].vChar |= reg[srn].vChar;
										break;
									case RegFloat:
										return 1;
									case RegInt:
										reg[drn].vInt |= reg[srn].vInt;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
							} else return 1;
						} else return 1;
					} else {
						if (src[0] == '\'') {
							if (src[strlen(src) - 1] != '\'') return 1;
							char tmp = 0;
							if(sscanf(src, "%*[\']%[^\']c", &tmp) && reg[drn].type == RegChar) {
								reg[drn].vChar |= tmp;
							} else {
								return 1;
							}
						} else if (src[0] >= '0' && src[0] <= '9' || src[0] == '-' || src[0] == '+') {
							if (src[strlen(src) - 1] == 'F' || src[strlen(src) - 1] == 'f' || 
								strchr(src, '.') > 0) {
								float tmp = 0;
								return 1;
							} else {
								int tmp = 0;
								if (src[1] == 'x' || src[1] == 'X' || 
								src[strlen(src) - 1] == 'h' || src[strlen(src) - 1] == 'H') {
									sscanf(src, "%x", &tmp);
								} else {
									sscanf(src, "%d", &tmp);
								}
								if (reg[drn].type == RegInt) {
									reg[drn].vInt |= tmp;
								} else {
									return 1;
								}
							}
						} else {
							Register* r = mm->get(mm->p, src);
							if (r == 0) return 1;
							if (reg[drn].type == r->type) {
								switch (reg[drn].type) {
									case RegChar:
										reg[drn].vChar |= r->vChar;
										break;
									case RegFloat:
										return 1;
									case RegInt:
										reg[drn].vInt |= r->vInt;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
							} else return 1;
						}
					}
				} else return 1;
			} else {
				Register* dr = mm->get(mm->p, dst);
				if (dr == 0) return 1;
				if (src[0] == 'r' || src[0] == 'R') {
					int srn = -1;
					sscanf(src, "%*[rR]%d", &srn);
					if (srn >= 0 && srn < REG_CNT) {
						if (dr->type == reg[srn].type) {
							switch (dr->type) {
								case RegChar:
									dr->vChar |= reg[srn].vChar;
									break;
								case RegFloat:
									return 1;
								case RegInt:
									dr->vInt |= reg[srn].vInt;
									break;
								case RegPtr:
									return 1;
								default:
									return 1;
							}
						} else return 1;
					} else return 1;
				} else {
					if (src[0] == '\'') {
						if (src[strlen(src) - 1] != '\'') return 1;
						char tmp = 0;
						if(sscanf(src, "%*[\']%[^\']c", &tmp) && dr->type == RegChar) {
							dr->vChar |= tmp;
						} else {
							return 1;
						}
					} else if (src[0] >= '0' && src[0] <= '9' || src[0] == '-' || src[0] == '+') {
						if (src[strlen(src) - 1] == 'F' || src[strlen(src) - 1] == 'f' || 
							strchr(src, '.') > 0) {
							float tmp = 0;
							return 1;
						} else {
							int tmp = 0;
							if (src[1] == 'x' || src[1] == 'X' || 
							src[strlen(src) - 1] == 'h' || src[strlen(src) - 1] == 'H') {
								sscanf(src, "%x", &tmp);
							} else {
								sscanf(src, "%d", &tmp);
							}
							if (dr->type == RegInt) {
								dr->vInt |= tmp;
							} else {
								return 1;
							}
						}
					} else {
						Register* r = mm->get(mm->p, src);
						if (r == 0) return 1;
						if (dr->type == r->type) {
							switch (dr->type) {
								case RegChar:
									dr->vChar |= r->vChar;
									break;
								case RegFloat:
									return 1;
								case RegInt:
									dr->vInt |= r->vInt;
									break;
								case RegPtr:
									return 1;
								default:
									return 1;
							}
						} else return 1;
					}
				}
			}
		} else if (strcmp(strlwr(head), "xor") == 0) {
			if (dst[0] == 'r' || dst[0] == 'R') {
				int drn = -1;
				sscanf(dst, "%*[rR]%d", &drn);
				if (drn >= 0 && drn < REG_CNT) {
					if (src[0] == 'r' || src[0] == 'R') {
						int srn = -1;
						sscanf(src, "%*[rR]%d", &srn);
						if (srn >= 0 && srn < REG_CNT) {
							if (reg[drn].type == reg[srn].type) {
								switch (reg[drn].type) {
									case RegChar:
										reg[drn].vChar ^= reg[srn].vChar;
										break;
									case RegFloat:
										return 1;
									case RegInt:
										reg[drn].vInt ^= reg[srn].vInt;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
							} else return 1;
						} else return 1;
					} else {
						if (src[0] == '\'') {
							if (src[strlen(src) - 1] != '\'') return 1;
							char tmp = 0;
							if(sscanf(src, "%*[\']%[^\']c", &tmp) && reg[drn].type == RegChar) {
								reg[drn].vChar ^= tmp;
							} else {
								return 1;
							}
						} else if (src[0] >= '0' && src[0] <= '9' || src[0] == '-' || src[0] == '+') {
							if (src[strlen(src) - 1] == 'F' || src[strlen(src) - 1] == 'f' || 
								strchr(src, '.') > 0) {
								float tmp = 0;
								return 1;
							} else {
								int tmp = 0;
								if (src[1] == 'x' || src[1] == 'X' || 
								src[strlen(src) - 1] == 'h' || src[strlen(src) - 1] == 'H') {
									sscanf(src, "%x", &tmp);
								} else {
									sscanf(src, "%d", &tmp);
								}
								if (reg[drn].type == RegInt) {
									reg[drn].vInt ^= tmp;
								} else {
									return 1;
								}
							}
						} else {
							Register* r = mm->get(mm->p, src);
							if (r == 0) return 1;
							if (reg[drn].type == r->type) {
								switch (reg[drn].type) {
									case RegChar:
										reg[drn].vChar ^= r->vChar;
										break;
									case RegFloat:
										return 1;
									case RegInt:
										reg[drn].vInt ^= r->vInt;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
							} else return 1;
						}
					}
				} else return 1;
			} else {
				Register* dr = mm->get(mm->p, dst);
				if (dr == 0) return 1;
				if (src[0] == 'r' || src[0] == 'R') {
					int srn = -1;
					sscanf(src, "%*[rR]%d", &srn);
					if (srn >= 0 && srn < REG_CNT) {
						if (dr->type == reg[srn].type) {
							switch (dr->type) {
								case RegChar:
									dr->vChar ^= reg[srn].vChar;
									break;
								case RegFloat:
									return 1;
								case RegInt:
									dr->vInt ^= reg[srn].vInt;
									break;
								case RegPtr:
									return 1;
								default:
									return 1;
							}
						} else return 1;
					} else return 1;
				} else {
					if (src[0] == '\'') {
						if (src[strlen(src) - 1] != '\'') return 1;
						char tmp = 0;
						if(sscanf(src, "%*[\']%[^\']c", &tmp) && dr->type == RegChar) {
							dr->vChar ^= tmp;
						} else {
							return 1;
						}
					} else if (src[0] >= '0' && src[0] <= '9' || src[0] == '-' || src[0] == '+') {
						if (src[strlen(src) - 1] == 'F' || src[strlen(src) - 1] == 'f' || 
							strchr(src, '.') > 0) {
							float tmp = 0;
							return 1;
						} else {
							int tmp = 0;
							if (src[1] == 'x' || src[1] == 'X' || 
							src[strlen(src) - 1] == 'h' || src[strlen(src) - 1] == 'H') {
								sscanf(src, "%x", &tmp);
							} else {
								sscanf(src, "%d", &tmp);
							}
							if (dr->type == RegInt) {
								dr->vInt ^= tmp;
							} else {
								return 1;
							}
						}
					} else {
						Register* r = mm->get(mm->p, src);
						if (r == 0) return 1;
						if (dr->type == r->type) {
							switch (dr->type) {
								case RegChar:
									dr->vChar ^= r->vChar;
									break;
								case RegFloat:
									return 1;
								case RegInt:
									dr->vInt ^= r->vInt;
									break;
								case RegPtr:
									return 1;
								default:
									return 1;
							}
						} else return 1;
					}
				}
			}
		} else if (strcmp(strlwr(head), "not") == 0) {
			if (dst[0] == 'r' || dst[0] == 'R') {
				int drn = -1;
				sscanf(dst, "%*[rR]%d", &drn);
				if (drn >= 0 && drn < REG_CNT) {
					switch (reg[drn].type) {
						case RegChar:
							reg[drn].vChar = ~reg[drn].vChar;
							break;
						case RegFloat:
							return 1;
						case RegInt:
							reg[drn].vInt = ~reg[drn].vInt;
							break;
						case RegPtr:
							return 1;
						default:
							return 1;
					}
				} else return 1;
			} else {
				Register* dr = mm->get(mm->p, dst);
				if (dr == 0) return 1;
				switch (dr->type) {
					case RegChar:
						dr->vChar = ~dr->vChar;
						break;
					case RegFloat:
						return 1;
					case RegInt:
						dr->vInt = ~dr->vInt;
						break;
					case RegPtr:
						return 1;
					default:
						return 1;
				}
			}
		} else if (strcmp(strlwr(head), "shl") == 0) {
			if (dst[0] == 'r' || dst[0] == 'R') {
				int drn = -1;
				sscanf(dst, "%*[rR]%d", &drn);
				if (drn >= 0 && drn < REG_CNT) {
					if (src[0] == 'r' || src[0] == 'R') {
						int srn = -1;
						sscanf(src, "%*[rR]%d", &srn);
						if (srn >= 0 && srn < REG_CNT) {
							switch (reg[drn].type) {
								case RegChar:
									switch (reg[srn].type) {
										case RegChar:
											reg[drn].vChar = reg[drn].vChar << reg[srn].vChar;
											break;
										case RegFloat:
											return 1;
										case RegInt:
											reg[drn].vChar = reg[drn].vChar << reg[srn].vInt;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegFloat:
									return 1;
								case RegInt:
									switch (reg[srn].type) {
										case RegChar:
											reg[drn].vInt = reg[drn].vInt << reg[srn].vChar;
											break;
										case RegFloat:
											return 1;
										case RegInt:
											reg[drn].vInt = reg[drn].vInt << reg[srn].vInt;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegPtr:
									return 1;
								default:
									return 1;
							}
						} else return 1;
					} else {
						if (src[0] == '\'') {
							if (src[strlen(src) - 1] != '\'') return 1;
							char tmp = 0;
							if(sscanf(src, "%*[\']%[^\']c", &tmp)) {
								switch (reg[drn].type) {
									case RegChar:
										reg[drn].vChar = reg[drn].vChar << tmp;
										break;
									case RegFloat:
										return 1;
									case RegInt:
										reg[drn].vInt = reg[drn].vInt << tmp;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
							} else {
								return 1;
							}
						} else if (src[0] >= '0' && src[0] <= '9' || src[0] == '-' || src[0] == '+') {
							if (src[strlen(src) - 1] == 'F' || src[strlen(src) - 1] == 'f' || 
								strchr(src, '.') > 0) {
								float tmp = 0;
								return 1;
							} else {
								int tmp = 0;
								if (src[1] == 'x' || src[1] == 'X' || 
								src[strlen(src) - 1] == 'h' || src[strlen(src) - 1] == 'H') {
									sscanf(src, "%x", &tmp);
								} else {
									sscanf(src, "%d", &tmp);
								}
								switch (reg[drn].type) {
									case RegChar:
										reg[drn].vChar = reg[drn].vChar << tmp;
										break;
									case RegFloat:
										return 1;
									case RegInt:
										reg[drn].vInt = reg[drn].vInt << tmp;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
							}
						} else {
							Register* r = mm->get(mm->p, src);
							if (r == 0) return 1;
							switch (reg[drn].type) {
								case RegChar:
									switch (r->type) {
										case RegChar:
											reg[drn].vChar = reg[drn].vChar << r->vChar;
											break;
										case RegFloat:
											return 1;
										case RegInt:
											reg[drn].vChar = reg[drn].vChar << r->vInt;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegFloat:
									return 1;
								case RegInt:
									switch (r->type) {
										case RegChar:
											reg[drn].vInt = reg[drn].vInt << r->vChar;
											break;
										case RegFloat:
											return 1;
										case RegInt:
											reg[drn].vInt = reg[drn].vInt << r->vInt;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegPtr:
									return 1;
								default:
									return 1;
							}
						}
					}
				} else return 1;
			} else {
				Register* dr = mm->get(mm->p, dst);
				if (dr == 0) return 1;
				if (src[0] == 'r' || src[0] == 'R') {
					int srn = -1;
					sscanf(src, "%*[rR]%d", &srn);
					if (srn >= 0 && srn < REG_CNT) {
						switch (dr->type) {
							case RegChar:
								switch (reg[srn].type) {
									case RegChar:
										dr->vChar = dr->vChar << reg[srn].vChar;
										break;
									case RegFloat:
										return 1;
									case RegInt:
										dr->vChar = dr->vChar << reg[srn].vInt;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegFloat:
								return 1;
							case RegInt:
								switch (reg[srn].type) {
									case RegChar:
										dr->vInt = dr->vInt << reg[srn].vChar;
										break;
									case RegFloat:
										return 1;
									case RegInt:
										dr->vInt = dr->vInt << reg[srn].vInt;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegPtr:
								return 1;
							default:
								return 1;
						}
					} else return 1;
				} else {
					if (src[0] == '\'') {
						if (src[strlen(src) - 1] != '\'') return 1;
						char tmp = 0;
						if(sscanf(src, "%*[\']%[^\']c", &tmp)) {
							switch (dr->type) {
								case RegChar:
									dr->vChar = dr->vChar << tmp;
									break;
								case RegFloat:
									return 1;
								case RegInt:
									dr->vInt = dr->vInt << tmp;
									break;
								case RegPtr:
									return 1;
								default:
									return 1;
							}
						} else {
							return 1;
						}
					} else if (src[0] >= '0' && src[0] <= '9' || src[0] == '-' || src[0] == '+') {
						if (src[strlen(src) - 1] == 'F' || src[strlen(src) - 1] == 'f' || 
							strchr(src, '.') > 0) {
							float tmp = 0;
							return 1;
						} else {
							int tmp = 0;
							if (src[1] == 'x' || src[1] == 'X' || 
							src[strlen(src) - 1] == 'h' || src[strlen(src) - 1] == 'H') {
								sscanf(src, "%x", &tmp);
							} else {
								sscanf(src, "%d", &tmp);
							}
							switch (dr->type) {
								case RegChar:
									dr->vChar = dr->vChar << tmp;
									break;
								case RegFloat:
									return 1;
								case RegInt:
									dr->vInt = dr->vInt << tmp;
									break;
								case RegPtr:
									return 1;
								default:
									return 1;
							}
						}
					} else {
						Register* r = mm->get(mm->p, src);
						if (r == 0) return 1;
						switch (dr->type) {
							case RegChar:
								switch (r->type) {
									case RegChar:
										dr->vChar = dr->vChar << r->vChar;
										break;
									case RegFloat:
										return 1;
									case RegInt:
										dr->vChar = dr->vChar << r->vInt;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegFloat:
								return 1;
							case RegInt:
								switch (r->type) {
									case RegChar:
										dr->vInt = dr->vInt << r->vChar;
										break;
									case RegFloat:
										return 1;
									case RegInt:
										dr->vInt = dr->vInt << r->vInt;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegPtr:
								return 1;
							default:
								return 1;
						}
					}
				}
			}
		} else if (strcmp(strlwr(head), "shr") == 0) {
			if (dst[0] == 'r' || dst[0] == 'R') {
				int drn = -1;
				sscanf(dst, "%*[rR]%d", &drn);
				if (drn >= 0 && drn < REG_CNT) {
					if (src[0] == 'r' || src[0] == 'R') {
						int srn = -1;
						sscanf(src, "%*[rR]%d", &srn);
						if (srn >= 0 && srn < REG_CNT) {
							switch (reg[drn].type) {
								case RegChar:
									switch (reg[srn].type) {
										case RegChar:
											reg[drn].vChar = reg[drn].vChar >> reg[srn].vChar;
											break;
										case RegFloat:
											return 1;
										case RegInt:
											reg[drn].vChar = reg[drn].vChar >> reg[srn].vInt;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegFloat:
									return 1;
								case RegInt:
									switch (reg[srn].type) {
										case RegChar:
											reg[drn].vInt = reg[drn].vInt >> reg[srn].vChar;
											break;
										case RegFloat:
											return 1;
										case RegInt:
											reg[drn].vInt = reg[drn].vInt >> reg[srn].vInt;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegPtr:
									return 1;
								default:
									return 1;
							}
						} else return 1;
					} else {
						if (src[0] == '\'') {
							if (src[strlen(src) - 1] != '\'') return 1;
							char tmp = 0;
							if(sscanf(src, "%*[\']%[^\']c", &tmp)) {
								switch (reg[drn].type) {
									case RegChar:
										reg[drn].vChar = reg[drn].vChar >> tmp;
										break;
									case RegFloat:
										return 1;
									case RegInt:
										reg[drn].vInt = reg[drn].vInt >> tmp;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
							} else {
								return 1;
							}
						} else if (src[0] >= '0' && src[0] <= '9' || src[0] == '-' || src[0] == '+') {
							if (src[strlen(src) - 1] == 'F' || src[strlen(src) - 1] == 'f' || 
								strchr(src, '.') > 0) {
								float tmp = 0;
								return 1;
							} else {
								int tmp = 0;
								if (src[1] == 'x' || src[1] == 'X' || 
								src[strlen(src) - 1] == 'h' || src[strlen(src) - 1] == 'H') {
									sscanf(src, "%x", &tmp);
								} else {
									sscanf(src, "%d", &tmp);
								}
								switch (reg[drn].type) {
									case RegChar:
										reg[drn].vChar = reg[drn].vChar >> tmp;
										break;
									case RegFloat:
										return 1;
									case RegInt:
										reg[drn].vInt = reg[drn].vInt >> tmp;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
							}
						} else {
							Register* r = mm->get(mm->p, src);
							if (r == 0) return 1;
							switch (reg[drn].type) {
								case RegChar:
									switch (r->type) {
										case RegChar:
											reg[drn].vChar = reg[drn].vChar >> r->vChar;
											break;
										case RegFloat:
											return 1;
										case RegInt:
											reg[drn].vChar = reg[drn].vChar >> r->vInt;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegFloat:
									return 1;
								case RegInt:
									switch (r->type) {
										case RegChar:
											reg[drn].vInt = reg[drn].vInt >> r->vChar;
											break;
										case RegFloat:
											return 1;
										case RegInt:
											reg[drn].vInt = reg[drn].vInt >> r->vInt;
											break;
										case RegPtr:
											return 1;
										default:
											return 1;
									}
									break;
								case RegPtr:
									return 1;
								default:
									return 1;
							}
						}
					}
				} else return 1;
			} else {
				Register* dr = mm->get(mm->p, dst);
				if (dr == 0) return 1;
				if (src[0] == 'r' || src[0] == 'R') {
					int srn = -1;
					sscanf(src, "%*[rR]%d", &srn);
					if (srn >= 0 && srn < REG_CNT) {
						switch (dr->type) {
							case RegChar:
								switch (reg[srn].type) {
									case RegChar:
										dr->vChar = dr->vChar >> reg[srn].vChar;
										break;
									case RegFloat:
										return 1;
									case RegInt:
										dr->vChar = dr->vChar >> reg[srn].vInt;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegFloat:
								return 1;
							case RegInt:
								switch (reg[srn].type) {
									case RegChar:
										dr->vInt = dr->vInt >> reg[srn].vChar;
										break;
									case RegFloat:
										return 1;
									case RegInt:
										dr->vInt = dr->vInt >> reg[srn].vInt;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegPtr:
								return 1;
							default:
								return 1;
						}
					} else return 1;
				} else {
					if (src[0] == '\'') {
						if (src[strlen(src) - 1] != '\'') return 1;
						char tmp = 0;
						if(sscanf(src, "%*[\']%[^\']c", &tmp)) {
							switch (dr->type) {
								case RegChar:
									dr->vChar = dr->vChar >> tmp;
									break;
								case RegFloat:
									return 1;
								case RegInt:
									dr->vInt = dr->vInt >> tmp;
									break;
								case RegPtr:
									return 1;
								default:
									return 1;
							}
						} else {
							return 1;
						}
					} else if (src[0] >= '0' && src[0] <= '9' || src[0] == '-' || src[0] == '+') {
						if (src[strlen(src) - 1] == 'F' || src[strlen(src) - 1] == 'f' || 
							strchr(src, '.') > 0) {
							float tmp = 0;
							return 1;
						} else {
							int tmp = 0;
							if (src[1] == 'x' || src[1] == 'X' || 
							src[strlen(src) - 1] == 'h' || src[strlen(src) - 1] == 'H') {
								sscanf(src, "%x", &tmp);
							} else {
								sscanf(src, "%d", &tmp);
							}
							switch (dr->type) {
								case RegChar:
									dr->vChar = dr->vChar >> tmp;
									break;
								case RegFloat:
									return 1;
								case RegInt:
									dr->vInt = dr->vInt >> tmp;
									break;
								case RegPtr:
									return 1;
								default:
									return 1;
							}
						}
					} else {
						Register* r = mm->get(mm->p, src);
						if (r == 0) return 1;
						switch (dr->type) {
							case RegChar:
								switch (r->type) {
									case RegChar:
										dr->vChar = dr->vChar >> r->vChar;
										break;
									case RegFloat:
										return 1;
									case RegInt:
										dr->vChar = dr->vChar >> r->vInt;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegFloat:
								return 1;
							case RegInt:
								switch (r->type) {
									case RegChar:
										dr->vInt = dr->vInt >> r->vChar;
										break;
									case RegFloat:
										return 1;
									case RegInt:
										dr->vInt = dr->vInt >> r->vInt;
										break;
									case RegPtr:
										return 1;
									default:
										return 1;
								}
								break;
							case RegPtr:
								return 1;
							default:
								return 1;
						}
					}
				}
			}
		} else if (strcmp(strlwr(head), "end") == 0) {
			return 0;
		} else if (strcmp(strlwr(head), "nop") == 0) {
		#ifdef WINDOWS
			__asm { nop; }
		#else
			asm("nop");
		#endif
		} else if (strcmp(strlwr(head), "rst") == 0) {
			
		} else if (strcmp(strlwr(head), "rem") == 0) {
			//just rem
		} else if (head[0] == '[' && head[strlen(head) - 1] == ']') {
			//jump tag
		} else {
			return 1;
		}
	}
	return 0;
}

void compile(char* var) {
	if (var == 0) return;
	print("Note: Compile module is in coding.\n\n");
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
	mm = InitMemoryManager(stackSiz, heapSiz);
	
	if (data != 0) {
		int dataLines = lines(data);
		print("DATA: %d line(s), loading...\n", dataLines);
		for (int i = 0; i < dataLines; i++)
			if (execute(line(data, i), 'd')) {
				print("\nNSASM running error!\n");
				print("At [DATA] line %d: %s\n\n", i + 1, line(data, i));
				return;
			}
	}
	
	if (code != 0) {
		cnt = 0; tagBuf = "";
		int prev = 0;
		int codeLines = lines(code);
		print("CODE: %d line(s), running...\n\n", codeLines);
		for (; cnt < codeLines; cnt++) {
			prev = cnt;
			if (execute(line(code, cnt), 'c')) {
				print("\nNSASM running error!\n");
				print("At line %d: %s\n\n", prev + 1, line(code, prev));
				return;
			}
			cnt = jump(code , tagBuf);
			tagBuf = "";
			if (cnt >= codeLines) {
				print("\nNSASM running error!\n");
				print("At [CODE] line %d: %s\n\n", prev + 1, line(code, prev));
				return;
			}
		}
	}
	
	print("\nNSASM running finished.\n\n");
}

int jump(char* src, char* tag) {
	int len = lines(src);
	if (strlen(tag) == 0) return cnt;
	for (int i = 0; i < len; i++) {
		if (strcmp(line(src, i), tag) == 0) return i;
	}
	return cnt;
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
						if (i == srcLen) return 0;
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
	return 0;
}

int lines(char* src) {
	if(src == 0) return 0;
	int cnt = 0, length = strlen(src);
	for (int i = 0; i < length; i++)
		if (src[i] == '\n') cnt += 1;
	return cnt;
}

char* line(char* src, int index) {
	if (index >= lines(src)) return 0;
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
	return 0;
}

