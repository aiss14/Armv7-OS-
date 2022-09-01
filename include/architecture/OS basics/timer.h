#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include <arch/cpu/arm.h>
#include <arch/cpu/mm.h>

#define NUM_TIMERS  4
#define TIMER_BASE       (0x7E003000 - PERIPH_OFFSET)

void setup_timer(uint32_t timer_num, uint32_t compare_value, void (*callback)(void * arg));
void timer_intr_h(struct registers_t *reg);
void timer_get_counter(uint32_t * high, uint32_t * low);
void ksleep(uint32_t micros);

#endif // TIMER_H
