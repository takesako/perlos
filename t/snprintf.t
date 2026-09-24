#include <stdio.h>
#include <string.h>
#include <stddef.h>

static int no, fail;

#define ok(c,n) do { \
    no++; \
    if (c) printf("ok %d - %s\n", no, n); \
    else { printf("not ok %d - %s\n", no, n); fail++; } \
} while (0)

static int eq(const char *a, const char *b)
{
    while (*a && *a == *b) a++, b++;
    return *a == *b;
}

int main(void)
{
    char b[64];
    int n;

    printf("1..20\n");

    n = snprintf(b, sizeof(b), "abc");
    ok(n == 3 && eq(b, "abc"), "literal");

    n = snprintf(b, sizeof(b), "%s %c", "abc", 'X');
    ok(n == 5 && eq(b, "abc X"), "string and char");

    n = snprintf(b, sizeof(b), "%d", 12345);
    ok(n == 5 && eq(b, "12345"), "positive decimal");

    n = snprintf(b, sizeof(b), "%d", -12345);
    ok(n == 6 && eq(b, "-12345"), "negative decimal");

    n = snprintf(b, sizeof(b), "%d", (-2147483647 - 1));
    ok(n == 11 && eq(b, "-2147483648"), "INT_MIN");

    n = snprintf(b, sizeof(b), "%x %X", 0x12ab, 0x12ab);
    ok(n == 9 && eq(b, "12ab 12AB"), "hex lower and upper");

    n = snprintf(b, sizeof(b), "%05d", 42);
    ok(n == 5 && eq(b, "00042"), "zero padded decimal");

    n = snprintf(b, sizeof(b), "%04x", 0x2a);
    ok(n == 4 && eq(b, "002a"), "zero padded hex");

    n = snprintf(b, sizeof(b), "100%%");
    ok(n == 4 && eq(b, "100%"), "percent");

    n = snprintf(b, 4, "abc");
    ok(n == 3 && eq(b, "abc"), "exact fit");

    b[0] = b[1] = b[2] = b[3] = '?';
    n = snprintf(b, 4, "abcdef");
    ok(n == 6 && b[0] == 'a' && b[1] == 'b' &&
       b[2] == 'c' && b[3] == 0, "truncate and return full length");

    b[0] = '?';
    n = snprintf(b, 1, "abc");
    ok(n == 3 && b[0] == 0, "size one");

    b[0] = 'X';
    n = snprintf(b, 0, "abc");
    ok(n == 3 && b[0] == 'X', "size zero writes nothing");

    n = snprintf(b, sizeof(b), "%s", "");
    ok(n == 0 && b[0] == 0, "empty string");

    b[0] = b[1] = 'X';
    n = snprintf(b, sizeof(b), "%c", 0);
    ok(n == 1 && b[0] == 0 && b[1] == 0, "nul character");

    n = snprintf(b, sizeof(b), "%x", (unsigned int)-1);
    ok(n == 8 && eq(b, "ffffffff"), "unsigned max hex");

    n = snprintf(NULL, 0, "abcdef");
    ok(n == 6, "NULL with size zero");

    snprintf(b,sizeof b,"%.*g",6,1.5);ok(!strcmp(b,"1.5"),"Perl Gconvert formatting double");
    snprintf(b,sizeof b,"%.*g",6,1.5f);ok(!strcmp(b,"1.5"),"Perl Gconvert formatting float");
    snprintf(b,sizeof b,"%.2f",-2.5);ok(!strcmp(b,"-2.50"),"float formatting");

    return fail != 0;
}
