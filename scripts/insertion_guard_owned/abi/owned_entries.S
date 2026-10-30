// Original owned ARM entry veneers and register probe; no firmware body.
.syntax unified
.arch armv7-a
.fpu neon-vfpv4
.arm
.text
.align 2
.global owned_outer_entry
.type owned_outer_entry,%function
owned_outer_entry:
.fnstart
    push {r4-r11,lr}
.save {r4-r11,lr}
    mov r4,r0
    vpush {d8-d9}
.vsave {d8-d9}
    sub sp,sp,#12
.pad #12
    ldr r12,[sp,#64]
    str r12,[sp,#0]
    ldr r12,[sp,#68]
    str r12,[sp,#4]
    ldr r12,[sp,#72]
    str r12,[sp,#8]
    vmov.f64 d8,#8.0
    vmov.f64 d9,#9.0
    bl owned_outer_body
    add sp,sp,#12
    vpop {d8-d9}
    pop {r4-r11,pc}
.fnend
.size owned_outer_entry,.-owned_outer_entry

.align 2
.global owned_inner_entry
.type owned_inner_entry,%function
owned_inner_entry:
.fnstart
    push {r4-r11,lr}
.save {r4-r11,lr}
    mov r5,r3
    vpush {d8-d9}
.vsave {d8-d9}
    sub sp,sp,#12
.pad #12
    ldr r12,[sp,#64]
    str r12,[sp,#0]
    ldr r12,[sp,#68]
    str r12,[sp,#4]
    vmov.f64 d8,#4.0
    vmov.f64 d9,#5.0
    bl owned_inner_body
    add sp,sp,#12
    vpop {d8-d9}
    pop {r4-r11,pc}
.fnend
.size owned_inner_entry,.-owned_inner_entry

.align 2
.global owned_probe_registers
.type owned_probe_registers,%function
owned_probe_registers:
.fnstart
    push {r4-r6,lr}
.save {r4-r6,lr}
    vpush {d8-d9}
.vsave {d8-d9}
    mov r4,r0
    mov r0,r1
    mov r5,#0
    movw r6,#0
    movt r6,#0x3ff2
    vmov d8,r5,r6
    movw r6,#0
    movt r6,#0x4002
    vmov d9,r5,r6
    blx r4
    vmov r1,r2,d8
    movw r3,#0
    movt r3,#0x3ff2
    cmp r1,#0
    cmpeq r2,r3
    moveq r0,#1
    movne r0,#0
    vmov r1,r2,d9
    movw r3,#0
    movt r3,#0x4002
    cmp r1,#0
    cmpeq r2,r3
    orreq r0,r0,#2
    vpop {d8-d9}
    pop {r4-r6,pc}
.fnend
.size owned_probe_registers,.-owned_probe_registers
.section .note.GNU-stack,"",%progbits
