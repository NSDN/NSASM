#include "base.h"

extern int APP_NAME(int argc, char* argv[]);

#if defined(WINDOWS)

void clearScreen() {
	system("cls");
}

int print(const char* format, ...) {
	va_list args = 0;
	__crt_va_start(args, format);
	int result = vprintf(format, args);
	__crt_va_end(args);
	return result;
}

int scan(char* buffer) {
	return gets(buffer);
}

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

#endif
