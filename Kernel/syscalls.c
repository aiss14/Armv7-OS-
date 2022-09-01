#include <stdint.h>
#include <kernel/syscalls.h>
#include <kernel/thread.h>
#include <arch/cpu/arm.h>
#include <arch/cpu/mm.h>
#include <arch/bsp/uart.h>
#include <kernel/debug.h>

void handle_exit(struct registers_t *reg);
void handle_kthread_create(struct registers_t *reg);
void handle_sleep(struct registers_t *reg);
void handle_read_char(struct registers_t *reg);
void handle_write_char(struct registers_t *reg);

void (*syscall_callbacks[])(struct registers_t *reg) =
{
    handle_exit,
    handle_kthread_create,
    handle_sleep,
    handle_read_char,
    handle_write_char
};

uint8_t process_svc_code(uint32_t svc_code, struct registers_t *reg)
{
    /* 
    returns 0 if syscall exists and could be executed without errors;
    returns 1 if syscall does not exists
     */

    if (svc_code>=N_SYSCALL_CODES)
        return 1;

    syscall_callbacks[svc_code](reg);

    return 0;
}


void handle_exit(struct registers_t *reg)
{
    terminate_current_thread(reg);
}

void handle_kthread_create(struct registers_t *reg)
{
    void(*func_handle)(void*) = (void(*)(void*)) reg->base_registers[0];
    void *args = (void*) reg->base_registers[1];
    uint32_t args_size = (uint32_t) reg->base_registers[2];
    uint8_t is_proc = (uint32_t) reg->base_registers[3];

    kthread_create(reg, func_handle, args, args_size, is_proc);
}

void handle_sleep(struct registers_t *reg) {
    uint32_t millis_to_sleep = reg->base_registers[0];
    thread_make_sleep_current(reg, millis_to_sleep);
}

void handle_read_char(struct registers_t *reg)
{
    // received char shall be stored in c
    char *c = (char*) reg->base_registers[0];
    uint8_t ret_val;

    // deliver char if available
    if (uart_char_available()) {
        *c = uart_get_char();
        ret_val = 0;
    }
    // otherwise let thread wait for char
    else {
        ret_val = thread_wait_for_char(reg);
    }
    
    reg->base_registers[0] = ret_val;   // this is the return value of the user function
}

void handle_write_char(struct registers_t *reg) {
    uart_put_char((char) reg->base_registers[0]);
}