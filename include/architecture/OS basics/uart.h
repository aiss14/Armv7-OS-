#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <arch/cpu/arm.h>
#include <arch/cpu/mm.h>

#define UART_BASE   (0x7E201000 - PERIPH_OFFSET)

void uart_enable(void);
uint8_t uart_char_available(void);
char uart_get_char(void);
void uart_put_char(char c);
void uart_put_str(const char *input);

void uart_intr_h(struct registers_t * reg);

#endif // UART_H
