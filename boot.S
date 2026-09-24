.syntax unified
.cpu cortex-m33
.fpu fpv5-sp-d16
.thumb

.section .vectors,"a"
.balign 128
vectors:
 .word __stack_top,reset+1
 .rept 13
 .word _fault+1
 .endr
 .word _systick+1

.section .text.reset,"ax"
.global reset
.thumb_func
reset:
 cpsid i
 ldr r0,=vectors
 ldr r1,=0xe000ed08
 str r0,[r1]
 ldr r0,=0xe000ed88
 ldr r1,[r0]
 ldr r2,=0xf00000
 orr r1,r2
 str r1,[r0]
 dsb
 isb
 ldr r0,=__data_load
 ldr r1,=__data_start
 ldr r2,=__data_end
 cmp r0,r1
 beq 2f
1: cmp r1,r2
 bhs 2f
 ldr r3,[r0],#4
 str r3,[r1],#4
 b 1b
2: ldr r0,=__bss_start
 ldr r1,=__bss_end
 movs r2,#0
3: cmp r0,r1
 bhs 4f
 str r2,[r0],#4
 b 3b
4: bl kernel_main
_fault:
 b _fault