#include <stdint.h>
#include <arch/bsp/timer.h>
#include <arch/bsp/intr.h>
#include <arch/cpu/arm.h>
#include <kernel/thread.h>
#include <kernel/kprintf.h>
#include <lib/assert.h>
#include <lib/time.h>

/*
 * Device driver for Systen timer
 * Datasheet: https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf
 * Page: From 172
*/

struct time_tr {
    uint32_t cs;    /* System Timer Control/Status */
    uint32_t clo;   /* System Timer Counter Lower 32 bits */
    uint32_t chi;   /* System Timer Counter Higher 32 bits */
    uint32_t c[4];  /* System Timer Compare 0-3 */
};

/* offset of 0x200 given in table on datasheet page 112 */
struct time_tr* timer_dev = (struct time_tr*) (TIMER_BASE);
uint32_t c_user_values[NUM_TIMERS];
void (*timer_callbacks[NUM_TIMERS])(void * args);

/*
 * public function defintions
*/

void setup_timer(uint32_t timer_num, uint32_t compare_value, void (*callback)(void * arg))
{
    assert(timer_num < NUM_TIMERS);
    
    timer_dev->c[timer_num] = timer_dev->clo + compare_value;
    c_user_values[timer_num] = compare_value;
    interrupt_enable(IRQ_TIMER_BASE + timer_num, 0);

    timer_callbacks[timer_num] = callback;
}

void timer_intr_h(struct registers_t *reg) 
{
    /* find interrupting timer */
    int32_t timer_match = -1;
    uint32_t i=0;
    uint8_t timer_found = 0;

    while (!timer_found) {
        if (timer_dev->cs & (1<<i)) {
            timer_found = 1;
            timer_match = i;
        }
        i++;
    }

    assert(timer_match >= 0);
    assert(timer_match < NUM_TIMERS);

    /* reset interrupt */
    timer_dev->cs = 1<<timer_match;
    timer_dev->c[timer_match] += c_user_values[timer_match];

    /* handler */
    if (timer_match == SCHEDULER_TIMER)
        timer_callbacks[timer_match]((void *) reg);
    else 
        timer_callbacks[timer_match]((void *) 0);
}

void timer_get_counter(uint32_t * high, uint32_t * low)
{
    *high = timer_dev->chi;
    *low = timer_dev->clo;
}