#include "base.h"

extern int APP_NAME(int argc, char* argv[]);

extern char* strlwr(char* s);

#if defined(LINUX)

void clearScreen() {
	system("clear");
}

int print(const char* format, ...) {
	va_list args;
	va_start(args, format);
	int result = vprintf(format, args);
	va_end(args);
	return result;
}

int scan(char* buffer) {
	char count = 0, tmp = 0;
	while (1) {
		tmp = getchar();
		if (tmp == '\n') break;
		buffer[count] = tmp;
		if (buffer[count] == 0x08 && count > 0) {
			count -= 1;
			#ifdef BASE_IO
				print("%c", 0x08);
			#endif
			continue;
		}
		else if (buffer[count] != 0x08) {
			#ifdef BASE_IO
				print("%c", buffer[count]);
			#endif
			count += 1;
		}
	}
	buffer[count] = '\0';
	#ifdef BASE_IO
		print("\n");
	#endif
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

#endif
