#ifndef __NSASM_PUB_H_
#define __NSASM_PUB_H_


#define VERSION "0.21r"

#ifndef OK
#define OK 0
#endif

#ifndef ERR
#define ERR 1
#endif

#ifndef ETC
#define ETC -1
#endif

typedef enum {
	RegInt,
	RegFloat,
	RegChar,
	RegPtr
} RegType;

typedef struct {
	char readOnly;
	RegType type;
	union {
		char vChar;
		int vInt;
		float vFloat;
		char* vPtr;
	} data;
} Register;


#endif
