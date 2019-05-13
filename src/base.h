#ifndef __BASE_H_
#define __BASE_H_


//#define WINDOWS
#define LINUX
//#define ARDUINO
//#define STM32

#if defined(WINDOWS)
#define _CRT_SECURE_NO_WARNINGS
#else
char* strlwr(char* s);
#endif

#include <stdio.h>
#include <string.h>

#if defined(WINDOWS)
#include <stdlib.h>
#include <vadefs.h>

#elif defined(LINUX)
#include <stdlib.h>
#include <stdarg.h>
//#define BASE_IO

#elif defined(ARDUINO)
#include <stdarg.h>
#define BACKCOLOR 0x0000
#define __print(buf) tft.print(buf)

#elif defined(STM32)
#include <stdarg.h>
#define HUART huart2
#define __print(buf) lcd->printfa(lcd->p, buf)

#endif

#ifndef OK
#define OK 0
#endif

#ifndef ERR
#define ERR 1
#endif

#ifndef ETC
#define ETC -1
#endif

#define APP_NAME nsasm
#define IOBUF 128

void clearScreen();
int print(const char* format, ...);
int scan(char* buffer);
int fscan(char* buffer, const char* format, ...);


#endif
