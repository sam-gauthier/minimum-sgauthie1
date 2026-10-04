    .syntax unified
    .cpu cortex-a9
    .arm

    .global minemu_irq_trampoline
    .type minemu_irq_trampoline, %function
    .extern minemu_irq_dispatch

minemu_irq_trampoline:
    sub   sp, sp, #88            @ allocate one trap frame on the IRQ stack
    stmia sp, {r0-r12}           @ save r0-r12 at offset 0
    str   lr, [sp, #52]          @ return_lr (interrupted PC + 4)
    mrs   r1, spsr
    str   r1, [sp, #56]          @ interrupted CPSR
    ldr   r1, =0x10000000        @ interrupt controller base
    ldr   r1, [r1, #8]           @ read CLAIM -> active source id
    str   r1, [sp, #60]          @ exception_id
    sub   r1, lr, #4
    str   r1, [sp, #64]          @ fault_pc = address of interrupted instruction
    mov   r1, #0
    str   r1, [sp, #68]          @ dfsr = 0
    str   r1, [sp, #72]          @ dfar = 0
    add   r1, sp, #76
    stmia r1, {sp, lr}^          @ save interrupted mode's banked sp/lr
    mov   r1, #0
    str   r1, [sp, #84]          @ reserved
    mov   r0, sp
    bl    minemu_irq_dispatch    @ r0 = frame; returns frame to restore
    mov   sp, r0
    ldr   lr, [sp, #52]
    ldr   r1, [sp, #56]
    msr   spsr_cxsf, r1
    add   r1, sp, #76
    ldmia r1, {sp, lr}^          @ restore banked sp/lr
    ldmia sp, {r0-r12}
    add   sp, sp, #88
    subs  pc, lr, #4             @ return, restoring CPSR from SPSR