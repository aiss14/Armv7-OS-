#include <arch/cpu/mm.h>
#include <arch/cpu/exceptions.h>
#define undef .word 0xe7ffffff

.section .init

.global _start
_start:
	/* Erkennen ob Hypervisor Mode aktiv */
	mrs r0, cpsr
	and r0, r0, #0x1F
	mov r1, #0x1A	//Modebits = 0x1A = Hypervisor Mode
	cmp r0, r1		//Vergleichen und Statusbits setzen für bedingte Sprünge

	/* Wenn Hypemode erkannt: abschalten (sprung nach _exitHyper)
	 * wenn nicht erkannt: dann weitermachen (weiterlaufen nach _bsprak)
	 */
	beq _exitHyper

	/* Qemu startet immer alle 4 Kerne
	 * Wir lassen alle anderen Kerne endless loopen
	 */
_checkCores:
	/* Id des Cpu Cores Abfragen */
	mrc p15, 0, r0, c0, c0, 5
	/* Falls nicht core 0 Core disablen */
	tst r0, #3
	bne _parkCore

/* not modeled in qemu 6.0 */
_enableAlignCheck:
	/* SCTLR des cp15 laden */
	mrc p15, 0, r0, c1, c0, 0
	/* A-bit (c1[1]) auf 1 setzen */
	orr r0, r0, #0x2
	/* neues SCTLR speichern */
	mcr p15, 0, r0, c1, c0, 0


_bsprak:
	/* Set all stack pointers */
	ldr r0,=_ram_kernel_end
	msr sp_fiq,r0
	sub r0,r0,#STACK_SIZE_FIQ
	msr sp_irq,r0
	sub r0,r0,#STACK_SIZE_IRQ
	mov sp,r0	/* currently in supervisor mode */
	sub r0,r0,#STACK_SIZE_SVC
	msr sp_und,r0
	sub r0,r0,#STACK_SIZE_UND
	msr sp_abt,r0
	ldr r0,=_ram_user_end
	msr sp_usr,r0

	/* set ivt */
	ldr r0,=_ivt
	mcr p15, 0, r0, c12, c0, 0
	
	/* Zu c Code springen */
	bl  start_kernel
.Lend:
	WFI
	b .Lend

_parkCore:
	/* Interrupts für Core 1 bis 3 ausschalten  */
	cpsid if
	/* In Endlosschleife springen */
	b .Lend

_exitHyper:

	/* Rücksprungadresse ins Hypervisor Rücksprungregister schreiben.
	 * Dies ist das Label _bsprak für die normale Programmausfürung.
	 */
	ldr lr, =_checkCores
	msr ELR_hyp, lr

	/* Der Hypervisor mode ist auch "nur" eine etwas kompliziertere Exception.
	 * Also springen wir aus dieser zurück.
	 * (Rücksprunge in Aufgabe 2 sind anders zu lösen!).
	 * Dazu zunächst das CPSR auslesen und die Modebits zum Supervisor ändern.
	 * Danach dies in das SPSR für den Rücksprung schreiben.
	 */
	mrs r0, cpsr
	bic r0, r0, #0x1F
	orr r0, r0, #0x13	//Supervisormode
	msr spsr_hyp, r0
	eret

.section .text

.global _leave_kernel
.weak _infinite_loop
_leave_kernel:
	// prepare spsr
	mrs r0, cpsr
	bic r0, r0, #0x1F	// clear mode bits
	orr r0, r0, #0x10	// set user mode
	msr spsr, r0
	// prepare location to jump
    ldr r0, =_infinite_loop
	push {r0}
	// change to user
	ldm sp!, {pc}^


.balign 32
_ivt:
    b 	reset
    b	_undefined_instruction
    b	_software_interrupt
    b	_prefetch_abort
    b	_data_abort
    b	_unused_handler
    b	_interrupt
    b	_fast_interrupt

_undefined_instruction:
	sub lr, lr, #UND_LR_CORRECTION
	push {lr}
	push {r0-r12}	/* 13 words */
	ldr r1, =undefined_instruction
	b _exception_trampolin

_software_interrupt:
	sub lr, lr, #SVC_LR_CORRECTION
	push {lr}
	push {r0-r12}	/* 13 words */
	ldr r1, =software_interrupt
	b _exception_trampolin

_prefetch_abort:
	sub lr, lr, #PREF_ABT_LR_CORRECTION
	push {lr}
	push {r0-r12}	/* 13 words */
	ldr r1, =prefetch_abort
	b _exception_trampolin

_data_abort:
	sub lr, lr, #DATA_ABT_LR_CORRECTION
	push {lr}
	push {r0-r12}	/* 13 words */
	ldr r1, =data_abort
	b _exception_trampolin

_unused_handler:
	sub lr, lr, #0
	push {lr}
	push {r0-r12}	/* 13 words */
	ldr r1, =unused_handler
	b _exception_trampolin

_interrupt:
	sub lr, lr, #IRQ_LR_CORRECTION
	push {lr}
	push {r0-r12}	/* 13 words */
	ldr r1, =irq
	b _exception_trampolin

_fast_interrupt:
	sub lr, lr, #FIQ_LR_CORRECTION
	push {lr}
	push {r0-r12}	/* 13 words */
	ldr r1, =fiq
	b _exception_trampolin


_exception_trampolin:
	/* 
	r1: exception handler 
	*/

	/* entry trampolin */
	push {pc}	/* 2 words */
	push {sp}		/* 1 word */
	mov r0, sp		/* registers first parameter of handlers */
	blx r1

	/* exit trampolin */
	add sp, sp, #8	/* skip sp and pc (2*4 Bytes) */
	ldm	sp!, {r0-r12, pc}^

#undef __ASSEMBLY__
