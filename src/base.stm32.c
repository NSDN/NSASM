#include "base.h"

extern char* strlwr(char* s);

#if defined(STM32)

void clearScreen() {
	lcd->clear(lcd->p);
}

int print(const char* format, ...) {
	char* iobuf = malloc(sizeof(char) * IOBUF);
	va_list args;
	va_start(args, format);
	int result = vsprintf(iobuf, format, args);
	va_end(args);
	__print(iobuf);
	free(iobuf);
	return result;
}

int scan(char* buffer) {
	char count = 0, tmp = '\0';
	while (1) {
		if (HAL_UART_Receive(&HUART, &tmp, 1, 1) == HAL_OK) {
			if (tmp == '\n') break;
			buffer[count] = tmp;
			if (buffer[count] == 0x08 && count > 0) {
				count -= 1;
				print("%c", 0x08);
				continue;
			} else if (buffer[count] != 0x08) {
				print("%c", buffer[count]);
				count += 1;
			}
		}
	}
	buffer[count] = '\0';
	print("\n");
}

int fscan(char* buffer, const char* format, ...) {
	scan(buffer);
	va_list args;
	va_start(args, format);
	int result = vsscanf(buffer, format, args);
	va_end(args);
	return result;
}

#endif
