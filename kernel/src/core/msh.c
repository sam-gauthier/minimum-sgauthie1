#include <stdint.h>
#include "minemu/uart.h"

#define LINE_MAX_LEN 20u   /* documented capacity: 20 bytes */

static int str_eq(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

static void run_command(char *line, uint32_t len) {
    line[len] = '\0';
    char *p = line;
    while (*p == ' ') p++;
    if (*p == '\0') return;                 /* empty or all spaces */

    char *cmd = p;
    while (*p && *p != ' ') p++;
    if (*p) *p++ = '\0';                    /* terminate command word */

    if (str_eq(cmd, "echo")) {
        while (*p == ' ') p++;              /* repeated spaces = one separator */
        uart_puts(p);
        uart_putc('\n');
    } else {
        kprintf("command not found: %s\n", cmd);
    }
}

void msh_run(void) {
    char line[LINE_MAX_LEN + 1];
    uint32_t len = 0, overflow = 0;

    kprintf("msh> ");
    for (;;) {
        char c = uart_getc();
        if (c == '\n') {
            run_command(line, len);
            len = 0;
            overflow = 0;
            kprintf("msh> ");
        } else if (c == 0x08 || c == 0x7f) {
            if (overflow) overflow--;       /* undo a dropped char first */
            else if (len) len--;            /* ignored on an empty line */
        } else if (len < LINE_MAX_LEN) {
            line[len++] = c;
        } else {
            overflow++;                     /* excess input discarded */
        }
    }
}