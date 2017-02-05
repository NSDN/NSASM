#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VERSION 0.01

typedef enum {
    RegInt,
    RegFloat,
    RegPtr
} RegType;

typedef struct {
    RegType type;
    int vInt;
    float vFloat;
    void* vPtr;
} Register;

int lines(char* src);
char* line(char* src, int index);
char* cut(char* src, const char* head);
char* get(char* src, int start, char* buf, int size);

void compile(char* var);
void run(char* var);

char* read(char* path) {
	FILE* f = fopen(path, "r");
	if (f == 0) {
		printf("Error: File open failed.\n\n");
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
		printf("Error: File open failed.\n\n");
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

int main(int argc, char* argv[]) {
    printf("NyaSama Assembly Script Module\n");
    printf("Version: %1.2f\n\n", VERSION);
    if (argc < 2) {
        printf("Usage: nasm [c/r] [FILE]\n\n");
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

void compile(char* var) {
	if (var == 0) return;
	printf("Note: Compile module is in coding.\n\n");
}

void run(char* var) {
	if (var == 0) return;
	int length = strlen(var);
	char* data = cut(var, ".data");
	char* code = cut(var, ".code");
	printf("DATA: (%d line(s))\n\n%s\n\n", lines(data), data);
	printf("CODE: (%d line(s))\n\n%s\n\n", lines(code), code);
	printf("NASM running finished.\n\n");
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
	int cnt = 0, length = strlen(src);
	for (int i = 0; i < length; i++)
		if (src[i] == '\n') cnt += 1;
	return cnt;
}

char* line(char* src, int index) {
	
}













