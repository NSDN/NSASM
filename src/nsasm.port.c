#include "nsasm.priv.h"

#include "base.h"
#include "memman.h"

#include "util.h"

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

int execute(Instance* inst, char* var, char type) {
	char head[32] = "\0", dst[32] = "\0", src[64] = "\0";
	if (type == 'd') {
		sscanf(var, "%s %[^ \t=] %*[= \t]%[^\n]", head, dst, src);
		int index = getSymbolIndex(getFuncList(), head);
		if (index == ETC) return ERR;
		if (verifyVarName(dst)) return ERR;
		Register dr;
		dr.data.vPtr = malloc(sizeof(char) * (strlen(dst) + 1));
		dr.type = RegPtr;
		strcpy(dr.data.vPtr, dst);
		Register* sr;
		if (getRegister(inst, src, &sr) == ETC) {
			if (getFuncList()[index].fun(inst, &dr, sr)) {
				return ERR;
			}
			free(dr.data.vPtr);
			free(sr);
		} else return ERR;
	} else if (type == 'c') {
		sscanf(var, "%s %[^,] %*[, \t]%[^\n]", head, dst, src);
		int index = getSymbolIndex(getFuncList(), head);
		if (index == ETC) {
			return verifyTag(head);
		}
		Register* dr = 0; Register* sr = 0; int dresult = 0, sresult = 0;
		dresult = getRegister(inst, var + strlen(head) + 1, &dr);
		if (dresult == ETC) {
			dr->readOnly = 1;
		} else {
			if (dresult == ERR) free(dr);
			dresult = getRegister(inst, dst, &dr);
			if (dresult != OK) {
				if (dresult == ETC) {
					dr->readOnly = 1;
				} else if (index < getNoperCount()) {
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
		}
		int result = getFuncList()[index].fun(inst, dr, sr);
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
	print("Now in console mode.\n\n");
	char buf[IOBUF]; int lines = 1, result = 0;
	Instance* instance = NewInstance(16, 32);

	while (1) {
		print("%d >>> ", lines);
		scan(buf);
		if (strlen(buf) == 0) {
			lines += 1;
			continue;
		}
		if (execute(instance, buf, 'd') == ERR) {
			result = execute(instance, buf, 'c');
			if (result == ERR) {
				print("\nNSASM running error!\n");
				print("At line %d: %s\n\n", lines, buf);
			} else if (result == ETC) {
				break;
			}
		}
		lines += 1;
	}
	FreeInstance(instance);
}

void run(char* var) {
	if (var == 0) return;
	char* conf = cut(var, ".conf");
	char* data = cut(var, ".data");
	char* code = cut(var, ".code");
	char* _gcvar = 0;
	free(var);
	
	int stackSiz = 0, heapSiz = 0;
	if (conf != 0) {
		int confLines = lines(conf);
		char type[8] = "", value[8] = "";
		print("CONF: %d line(s), init...\n", confLines);
		for (int i = 0; i < confLines; i++) {
			sscanf(_gcvar = line(conf, i), "%s %[^\n]", type, value);
			free(_gcvar);
			if (strcmp(strlwr(type), "stack") == 0) {
				if (sscanf(value, "%d", &stackSiz) == 0) {
					print("\nNSASM init error!\n");
					print("At [CONF] line %d: %s\n\n", i + 1, _gcvar = line(data, i));
					free(_gcvar);
					return;
				}
			} else if (strcmp(strlwr(type), "heap") == 0) {
				if (sscanf(value, "%d", &heapSiz) == 0) {
					print("\nNSASM init error!\n");
					print("At [CONF] line %d: %s\n\n", i + 1, _gcvar = line(data, i));
					free(_gcvar);
					return;
				}
			} else {
				print("\nNSASM init error!\n");
				print("At [CONF] line %d: %s\n\n", i + 1, _gcvar = line(data, i));
				free(_gcvar);
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
			if (execute(instance, _gcvar = line(data, i), 'd')) {
				free(_gcvar);
				print("\nNSASM running error!\n");
				print("At [DATA] line %d: %s\n\n", i + 1, _gcvar = line(data, i));
				free(_gcvar);
				return;
			} else free(_gcvar);
	}
	
	if (code != 0) {
		int prev = 0, result = 0, codeLines = lines(code);
		print("CODE: %d line(s), running...\n\n", codeLines);
		for (; instance->cnt < codeLines; instance->cnt++) {
			prev = instance->cnt;
			result = execute(instance, _gcvar = line(code, instance->cnt), 'c');
			free(_gcvar);
			if (result == ETC) break;
			if (result) {
				print("\nNSASM running error!\n");
				print("At line %d: %s\n\n", prev + 1, _gcvar = line(code, prev));
				free(_gcvar);
				return;
			}
			for (int i = 0; i < codeLines; i++) {
				if (strcmp(_gcvar = line(code, i), instance->tag) == 0) {
					free(_gcvar);
					for (int j = i + 1; j < codeLines; j++) {
						if (strcmp(_gcvar = line(code, j), instance->tag) == 0) {
							free(_gcvar);
							print("\nNSASM running error!\n");
							print("At  [CODE] line %d: %s\n", i, _gcvar = line(code, i));
							free(_gcvar);
							print("And [CODE] line %d: %s\n\n", j, _gcvar = line(code, j));
							free(_gcvar);
							return;
						} else free(_gcvar);
					}
					instance->cnt = i;
					instance->tag[0] = '\0';
				} else free(_gcvar);
			}
			if (instance->cnt >= codeLines) {
				print("\nNSASM running error!\n");
				print("At [CODE] line %d: %s\n\n", prev + 1, _gcvar = line(code, prev));
				free(_gcvar);
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
	char* _gcvar = 0;
	free(raw);
	
	int stackSiz = 0, heapSiz = 0;
	if (conf != 0) {
		int confLines = lines(conf);
		char type[8] = "", value[8] = "";
		for (int i = 0; i < confLines; i++) {
			sscanf(_gcvar = line(conf, i), "%s %[^\n]", type, value);
			free(_gcvar);
			if (strcmp(strlwr(type), "stack") == 0) {
				if (sscanf(value, "%d", &stackSiz) == 0) {
					print("\nNSASM init error in \"%s\"!\n", var);
					print("At [CONF] line %d: %s\n\n", i + 1, _gcvar = line(data, i));
					free(_gcvar);
					return;
				}
			} else if (strcmp(strlwr(type), "heap") == 0) {
				if (sscanf(value, "%d", &heapSiz) == 0) {
					print("\nNSASM init error in \"%s\"!\n", var);
					print("At [CONF] line %d: %s\n\n", i + 1, _gcvar = line(data, i));
					free(_gcvar);
					return;
				}
			} else {
				print("\nNSASM init error in \"%s\"!\n", var);
				print("At [CONF] line %d: %s\n\n", i + 1, _gcvar = line(data, i));
				free(_gcvar);
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
			if (execute(instance, _gcvar = line(data, i), 'd')) {
				free(_gcvar);
				print("\nNSASM running error in \"%s\"!\n", var);
				print("At [DATA] line %d: %s\n\n", i + 1, _gcvar = line(data, i));
				free(_gcvar);
				return;
			} else free(_gcvar);
	}
	
	if (code != 0) {
		int prev = 0, result = 0, codeLines = lines(code);
		for (; instance->cnt < codeLines; instance->cnt++) {
			prev = instance->cnt;
			result = execute(instance, _gcvar = line(code, instance->cnt), 'c');
			free(_gcvar);
			if (result == ETC) break;
			if (result) {
				print("\nNSASM running error in \"%s\"!\n", var);
				print("At line %d: %s\n\n", prev + 1, _gcvar = line(code, prev));
				free(_gcvar);
				return;
			}
			for (int i = 0; i < codeLines; i++) {
				if (strcmp(_gcvar = line(code, i), instance->tag) == 0) {
					free(_gcvar);
					for (int j = i + 1; j < codeLines; j++) {
						if (strcmp(_gcvar = line(code, j), instance->tag) == 0) {
							free(_gcvar);
							print("\nNSASM running error!\n");
							print("At  [CODE] line %d: %s\n", i, _gcvar = line(code, i));
							free(_gcvar);
							print("And [CODE] line %d: %s\n\n", j, _gcvar = line(code, j));
							free(_gcvar);
							return;
						} else free(_gcvar);
					}
					instance->cnt = i;
					instance->tag[0] = '\0';
				} else free(_gcvar);
			}
			if (instance->cnt >= codeLines) {
				print("\nNSASM running error in \"%s\"!\n", var);
				print("At [CODE] line %d: %s\n\n", prev + 1, _gcvar = line(code, prev));
				free(_gcvar);
				return;
			}
		}
	}
	
	FreeInstance(instance);
	free(conf); free(data); free(code);
}
