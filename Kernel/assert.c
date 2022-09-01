#include <lib/assert.h>
#include <kernel/kprintf.h>

void __assert_func(const char * file, int line, const char * function, \
                                const char * stmt) 
{
    kprintf("Assertion failed! \n\tFunction: %s\n\tin %s: %i\n\tStatement:%s\n", function, file, line, stmt);
}