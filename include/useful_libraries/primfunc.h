#ifndef PRIMFUNC_H
#define PRIMFUNC_H

#include <stdint.h>

int32_t strlength(const char *str);
int32_t ctoi(char c);
void kmemcpy(void * dest, const void * src, uint32_t size);

#endif