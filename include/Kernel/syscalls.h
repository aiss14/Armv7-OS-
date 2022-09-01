#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdint.h>
#include <arch/cpu/arm.h>

#define SYS_EXIT        0
#define SYS_CREATE_THREAD   1
#define SYS_SLEEP           2
#define SYS_READ_CHAR       3
#define SYS_WRITE_CHAR      4
#define N_SYSCALL_CODES 5

uint8_t process_svc_code(uint32_t svc_code, struct registers_t *reg);

#endif // SYSCALLS_H