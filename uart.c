#include <stdint.h>
#include <stdarg.h>
#include <config.h>
#include <arch/cpu/arm.h>
#include <arch/bsp/uart.h>
#include <arch/bsp/intr.h>
#include <lib/primfunc.h>
#include <kernel/kprintf.h>
#include <kernel/thread.h>
#include <user/userthread.h>

/*
 * Device driver for UART peripherial
 * Datasheet: https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf
 * Page: From 175
*/

#define UART_CR_TX  8   /* cr register, receive flag */
#define UART_CR_RX  9   /* cr register, transmit flag */

#define INTR_RX     4    /* ris, mis and icr register; receive bit */


struct uart {
    uint32_t dr;        /* data register */
    uint32_t rsrecr;    /* receive status register/error clear register */
    uint32_t unused1[4];
    uint32_t fr;        /* flag register */
    uint32_t unused2[2]; /* the first byte is a "missed" one between ILPR and FR */
    uint32_t ibrd;      /* integer baud rate divisor */
    uint32_t fbrd;      /* fractional baud rate divisor */
    uint32_t lcrh;      /* line control register */
    uint32_t cr;        /* control register */
    uint32_t ifls;      /* interupt FIFO Level Select Register */
    uint32_t imsc;      /* interupt mask set clear register */
    uint32_t ris;       /* raw interupt status register */
    uint32_t mis;       /* masked interupt status register */
    uint32_t icr;       /* interupt clear register */
    uint32_t dmacr;     /* DMA control register */
    uint32_t unused3[13];
    uint32_t itcr;      /* test control register */
    uint32_t itip;      /* integration test input register */
    uint32_t itop;      /* integration test output register */
    uint32_t tdr;       /* test data register */
};

struct ring_buf {
    int32_t head;
    int32_t tail;
    uint8_t is_full;
    char buff[UART_INPUT_BUFFER_SIZE];
};

volatile struct uart* uart_dev = (struct uart*) UART_BASE;
volatile struct ring_buf uart_input_buffer = {0,0,0,{0}};

void uart_enable()
{
    uart_dev->imsc = (1 << INTR_RX);
    interrupt_enable(IRQ_UART, 0);
}

uint8_t uart_char_available()
{
    return !(uart_input_buffer.head == uart_input_buffer.tail);
}

char uart_get_char()
{
    /* wait for data */
    while (!uart_input_buffer.is_full && !uart_char_available()) {
        continue;
    }

    char c = uart_input_buffer.buff[uart_input_buffer.tail];

    uart_input_buffer.tail = (uart_input_buffer.tail + 1) % UART_INPUT_BUFFER_SIZE;
    uart_input_buffer.is_full = 0;

    return c;
}

void uart_put_char(char c) 
{
    uart_dev->dr = c;
}


void uart_put_str(const char *input){
    while(*input != '\0')
        uart_put_char(*input++);
}

void uart_intr_h(struct registers_t * reg)
{
    char c = uart_dev->dr;

    /* Task 5: access invalid memory regions */
    #define READ_FROM(location) \
            asm("mov r5, %0" :: "r"(location): "r5"); \
            asm("ldr r5,[r5]" ::: "r5");
    #define WRITE_TO(location) \
            asm("mov r5, %0" :: "r"(location): "r5"); \
            asm("mov r6, #0x123" ::: "r6"); \
            asm("str r6,[r5]" ::: );
    #define JUMP_TO(location) \
            asm("mov r5, %0" :: "r"(location): "r5"); \
            asm("mov pc, r5" ::: "pc");
    #define LINKER2VAL(linker)  ((uint32_t) &linker)

    extern uint32_t _text_kernel_start;
    extern uint32_t _text_user_start;
    extern uint32_t _ram_user_start;

    switch (c)
    {
    case 'N':
        READ_FROM(0)
        break;
    case 'P':
        JUMP_TO(0)
        break;
    case 'C':
        WRITE_TO(LINKER2VAL(_text_kernel_start))
        break;
    case 'U':
        READ_FROM(LINKER2VAL(_ram_user_start)-8)
        break;
    case 'X':
        JUMP_TO(LINKER2VAL(_text_user_start))
        break;
    default:
        break;
    }

    /* add received char to buffer */
    if (uart_input_buffer.is_full) {
        #ifdef DEBUG_ENABLE
        uart_put_str("WARNING: Full UART buffer. The input (\0");
        uart_put_char(c);
        uart_put_str(") wasn't saved!\n\0");
        #else
        (void) c;
        #endif // DEBUG_ENABLE

        return;
    }

    uart_input_buffer.buff[uart_input_buffer.head] = c;
    uart_input_buffer.head = (uart_input_buffer.head + 1) % UART_INPUT_BUFFER_SIZE;

    if(uart_input_buffer.head == uart_input_buffer.tail)
        uart_input_buffer.is_full = 1;

    thread_process_char_received(reg);
}

