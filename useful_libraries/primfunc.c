#include <lib/primfunc.h>
#include <lib/math.h>

int32_t strlength(const char * str){
    int32_t res=0;
    if(!str) return -1;
    while(*str != '\0'){
        res++;
        str++;
    }
    return res;
}

int32_t ctoi(char c){
    /* returns integer that is described by c.
    If c does not represent a digit, -1 is returned */
    if ((c<'0') || (c>'9'))
        return -1;

    return (int32_t) c-'0'; 
}




void kmemcpy(void * dest, const void * src, uint32_t size)
{
	for (uint32_t i=0; i<size; i++) {
        *((char*)dest) = *((char*)src);
        dest++;
        src++;
	}
}