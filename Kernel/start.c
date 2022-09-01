#include <kernel/kprintf.h>
#include <kernel/thread.h>
#include <arch/bsp/uart.h>
#include <arch/cpu/arm.h>
#include <user/userthread.h>
#include <config.h>
#include <arch/bsp/mmu.h>

void _leave_kernel();

void start_kernel() 
{
	kprintf("Starting practOS ...\n");

	uart_enable();
	SWITCH_PROC_MODE(PSR_SYS);
	asm("cpsie if"); // enable interrupts
	SWITCH_PROC_MODE(PSR_SUP);
	
	mmu_init();

	init_threads();
	kthread_create(NO_REGISTERS, main, 0, 0, 1);
	kprintf("practOS ready.\n");

	start_scheduling();    

	_leave_kernel();
}