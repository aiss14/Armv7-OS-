#ifndef SYS_H
#define SYS_H

#include <stdint.h>

/*
This library provides functions to execute system calls.
Syscalls are called like usual C Funtions: arguments are
put in the registers r0-r3. The kernel evaluates the register
values during the syscall and executes the corresponding
kernel function. Return values are written into the r0
register.
*/

/*
Removes the current thread from the scheduler and gives frees
its resources for new threads. 
*/
void exit(void) __attribute__((weak));

/*
Creates a new thread with the following arguments:
- @input func: Function that is executed by the new thread
- @input args: Pointer to an argument that is handed to the 
    called function
- @input args_size: Number of bytes of args
*/
void thread_create(void(*func)(void*), const void *args, uint32_t args_size, uint8_t is_proc);

/* 
Gives the CPU to kernel. Function is not scheduled for at least
the given amount of time
- @input millis: time in milliseconds for which the thread shall
    not be scheduled
*/
void sleep(uint32_t millis);

/* 
Reads char from serial console in blocking mode.
- @input char_read: pointer to where the read char shall be stored
- @return: 0 if charackter was read; 1 if device is busy 
*/
uint8_t read_char(char* char_read);

/* 
Writes the given char to serial console.
- @input char_write: character that shall be sent
*/
void write_char(char char_write);

/*
Calls an unknown syscall. This is used for debugging purposes.
*/
void unknown_syscall(void);

#endif // SYS_H