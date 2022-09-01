#ifndef TIME_H
#define TIME_H

#include <stdint.h>

typedef uint64_t time_t;

time_t get_current_time();
void ksleep(uint32_t micros);

#endif // TIME_H