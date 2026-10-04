#pragma once
#include <stdint.h>

void uart_init(void);
void uart_putc(char c);
void uart_puts(const char *s);
int  uart_getc_nonblock(void);
char uart_getc(void);
void kprintf(const char *fmt, ...);