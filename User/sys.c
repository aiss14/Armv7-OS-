#include <stdint.h>
#include <kernel/syscalls.h>

#define STR(x)  #x
#define XSTR(s) STR(s)

void exit()
{
    asm("svc " XSTR(SYS_EXIT));
}

void thread_create(void(*func)(void*), const void *args, uint32_t args_size, uint8_t is_proc)
{
    (void) func;
    (void) args;
    (void) args_size;
    (void) is_proc;
    asm("svc " XSTR(SYS_CREATE_THREAD));
}

void sleep(uint32_t millis)
{
    (void) millis;
    asm("svc " XSTR(SYS_SLEEP));
}


uint8_t read_char(char* char_read)
{
    (void) char_read;
    
    asm("svc " XSTR(SYS_READ_CHAR) ::: "r0");
    register char ret asm("r0");

    return ret;
}

void write_char(char char_write)
{  
    (void) char_write;

    asm("svc " XSTR(SYS_WRITE_CHAR) ::: );
}

void unknown_syscall()
{
    asm("svc " XSTR(N_SYSCALL_CODES)); // this syscall can never exist
}