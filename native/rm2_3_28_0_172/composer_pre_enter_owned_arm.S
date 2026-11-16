// Owned ELF caller/veneer storage only. Never assembled into a device payload.
.syntax unified
.arm
.section .stock_call,"ax",%progbits
.global owned_enter
.type owned_enter,%function
owned_enter:
.fnstart
.save {r4,lr}
push {r4,lr}
.global owned_site
owned_site:
bl stock_original
pop {r4,pc}
.fnend
.size owned_enter,.-owned_enter

.text
.balign 4
.global owned_abi_enter
.type owned_abi_enter,%function
owned_abi_enter:
.fnstart
.save {r4-r11,lr}
push {r4-r11,lr}
.pad #4
sub sp,sp,#4
.vsave {d8-d15}
vpush {d8-d15}
mov r4,#0x44
mov r5,#0x55
mov r6,#0x66
mov r7,#0x77
mov r8,#0x88
mov r9,#0x99
mov r10,#0xaa
mov r11,#0xbb
mov r3,#0xcc
vmov d8,r3,r3
vmov d9,r3,r3
vmov d10,r3,r3
vmov d11,r3,r3
vmov d12,r3,r3
vmov d13,r3,r3
vmov d14,r3,r3
vmov d15,r3,r3
bl owned_enter
mov r0,#0
cmp r4,#0x44
bne 1f
cmp r5,#0x55
bne 1f
cmp r6,#0x66
bne 1f
cmp r7,#0x77
bne 1f
cmp r8,#0x88
bne 1f
cmp r9,#0x99
bne 1f
cmp r10,#0xaa
bne 1f
cmp r11,#0xbb
bne 1f
.macro checkfp reg
vmov r2,r3,\reg
cmp r2,#0xcc
bne 1f
cmp r3,#0xcc
bne 1f
.endm
checkfp d8
checkfp d9
checkfp d10
checkfp d11
checkfp d12
checkfp d13
checkfp d14
checkfp d15
mov r0,#1
1:
vpop {d8-d15}
add sp,sp,#4
pop {r4-r11,pc}
.fnend
.size owned_abi_enter,.-owned_abi_enter

.section .stock_veneer,"ax",%progbits
.balign 4
.global owned_veneer
owned_veneer:
.word 0,0
.section .note.GNU-stack,"",%progbits
