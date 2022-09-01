#include <arch/cpu/mm.h>

/* 
Functions to get special registers 
*/

/* returns fault status and address register 
r0: Pointer where to store status register
r1: Pointer where to store address register
*/
.global _get_fault_registers
_get_fault_registers:
    mrc p15, 0, r2, c5, c0, 0
    str r2, [r0]
    mrc p15, 0, r2, c6, c0, 0
    str r2, [r1]
    mov pc, lr

.global _get_cpsr_spsr
_get_cpsr_spsr:
    mrs r2, cpsr
    str r2, [r0]
    mrs r2, spsr
    str r2, [r1]
    mov pc, lr

.global _get_proc_mode
_get_proc_mode:
    mrs r0, cpsr
    and r0, r0, #0x1F
    mov pc, lr

/* 
functions to access sp, lr and spsr of different processor modes 
r0: Adress of struct to store values
*/
.global _get_regs_usr
_get_regs_usr:
    mrs r1, sp_usr
	mrs r2, lr_usr
    movw r3, #0xDA7A
    movt r3, #0xDEAD
    stm r0, {r1-r3}
    mov pc, lr

.global _get_regs_svc
_get_regs_svc:
    mrs r1, sp_svc
	mrs r2, lr_svc
    mrs r3, spsr_svc
    stm r0, {r1-r3}
    mov pc, lr

.global _get_regs_abt
_get_regs_abt:
    mrs r1, sp_abt
	mrs r2, lr_abt
    mrs r3, spsr_abt
    stm r0, {r1-r3}
    mov pc, lr

.global _get_regs_fiq
_get_regs_fiq:
    mrs r1, sp_fiq
	mrs r2, lr_fiq
    mrs r3, spsr_fiq
    stm r0, {r1-r3}
    mov pc, lr

.global _get_regs_irq
_get_regs_irq:
    mrs r1, sp_irq
	mrs r2, lr_irq
    mrs r3, spsr_irq
    stm r0, {r1-r3}
    mov pc, lr

.global _get_regs_und
_get_regs_und:
    mrs r1, sp_und
	mrs r2, lr_und
    mrs r3, spsr_und
    stm r0, {r1-r3}
    mov pc, lr


/* 
Functions to set special registers 
*/
.global _set_usr_sp_lr
_set_usr_sp_lr:
    msr sp_usr, r0
    msr lr_usr, r1
    mov pc, lr

