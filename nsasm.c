#if defined(WINDOWS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <string.h>

/* ---- DEFINE ---- */
//#define WINDOWS
#define LINUX
//#define ARDUINO
//#define STM32
/* ---- DEFINE ---- */

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

#define VERSION 0.01

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

Register reg[8], state;
int cnt;

int lines(char* src);
char* line(char* src, int index);
char* cut(char* src, const char* head);
char* get(char* src, int start, char* buf, int size);

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
		if (strcmp(strlwr(head), "int") == 0) {
			
		} else if (strcmp(strlwr(head), "char") == 0) {
			
		} else if (strcmp(strlwr(head), "float") == 0) {
			
		} else if (strcmp(strlwr(head), "str") == 0) {
			
		} else if (strcmp(strlwr(head), "var") == 0) {
			
		} else {
			return 1;
		}
	} else if (type == 'c') {
		sscanf(var, "%s %[^ \t,] %*[, \t]%[^\n]", head, dst, src);
		if (strcmp(strlwr(head), "mov") == 0) {
			
		} else if (strcmp(strlwr(head), "push") == 0) {
			
		} else if (strcmp(strlwr(head), "pop") == 0) {
			
		} else if (strcmp(strlwr(head), "pusha") == 0) {
			
		} else if (strcmp(strlwr(head), "popa") == 0) {
			
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
							
					}
					break;
				default:
					return 1;
			}
		} else if (strcmp(strlwr(head), "add") == 0) {
			
		} else if (strcmp(strlwr(head), "inc") == 0) {
			
		} else if (strcmp(strlwr(head), "sub") == 0) {
			
		} else if (strcmp(strlwr(head), "dec") == 0) {
			
		} else if (strcmp(strlwr(head), "mul") == 0) {
			
		} else if (strcmp(strlwr(head), "div") == 0) {
			
		} else if (strcmp(strlwr(head), "cmp") == 0) {
			
		} else if (strcmp(strlwr(head), "jmp") == 0) {
			
		} else if (strcmp(strlwr(head), "jz") == 0) {
			
		} else if (strcmp(strlwr(head), "jnz") == 0) {
			
		} else if (strcmp(strlwr(head), "jg") == 0) {
			
		} else if (strcmp(strlwr(head), "jl") == 0) {
			
		} else if (strcmp(strlwr(head), "and") == 0) {
			
		} else if (strcmp(strlwr(head), "or") == 0) {
			
		} else if (strcmp(strlwr(head), "xor") == 0) {
			
		} else if (strcmp(strlwr(head), "not") == 0) {
			
		} else if (strcmp(strlwr(head), "shl") == 0) {
			
		} else if (strcmp(strlwr(head), "shr") == 0) {
			
		} else if (strcmp(strlwr(head), "end") == 0) {
			return 0;
		} else if (strcmp(strlwr(head), "nop") == 0) {
			asm("nop");
		} else if (strcmp(strlwr(head), "rst") == 0) {
			
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
	char* data = cut(var, ".data");
	char* code = cut(var, ".code");
	
	if (data != 0) {
		int dataLines = lines(data);
		print("DATA: %d line(s), init...\n", dataLines);
		for (int i = 0; i < dataLines; i++)
			if (execute(line(data, i), 'd')) {
				print("\nNSASM running error!\n");
				print("\tAt line %d: %s\n\n", i + 1, line(data, i));
				return;
			}
	}
	
	if (code != 0) {
		cnt = 0;
		int prev = 0;
		int codeLines = lines(code);
		print("CODE: %d line(s), running...\n\n", codeLines);
		for (; cnt < codeLines; cnt++) {
			prev = cnt;
			if (execute(line(code, cnt), 'c')) {
				print("\nNSASM running error!\n");
				print("\tAt line %d: %s\n\n", prev + 1, line(code, prev));
				return;
			}
			if (cnt >= codeLines) {
				print("\nNSASM running error!\n");
				print("\tAt line %d: %s\n\n", prev + 1, line(code, prev));
				return;
			}
		}
	}
	
	print("\nNSASM running finished.\n\n");
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

