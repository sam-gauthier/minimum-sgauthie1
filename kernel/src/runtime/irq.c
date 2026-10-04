#include <stdint.h>
#include "minemu/platform.h"
#include "minemu/trap.h"

#define MINEMU_IRQ_COUNT 4u

typedef void (*minemu_irq_handler_t)(void);
static minemu_irq_handler_t irq_handlers[MINEMU_IRQ_COUNT];

void minemu_irq_register(uint32_t id, minemu_irq_handler_t handler) {
    if (id < MINEMU_IRQ_COUNT) {
        irq_handlers[id] = handler;
    }
}

struct minemu_trap_frame *minemu_irq_dispatch(struct minemu_trap_frame *frame) {
    uint32_t id = (uint32_t)frame->exception_id;

    if (id == MINEMU_IRQ_NONE) {          /* spurious: nothing claimed, no EOI */
        return frame;
    }
    if (id < MINEMU_IRQ_COUNT && irq_handlers[id] != NULL) {
        irq_handlers[id]();               /* handler must drain the device first */
    }
    MINEMU_INTERRUPT->eoi = id;
    return frame;                         /* same frame until scheduling exists */
}