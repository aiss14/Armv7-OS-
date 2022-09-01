#ifndef THREAD_H
#define THREAD_H

#include <stdint.h>
#include <arch/cpu/arm.h>

#define SCHEDULER_TIMER 3

void init_threads(void);
void kthread_create(struct registers_t *reg, void(*func)(void*), const void *args, uint32_t args_size, 
    uint8_t is_proc // whether the new thread shall open a new address space
    );
void terminate_current_thread(struct registers_t *reg);
void start_scheduling(void);

/* thread_received_char is called, when ta thread needs
to wair for  a character 
returns 0 if thread is waiting
returns 1 if uart device is busy */
uint8_t thread_wait_for_char(struct registers_t * reg);

/* thread_received_char is called, when the uart device 
receives a character */
void thread_process_char_received(struct registers_t * reg);

/* sends the current thread to sleep for the given amount
of milliseconds*/
void thread_make_sleep_current(struct registers_t * reg, uint32_t millis);

#endif // THREAD_H