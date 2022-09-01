#ifndef INTR_H
#define INTR_H

#include <stdint.h>
#include <arch/cpu/mm.h>

#define INTR_BASE       (0x7E00B000 - PERIPH_OFFSET)

/* ARM peripherals interrupt sources */
#define IRQ_TIMER_BASE  0
#define IRQ_UART    57
/* ... other sources can be found on page 113 */

void interrupt_enable(uint32_t interrupt_number, uint8_t is_base_interrupt);
void interrupt_disable(uint32_t interrupt_number, uint8_t is_base_interrupt);
void interrupt_get_pending(uint32_t pending_interrupts[]);

#endif // INTR_H