#include "nsasm.priv.h"

#include "base.h"
#include "memman.h"

#include "util.h"

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
	if (strchr(var, ',') > 0) {
		if (var[0] != '\'' && var[0] != '\"') return ERR;
	}

	if (var[0] == 'r' || var[0] == 'R') {
		int srn = -1;
		if (sscanf(var, "%*[rR]%d", &srn) == 0) return ERR;
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
				if(sscanf(var, "%*[\"]%[^\"]s", buf)) {
					int bufLen = strlen(buf);
					(*ptr)->data.vPtr = malloc(sizeof(char) * (bufLen * (repeat + 1)));
					(*ptr)->data.vPtr[0] = '\0';
					for (int i = 0; i < repeat; i++) {
						strcat((*ptr)->data.vPtr, buf);
					}
					free(buf);
				} else {
					free(buf);
					return ERR;
				}
			}
			(*ptr)->type = RegPtr;
			(*ptr)->readOnly = 1;
			return ETC;
		} else if (!(var[0] < '0' || var[0] > '9') || var[0] == '-' || var[0] == '+' ||
					var[strlen(var) - 1] == 'h' || var[strlen(var) - 1] == 'H') {
			if ((
					(var[strlen(var) - 1] == 'F' || var[strlen(var) - 1] == 'f') && 
					(var[1] != 'x' && var[1] != 'X')
				) || strchr(var, '.') > 0) {
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
			char* tmp = malloc(sizeof(char) * strlen(var));
			sscanf(var, "%[^ \t]s", tmp);
			Register* r = inst->mm->get(inst->mm->p, tmp);
			free(tmp);
			if (r == 0) return ERR;
			*ptr = r;
			return OK;
		}
	}
}
