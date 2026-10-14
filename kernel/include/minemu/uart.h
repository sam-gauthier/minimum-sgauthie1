#ifndef MINEMU_UART_H
#define MINEMU_UART_H
#include <stdint.h>

void minemu_irq_register(uint32_t id, void (*handler)(void));

void uart_init(void);
void uart_putc(char c);
void uart_puts(const char *s);
int  uart_getc_nonblock(void);   /* -1 if empty */
int  uart_rx_ready(void);
char uart_getc(void);            /* blocks */
void kprintf(const char *fmt, ...);
void msh_run(void);

#endif