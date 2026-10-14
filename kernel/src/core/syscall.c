#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "minemu/platform.h"
#include "minemu/syscall.h"
#include "minemu/trace.h"
#include "minemu/trap.h"
#include "minemu/uart.h"
#include "minemu/user_abi.h"
#include "minemu/user_space.h"
#include "minemu/debug.h"

#define CPSR_MODE_MASK UINT32_C(0x1f)
#define CPSR_MODE_USR UINT32_C(0x10)
#define CPSR_THUMB UINT32_C(0x20)
#define A32_SVC_ZERO UINT32_C(0xef000000)

static int32_t uart0_read_ioctl(uint32_t user_dst, uint32_t count) {
    if ((int32_t)count < 0 || count > MINEMU_IOCTL_MAX_TRANSFER) {
        return MINEMU_IOCTL_ERROR;
    }
    if (count == 0) {
        return 0;
    }
    if (!user_range_writable(user_dst, count)) {
        return MINEMU_IOCTL_ERROR;
    }
    uint8_t buffer[MINEMU_IOCTL_MAX_TRANSFER];
    uint32_t copied = 0;
    while (copied < count) {
        int c = uart_getc_nonblock();
        if (c < 0) {
            break;
        }
        buffer[copied++] = (uint8_t)c;
    }
    while (copied < count && (MINEMU_UART0->status & MINEMU_UART_STATUS_RX_READY) != 0) {
        buffer[copied++] = (uint8_t)MINEMU_UART0->rx_data;
    }
    if (copied > 0) {
        if (!copy_to_user(user_dst, buffer, copied)) {
            return MINEMU_IOCTL_ERROR;
        }
    }
    return (int32_t)copied;
}

static int32_t uart0_write_ioctl(uint32_t user_src, uint32_t count) {
    if ((int32_t)count < 0 || count > MINEMU_IOCTL_MAX_TRANSFER) {
        return MINEMU_IOCTL_ERROR;
    }
    if (count == 0) {
        return 0;
    }
    if (!user_range_readable(user_src, count)) {
        return MINEMU_IOCTL_ERROR;
    }
    uint8_t buffer[MINEMU_IOCTL_MAX_TRANSFER];
    if (!copy_from_user(buffer, user_src, count)) {
        return MINEMU_IOCTL_ERROR;
    }
    for (uint32_t index = 0; index < count; ++index) {
        while ((MINEMU_UART0->status & MINEMU_UART_STATUS_TX_READY) == 0) {
        }
        MINEMU_UART0->tx_data = buffer[index];
    }
    return (int32_t)count;
}

static int32_t uart1_read_ioctl(uint32_t user_dst, uint32_t count) {
    if ((int32_t)count < 0 || count > MINEMU_IOCTL_MAX_TRANSFER) {
        return MINEMU_IOCTL_ERROR;
    }
    if (count == 0) {
        return 0;
    }
    if (!user_range_writable(user_dst, count)) {
        return MINEMU_IOCTL_ERROR;
    }
    uint8_t buffer[MINEMU_IOCTL_MAX_TRANSFER];
    uint32_t copied = 0;
    while (copied < count && (MINEMU_UART1->status & MINEMU_UART_STATUS_RX_READY) != 0) {
        buffer[copied++] = (uint8_t)MINEMU_UART1->rx_data;
    }
    if (copied > 0) {
        if (!copy_to_user(user_dst, buffer, copied)) {
            return MINEMU_IOCTL_ERROR;
        }
    }
    return (int32_t)copied;
}

static int32_t uart1_write_ioctl(uint32_t user_src, uint32_t count) {
    if ((int32_t)count < 0 || count > MINEMU_IOCTL_MAX_TRANSFER) {
        return MINEMU_IOCTL_ERROR;
    }
    if (count == 0) {
        return 0;
    }
    if (!user_range_readable(user_src, count)) {
        return MINEMU_IOCTL_ERROR;
    }
    uint8_t buffer[MINEMU_IOCTL_MAX_TRANSFER];
    if (!copy_from_user(buffer, user_src, count)) {
        return MINEMU_IOCTL_ERROR;
    }
    for (uint32_t index = 0; index < count; ++index) {
        while ((MINEMU_UART1->status & MINEMU_UART_STATUS_TX_READY) == 0) {
        }
        MINEMU_UART1->tx_data = buffer[index];
    }
    return (int32_t)count;
}

static int32_t ioctl_dispatch(int32_t fd, uint32_t request, uint32_t argument0, uint32_t argument1) {
    if (fd == MINEMU_FD_UART_IN || fd == MINEMU_FD_UART_OUT ||
        fd == MINEMU_FD_UART_DIAG || fd == (int32_t)MINEMU_UART0_BASE) {
        if (request == MINEMU_IOCTL_UART_READ || request == 0) {
            if (fd == MINEMU_FD_UART_OUT || fd == MINEMU_FD_UART_DIAG) {
                return MINEMU_IOCTL_ERROR;
            }
            return uart0_read_ioctl(argument0, argument1);
        } else if (request == MINEMU_IOCTL_UART_WRITE || request == 1) {
            if (fd == MINEMU_FD_UART_IN) {
                return MINEMU_IOCTL_ERROR;
            }
            return uart0_write_ioctl(argument0, argument1);
        } else if (request == MINEMU_IOCTL_UART_STATUS) {
            uint32_t status_flags = 0;
            if (uart_rx_ready()) {
                status_flags |= MINEMU_IOCTL_UART_RX_READY;
            }
            if ((MINEMU_UART0->status & MINEMU_UART_STATUS_TX_READY) != 0) {
                status_flags |= MINEMU_IOCTL_UART_TX_READY;
            }
            return (int32_t)status_flags;
        }
        return MINEMU_IOCTL_ERROR;
    }

    if (fd == (int32_t)MINEMU_UART1_BASE) {
        if (request == MINEMU_IOCTL_UART_READ || request == 0) {
            return uart1_read_ioctl(argument0, argument1);
        } else if (request == MINEMU_IOCTL_UART_WRITE || request == 1) {
            return uart1_write_ioctl(argument0, argument1);
        } else if (request == MINEMU_IOCTL_UART_STATUS) {
            uint32_t status_flags = 0;
            if ((MINEMU_UART1->status & MINEMU_UART_STATUS_RX_READY) != 0) {
                status_flags |= MINEMU_IOCTL_UART_RX_READY;
            }
            if ((MINEMU_UART1->status & MINEMU_UART_STATUS_TX_READY) != 0) {
                status_flags |= MINEMU_IOCTL_UART_TX_READY;
            }
            return (int32_t)status_flags;
        }
        return MINEMU_IOCTL_ERROR;
    }

    if (fd == MINEMU_FD_RNG || fd == (int32_t)MINEMU_RNG_BASE) {
        if (request == MINEMU_IOCTL_RNG_SEED || request == 0) {
            MINEMU_RNG->seed = argument0;
            return 0;
        } else if (request == MINEMU_IOCTL_RNG_NEXT || request == 1) {
            if ((argument0 & 3U) != 0 || !user_range_writable(argument0, sizeof(uint32_t))) {
                return MINEMU_IOCTL_ERROR;
            }
            uint32_t val = MINEMU_RNG->data;
            if (!copy_to_user(argument0, &val, sizeof(val))) {
                return MINEMU_IOCTL_ERROR;
            }
            return 0;
        } else if (request == MINEMU_IOCTL_RNG_STATE) {
            if ((argument0 & 3U) != 0 || !user_range_writable(argument0, sizeof(uint32_t))) {
                return MINEMU_IOCTL_ERROR;
            }
            uint32_t val = MINEMU_RNG->state;
            if (!copy_to_user(argument0, &val, sizeof(val))) {
                return MINEMU_IOCTL_ERROR;
            }
            return 0;
        }
        return MINEMU_IOCTL_ERROR;
    }

    if (fd == MINEMU_FD_TRACE || fd == (int32_t)MINEMU_TRACE_BASE) {
        if (request == MINEMU_IOCTL_TRACE_EVENT || request == 0) {
            minemu_trace_event(argument0);
            return 0;
        }
        return MINEMU_IOCTL_ERROR;
    }

    return MINEMU_IOCTL_ERROR;
}

struct minemu_trap_frame *minemu_svc_dispatch(struct minemu_trap_frame *frame) {
    uint32_t mode = frame->spsr & CPSR_MODE_MASK;
    if (frame->exception_id != MINEMU_EXCEPTION_SVC || mode != CPSR_MODE_USR ||
        (frame->spsr & CPSR_THUMB) != 0 ||
        frame->fault_pc < USER_IMAGE_BASE || frame->fault_pc >= USER_STACK_TOP ||
        (frame->fault_pc & 3U) != 0) {
        frame->r[0] = (uint32_t)MINEMU_IOCTL_ERROR;
        return frame;
    }

    uint32_t instruction;
    if (!copy_from_user(&instruction, frame->fault_pc, sizeof(instruction)) ||
        instruction != A32_SVC_ZERO) {
        frame->r[0] = (uint32_t)MINEMU_IOCTL_ERROR;
        return frame;
    }

    if (frame->r[7] == MINEMU_SYSCALL_IOCTL) {
        frame->r[0] = (uint32_t)ioctl_dispatch((int32_t)frame->r[0],
                                               frame->r[1],
                                               frame->r[2],
                                               frame->r[3]);
        return frame;
    }

    if (frame->r[7] == MINEMU_SYSCALL_EXIT) {
        minemu_fail_stop();
    }

    frame->r[0] = (uint32_t)MINEMU_IOCTL_ERROR;
    return frame;
}

void minemu_undefined_dispatch(struct minemu_trap_frame *frame) {
    (void)frame;
    minemu_fail_stop();
}

void minemu_abort_dispatch(struct minemu_trap_frame *frame) {
    (void)frame;
    minemu_fail_stop();
}
