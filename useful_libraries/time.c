#include <arch/bsp/timer.h>
#include <lib/time.h>

time_t get_current_time()
{
    uint32_t timer_high, timer_low;
    time_t return_time;
    timer_get_counter(&timer_high, &timer_low);

    return_time = timer_high;
    return_time <<= 32;
    return_time |= timer_low;

    return return_time;
}

void ksleep(uint32_t micros)
{
    time_t end_time = get_current_time() + micros;

    while (get_current_time() < end_time)
        continue;
}