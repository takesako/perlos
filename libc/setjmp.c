#include "setjmp.h"

#define UNUSED __attribute__((unused))

__attribute__((naked, returns_twice))
int setjmp(jmp_buf env UNUSED)
{
    __asm volatile(
        "stmia r0,{r4-r11}\n"
        "mov r1,sp\n"
        "str r1,[r0,#32]\n"
        "str lr,[r0,#36]\n"
#if defined(__ARM_FP) && (__ARM_FP != 0)
        "add r1,r0,#40\n"
        "vstmia r1!,{s16-s31}\n"
#endif
        "movs r0,#0\n"
        "bx lr\n"
    );
}

__attribute__((naked, noreturn))
void longjmp(jmp_buf env UNUSED, int val UNUSED)
{
    __asm volatile(
        "cmp r1,#0\n"
        "bne 1f\n"
        "movs r1,#1\n"
        "1:\n"
        "mov r12,r1\n"
#if defined(__ARM_FP) && (__ARM_FP != 0)
        "add r2,r0,#40\n"
        "vldmia r2!,{s16-s31}\n"
#endif
        "ldr r2,[r0,#32]\n"
        "ldr r3,[r0,#36]\n"
        "ldmia r0,{r4-r11}\n"
        "mov sp,r2\n"
        "mov lr,r3\n"
        "mov r0,r12\n"
        "bx lr\n"
    );
}
