#include <stdio.h>
#include <setjmp.h>

static int no, fail;
static jmp_buf env, env2;

#define ok(c,n) do { \
    no++; \
    if (c) printf("ok %d - %s\n", no, n); \
    else { printf("not ok %d - %s\n", no, n); fail++; } \
} while (0)

static void jump3(int n) { longjmp(env, n); }
static void jump2(int n) { jump3(n); }
static void jump1(int n) { jump2(n); }

static void deep(int n)
{
    volatile unsigned int pad[8];
    pad[0]=n;
    if(n<0) return;
    if(pad[0]) deep(n-1);
    else longjmp(env,77);
}

int main(void)
{
    int n;
    volatile int v = 1;
    volatile int stage = 0;

    printf("1..9\n");

    n = setjmp(env);
    if (!n) {
        ok(n == 0, "setjmp returns 0");
        jump1(42);
    }
    ok(n == 42, "longjmp returns value");

    n = setjmp(env);
    if (!n) longjmp(env, 0);
    ok(n == 1, "longjmp zero becomes one");

    n = setjmp(env);
    if (!n) {
        v = 123;
        longjmp(env, 5);
    }
    ok(v == 123, "volatile survives longjmp");

    n = setjmp(env);
    if (!n) deep(16);
    ok(n == 77, "stack restored after deep calls");

    n = setjmp(env);
    if (!n) {
        int m = setjmp(env2);
        if (!m) longjmp(env2, 7);
        ok(m == 7, "second jmp_buf works");
        longjmp(env, 9);
    }
    ok(n == 9, "nested jmp_buf returns outer");

    n = setjmp(env);
    if (stage == 0) {
        stage = 1;
        longjmp(env, 11);
    }
    if (stage == 1) {
        ok(n == 11, "same env can jump again");
        stage = 2;
        longjmp(env, 22);
    }
    ok(n == 22, "same env second jump");

    return fail != 0;
}
