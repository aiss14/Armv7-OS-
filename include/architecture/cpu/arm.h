#ifndef ARM_H
#define ARM_H

#include <stdint.h>

#define NUM_REGISTERS   13
#define N_MODES         7

#define N_bit 31
#define Z_bit 30
#define C_bit 29
#define V_bit 28
#define E_bit 9
#define I_bit 7
#define F_bit 6
#define T_bit 5

#define PSR_MODE_MASK 0x1F
#define PSR_USR 0x10
#define PSR_FIQ 0x11
#define PSR_IRQ 0x12
#define PSR_SUP 0x13
#define PSR_ABT 0x17
#define PSR_UND 0x1B
#define PSR_SYS 0x1F

#define GET_MODE_REG(MODE, GET_FUNC) \
    if (current_mode==MODE){    \
        mreg.lr=reg->lr;    \
        mreg.sp=reg->sp;    \
        mreg.spsr=spsr;     \
    } else  \
        GET_FUNC(&mreg);    \

#define SWITCH_PROC_MODE(mode) asm("cps %0" :: "I"(mode))


struct registers_t {
    uint32_t sp;
    uint32_t pc;
    uint32_t base_registers[NUM_REGISTERS];
    uint32_t lr;
};

#define NO_REGISTERS ((struct registers_t *) 0)

/* TODO: use this everywhere instead of registers_t */
struct context_t {
    uint32_t sp;
    uint32_t cpsr;
    uint32_t pc;
    uint32_t base_registers[NUM_REGISTERS];
    uint32_t lr;
};

struct mode_registers {
    uint32_t sp;
    uint32_t lr;
    uint32_t spsr;
};

enum modes_t {
    MODE_USR, MODE_FIQ, MODE_IRQ, MODE_SVC, MODE_UND, MODE_ABT, MODE_NONE
};


void _get_fault_registers(uint32_t * status, uint32_t * address);
void _get_cpsr_spsr(uint32_t * cpsr, uint32_t * spsr);
uint32_t _get_proc_mode();
void _get_regs_usr(struct mode_registers * mode_regs_usr);
void _get_regs_svc(struct mode_registers * mode_regs_usr);
void _get_regs_abt(struct mode_registers * mode_regs_usr);
void _get_regs_fiq(struct mode_registers * mode_regs_usr);
void _get_regs_irq(struct mode_registers * mode_regs_usr);
void _get_regs_und(struct mode_registers * mode_regs_usr);

void _set_usr_sp_lr(uint32_t sp, uint32_t lr);

void _infinite_loop(void);

#endif // ARM_H