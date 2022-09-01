#include <stdint.h>
#include <arch/bsp/intr.h>

/*
 * Device driver for Interrupt peripherial
 * Datasheet: https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf
 * Page: From 109
*/

struct intr {
    uint32_t irq_basic_pending;
    uint32_t irq_pending[2];
    uint32_t fiq_control;
    uint32_t enable_irq[2];
    uint32_t enable_basic_irq;
    uint32_t disable_irq[2];
    uint32_t disable_basic_irq;
};

/* offset of 0x200 given in table on datasheet page 112 */
struct intr* intr_dev = (struct intr*) (INTR_BASE + 0x200);


/*
 * public function defintions
*/

void interrupt_enable(uint32_t interrupt_number, uint8_t is_base_interrupt)
{
    if (is_base_interrupt) {
        intr_dev->enable_basic_irq = 1 << interrupt_number;
    }
    else {
        /* interrupt number is between 0...63; this range is split up into
        two 32bit long fields to avoid 64bit datatypes */
        if (interrupt_number < 32)
            intr_dev->enable_irq[0] = 1 << interrupt_number;
        else
            intr_dev->enable_irq[1] = 1 << (interrupt_number - 32);
    }
}

void interrupt_disable(uint32_t interrupt_number, uint8_t is_base_interrupt)
{
    if (is_base_interrupt) {
        intr_dev->disable_basic_irq = 1 << interrupt_number;
    }
    else {
        /* interrupt number is between 0...63; this range is split up into
        two 32bit long fields to avoid 64bit datatypes */
        if (interrupt_number < 32)
            intr_dev->disable_irq[0] = 1 << interrupt_number;
        else
            intr_dev->disable_irq[1] = 1 << (interrupt_number - 32);
    }
}

void interrupt_get_pending(uint32_t pending_interrupts[])
{
    for (int i=0; i<2; i++)
        pending_interrupts[i] = intr_dev->irq_pending[i];
}