#include <stdarg.h>
#include <stdint.h>
#include "minemu/platform.h"
#include "minemu/uart.h"

#define RX_BUF_SIZE 256u   /* power of two */

static volatile uint8_t  rx_buf[RX_BUF_SIZE];
static volatile uint32_t rx_head;   /* written only by ISR */
static volatile uint32_t rx_tail;   /* written only by main code */

static inline uint32_t irq_save(void) {
    uint32_t cpsr;
    __asm__ volatile("mrs %0, cpsr\n\tcpsid i" : "=r"(cpsr) :: "memory", "cc");
    return cpsr;
}
static inline void irq_restore(uint32_t cpsr) {
    if (!(cpsr & 0x80u)) {            /* only re-enable if they were enabled before */
        __asm__ volatile("cpsie i" ::: "memory");
    }
}

static void uart_rx_handler(void) {
    while (MINEMU_UART0->status & MINEMU_UART_STATUS_RX_READY) {
        uint8_t b = (uint8_t)MINEMU_UART0->rx_data;
        uint32_t next = (rx_head + 1u) & (RX_BUF_SIZE - 1u);
        if (next != rx_tail) {        /* full: drop byte (but still drained it) */
            rx_buf[rx_head] = b;
            rx_head = next;
        }
    }
}

void uart_init(void) {
    minemu_irq_register(MINEMU_IRQ_UART0, uart_rx_handler);
    MINEMU_UART0->control = MINEMU_UART_CONTROL_RX_IRQ_ENABLE;
    MINEMU_INTERRUPT->enable |= (UINT32_C(1) << MINEMU_IRQ_UART0);
}

void uart_putc(char c) {
    while (!(MINEMU_UART0->status & MINEMU_UART_STATUS_TX_READY)) { }
    MINEMU_UART0->tx_data = (uint32_t)(uint8_t)c;
}

void uart_puts(const char *s) {
    while (*s) uart_putc(*s++);
}

int uart_getc_nonblock(void) {
    int ret = -1;
    uint32_t f = irq_save();
    if (rx_tail != rx_head) {
        ret = rx_buf[rx_tail];
        rx_tail = (rx_tail + 1u) & (RX_BUF_SIZE - 1u);
    }
    irq_restore(f);
    return ret;
}

int uart_rx_ready(void) {
    uint32_t f = irq_save();
    int ready = (rx_tail != rx_head) || ((MINEMU_UART0->status & MINEMU_UART_STATUS_RX_READY) != 0);
    irq_restore(f);
    return ready;
}

char uart_getc(void) {
    int c;
    while ((c = uart_getc_nonblock()) < 0) { }
    return (char)c;
}

static void put_uint(uint32_t v, uint32_t base) {
    char tmp[12];
    int i = 0;
    if (v == 0) { uart_putc('0'); return; }
    while (v) { tmp[i++] = "0123456789abcdef"[v % base]; v /= base; }
    while (i--) uart_putc(tmp[i]);
}

void kprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    for (; *fmt; fmt++) {
        if (*fmt != '%') { uart_putc(*fmt); continue; }
        switch (*++fmt) {
        case 's': { const char *s = va_arg(ap, const char *); uart_puts(s ? s : "(null)"); break; }
        case 'c': uart_putc((char)va_arg(ap, int)); break;
        case 'd': { int v = va_arg(ap, int);
                    if (v < 0) { uart_putc('-'); put_uint(0u - (uint32_t)v, 10); }
                    else put_uint((uint32_t)v, 10);
                    break; }
        case 'u': put_uint(va_arg(ap, uint32_t), 10); break;
        case 'x': put_uint(va_arg(ap, uint32_t), 16); break;
        case '%': uart_putc('%'); break;
        case '\0': va_end(ap); return;
        default:  uart_putc('%'); uart_putc(*fmt); break;
        }
    }
    va_end(ap);
}