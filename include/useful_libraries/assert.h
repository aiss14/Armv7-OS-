#ifndef ASSERT_H
#define ASSERT_H

#include <config.h>

#ifdef ASSERT_ENABLE
    #define assert(__e) ((__e) ? (void)0 : __assert_func (__FILE__, __LINE__, \
						       __func__, #__e))
#else
    #define assert(__e) ((void)0)
#endif // ASSERT_ENABLE


void __assert (const char *, int, const char *);
void __assert_func (const char *, int, const char *, const char *);

#endif // ASSERT_H