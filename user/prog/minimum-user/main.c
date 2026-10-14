#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "minemu/user_abi.h"

#define MSH_LINE_MAX 20U

static void uart_write_all(const char *text, uint32_t length) {
    while (length > 0U) {
        int32_t written = ioctl(MINEMU_FD_UART_OUT, MINEMU_IOCTL_UART_WRITE,
                                (void *)text, length);
        if (written <= 0) {
            return;
        }
        text += written;
        length -= (uint32_t)written;
    }
}

static void uart_write_str(const char *text) {
    uint32_t length = 0;
    while (text[length] != '\0') {
        ++length;
    }
    uart_write_all(text, length);
}

/* uart_read is non-blocking: it returns 0 when nothing is available,
 * so poll until a byte arrives. */
static uint8_t uart_read_byte(void) {
    char c = 0;
    for (;;) {
        int32_t count = ioctl(MINEMU_FD_UART_IN, MINEMU_IOCTL_UART_READ,
                              (void *)&c, 1U);
        if (count > 0) {
            return (uint8_t)c;
        }
    }
}

static bool word_equals(const char *text, uint32_t length, const char *word) {
    uint32_t index = 0;
    while (index < length && word[index] != '\0') {
        if (text[index] != word[index]) {
            return false;
        }
        ++index;
    }
    return index == length && word[index] == '\0';
}

static void run_command(const char *line, uint32_t length) {
    uint32_t pos = 0;
    while (pos < length && line[pos] == ' ') {
        ++pos;
    }
    if (pos == length) {
        return; /* empty or all-space line */
    }

    uint32_t start = pos;
    while (pos < length && line[pos] != ' ') {
        ++pos;
    }
    uint32_t word_length = pos - start;

    if (word_equals(&line[start], word_length, "echo")) {
        /* msh-editing expects "echo   worked   fine" to print
         * "worked fine": words joined by one space, no trailing space. */
        bool first = true;
        while (pos < length) {
            while (pos < length && line[pos] == ' ') {
                ++pos;
            }
            if (pos == length) {
                break;
            }
            uint32_t word_start = pos;
            while (pos < length && line[pos] != ' ') {
                ++pos;
            }
            if (!first) {
                uart_write_str(" ");
            }
            uart_write_all(&line[word_start], pos - word_start);
            first = false;
        }
        uart_write_str("\n");
        return;
    }

    uart_write_str("command not found: ");
    uart_write_all(&line[start], word_length);
    uart_write_str("\n");
}

void minemu_user_main(void) {
    char line[MSH_LINE_MAX];

    for (;;) {
        uart_write_str("msh> ");
        uint32_t length = 0;
        bool overlong = false;

        /* Input is not echoed: the tests expect only prompts and results. */
        for (;;) {
            uint8_t c = uart_read_byte();
            if (c == '\n') {
                break;
            }
            if (c == 0x08U || c == 0x7fU) {
                if (length > 0U) {
                    --length;
                }
                continue;
            }
            if (length < MSH_LINE_MAX) {
                line[length++] = (char)c;
            } else if (c != ' ') {
                /* A non-space byte past 20 bytes: the line is overlong. */
                overlong = true;
            }
            /* Spaces past 20 bytes are ignored (msh-editing has two
             * trailing spaces after a 20-byte command). */
        }

        if (!overlong) {
            run_command(line, length);
        } /* overlong lines are discarded silently (msh-overlong) */
    }
}