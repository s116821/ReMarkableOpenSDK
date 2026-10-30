/* Original ARM checkpoint fixture. Fixed frame/words qualify the measurement ABI. */
.syntax unified
.arm
.fpu neon
.text
.align 2
.global fixture_current_caller
.global fixture_current_return
.type fixture_current_caller,%function
fixture_current_caller:
    push {r7,lr}
    bl fixture_normalize
fixture_current_return:
    pop {r7,pc}
.size fixture_current_caller,.-fixture_current_caller
.global fixture_other_caller
.type fixture_other_caller,%function
fixture_other_caller:
    push {r7,lr}
    bl fixture_normalize
    pop {r7,pc}
.size fixture_other_caller,.-fixture_other_caller
.type fixture_normalize,%function
fixture_normalize:
    push {r4-r10,lr}
    vpush {d8-d9}
    sub sp,sp,#128
    mov r5,r0
    mov r4,r1
    add r6,r5,#0x90
    add r2,sp,#0x20
    bl fixture_fractions
    mov r7,#0
    str r7,[sp,#108]
    cmp r0,#0
.global fixture_pre
fixture_pre:
    beq .Ltype_join
    mov r0,r6
    bl fixture_matrix_type
.Ltype_join:
    cmp r0,#0
    beq fixture_post
    mov r0,r5
    add r1,sp,#0x20
    bl fixture_transform
.global fixture_post
fixture_post:
    ldr r0,[sp,#108]
    vldr d0,[sp,#0x20]
    vldr d1,[sp,#0x28]
    vstr d0,[r5,#0xe0]
    vstr d1,[r5,#0xe8]
    add sp,sp,#128
    vpop {d8-d9}
    pop {r4-r10,pc}
.size fixture_normalize,.-fixture_normalize
.section .note.GNU-stack,"",%progbits
