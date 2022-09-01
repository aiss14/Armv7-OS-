#ifndef DEBUG_H
#define DEBUG_H

#include <config.h>
#include <kernel/kprintf.h>

#define DEBUG_LEVEL_ERROR   1
#define DEBUG_LEVEL_WARN    2
#define DEBUG_LEBEL_INFO    3

#ifdef DEBUG_LEVEL
    #if DEBUG_LEVEL >= DEBUG_LEVEL_ERROR
        #define ERROR(msg)  kprintf("ERROR: "); kprintf(msg);
    #else
        #define ERROR(msg)
    #endif

    #if DEBUG_LEVEL >= DEBUG_LEVEL_WARN
        #define WARN(msg)  kprintf("WARN: "); kprintf(msg);
    #else
        #define WARN(msg)
    #endif

    #if DEBUG_LEVEL >= DEBUG_LEBEL_INFO
        #define INFO(msg)  kprintf("INFO: "); kprintf(msg);
    #else
        #define INFO(msg)
    #endif

#else // DEBUG_LEVEL
    #define ERROR(msg)
    #define WARN(msg)
    #define INFO(msg)
#endif // DEBUG_LEVEL

#endif // DEBUG_H