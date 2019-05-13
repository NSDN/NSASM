#include "base.h"

extern char* strlwr(char* s);

#if defined(ARDUINO)

void clearScreen() { 
	tft.setCursor(0, 0);
	tft.fillScreen(BACKCOLOR);
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
	char count = 0;
	while (true) {
		if (Serial.available() > 0) {
			if (Serial.peek() == '\n') break;
			buffer[count] = Serial.read();
			if (buffer[count] == 0x08 && count > 0) {
				count -= 1;
				print("%c", 0x08);
				continue;
			}
			else if (buffer[count] != 0x08) {
				print("%c", buffer[count]);
				count += 1;
			}
		}
		else if (Serial1.available() > 0) {
			if (Serial1.peek() == '\n') break;
			buffer[count] = Serial1.read();
			if (buffer[count] == 0x08 && count > 0) {
				count -= 1;
				print("%c", 0x08);
				continue;
			}
			else if (buffer[count] != 0x08) {
				print("%c", buffer[count]);
				count += 1;
			}
		}
	}
	buffer[count] = '\0';
	if (Serial.available() > 0) Serial.read();
	else if (Serial1.available() > 0) Serial1.read();
	print("\n");
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
