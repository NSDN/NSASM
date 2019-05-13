#ifndef __UTIL_H_
#define __UTIL_H_


char* read(char* path);
int lines(char* src);
char* line(char* src, int index);
char* cut(char* src, const char* head);
char* get(char* src, int start, char* buf, int size);


#endif
