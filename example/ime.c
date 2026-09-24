#include <stdio.h>
#include <string.h>

#define SIZE 512
#define BLINK 500
#define KEYWAIT 50
#define CURSOR1 "□"
#define CURSOR2 "■"
#define CLEN 3

typedef struct {
    const char *from;
    const char *to;
} Conv;

static int replace(char *s, size_t size,
                   const char *from, const char *to)
{
    char *p = strstr(s, from);
    size_t a, b, tail;

    if (!p) return 0;

    a = strlen(from);
    b = strlen(to);
    tail = strlen(p + a);

    if (strlen(s) - a + b >= size) return 0;

    memmove(p + b, p + a, tail + 1);
    memcpy(p, to, b);
    return 1;
}

static void convert(char *s, size_t size,
                    const Conv *c, size_t n)
{
    size_t i;

    for (i = 0; i < n; i++)
        replace(s, size, c[i].from, c[i].to);
}

static void romaji(char *s, size_t size)
{
    static const Conv c[] = {
        {"kya","きゃ"},{"kyu","きゅ"},{"kyo","きょ"},
        {"gya","ぎゃ"},{"gyu","ぎゅ"},{"gyo","ぎょ"},
        {"sha","しゃ"},{"shu","しゅ"},{"sho","しょ"},
        {"sya","しゃ"},{"syu","しゅ"},{"syo","しょ"},
        {"jya","じゃ"},{"jyu","じゅ"},{"jyo","じょ"},
        {"ja","じゃ"},{"ju","じゅ"},{"jo","じょ"},
        {"cha","ちゃ"},{"chu","ちゅ"},{"cho","ちょ"},
        {"tya","ちゃ"},{"tyu","ちゅ"},{"tyo","ちょ"},
        {"nya","にゃ"},{"nyu","にゅ"},{"nyo","にょ"},
        {"hya","ひゃ"},{"hyu","ひゅ"},{"hyo","ひょ"},
        {"bya","びゃ"},{"byu","びゅ"},{"byo","びょ"},
        {"pya","ぴゃ"},{"pyu","ぴゅ"},{"pyo","ぴょ"},
        {"mya","みゃ"},{"myu","みゅ"},{"myo","みょ"},
        {"rya","りゃ"},{"ryu","りゅ"},{"ryo","りょ"},
        {"shi","し"},{"chi","ち"},{"tsu","つ"},{"fu","ふ"},
        {"xtsu","っ"},{"xtu","っ"},
        {"xya","ゃ"},{"xyu","ゅ"},{"xyo","ょ"},
        {"xa","ぁ"},{"xi","ぃ"},{"xu","ぅ"},
        {"xe","ぇ"},{"xo","ぉ"},
        {"kk","っk"},{"ss","っs"},{"tt","っt"},
        {"cc","っc"},{"pp","っp"},{"bb","っb"},
        {"dd","っd"},{"gg","っg"},
        {"nn","ん"},
        {"ka","か"},{"ki","き"},{"ku","く"},
        {"ke","け"},{"ko","こ"},
        {"ga","が"},{"gi","ぎ"},{"gu","ぐ"},
        {"ge","げ"},{"go","ご"},
        {"sa","さ"},{"si","し"},{"su","す"},
        {"se","せ"},{"so","そ"},
        {"za","ざ"},{"zi","じ"},{"zu","ず"},
        {"ze","ぜ"},{"zo","ぞ"},
        {"ta","た"},{"ti","ち"},{"tu","つ"},
        {"te","て"},{"to","と"},
        {"da","だ"},{"di","ぢ"},{"du","づ"},
        {"de","で"},{"do","ど"},
        {"na","な"},{"ni","に"},{"nu","ぬ"},
        {"ne","ね"},{"no","の"},
        {"ha","は"},{"hi","ひ"},{"hu","ふ"},
        {"he","へ"},{"ho","ほ"},
        {"ba","ば"},{"bi","び"},{"bu","ぶ"},
        {"be","べ"},{"bo","ぼ"},
        {"pa","ぱ"},{"pi","ぴ"},{"pu","ぷ"},
        {"pe","ぺ"},{"po","ぽ"},
        {"ma","ま"},{"mi","み"},{"mu","む"},
        {"me","め"},{"mo","も"},
        {"ya","や"},{"yu","ゆ"},{"yo","よ"},
        {"ra","ら"},{"ri","り"},{"ru","る"},
        {"re","れ"},{"ro","ろ"},
        {"wa","わ"},{"wo","を"},
        {"a","あ"},{"i","い"},{"u","う"},
        {"e","え"},{"o","お"}
    };

    convert(s, size, c, sizeof(c) / sizeof(c[0]));
}

static void kanji(char *s, size_t size)
{
    static const Conv c[] = {
        {"cあr","🚗"},
        {"たけさこ","竹迫"},
        {"かみやま","神山"},
        {"こうせん","高専"},
        {"このさき","この先"},
        {"いきのこる","生きのこる"},
        {".","。"},
        {"KMC","神山まるごと高等専門学校"}
    };

    convert(s, size, c, sizeof(c) / sizeof(c[0]));
}

static int ulen(const char *p)
{
    unsigned char c = *p;

    if (c < 0x80) return 1;
    if ((c & 0xe0) == 0xc0) return 2;
    if ((c & 0xf0) == 0xe0) return 3;
    return 4;
}

static char *cur(char *s)
{
    char *p = strstr(s, CURSOR1);
    if (p) return p;
    return strstr(s, CURSOR2);
}

static char *prev_char(char *s, char *p)
{
    if (p <= s) return p;

    p--;
    while (p > s && ((unsigned char)*p & 0xc0) == 0x80)
        p--;
    return p;
}

static void blink(char *s)
{
    char *p = cur(s);

    if (!p) return;

    if (!memcmp(p, CURSOR1, CLEN))
        memcpy(p, CURSOR2, CLEN);
    else
        memcpy(p, CURSOR1, CLEN);
}

static void insert_char(char *s, size_t size, char c)
{
    char *p = cur(s);
    size_t n;

    if (!p || strlen(s) + 1 >= size) return;

    n = strlen(p);
    memmove(p + 1, p, n + 1);
    *p = c;
}

static void backspace(char *s)
{
    char *p = cur(s);
    char *q;

    if (!p || p == s) return;

    q = prev_char(s, p);
    memmove(q, p, strlen(p) + 1);
}

static void delete_char(char *s)
{
    char *p = cur(s);
    char *q;
    char *r;

    if (!p) return;

    q = p + CLEN;
    if (!*q) return;

    r = q + ulen(q);
    memmove(q, r, strlen(r) + 1);
}

static void left(char *s)
{
    char *p = cur(s);
    char *q;
    char tmp[4];
    int n;

    if (!p || p == s) return;

    q = prev_char(s, p);
    n = p - q;

    memcpy(tmp, q, n);
    memmove(q, p, CLEN);
    memcpy(q + CLEN, tmp, n);
}

static void right(char *s)
{
    char *p = cur(s);
    char *q;
    char tmp[4];
    int n;

    if (!p) return;

    q = p + CLEN;
    if (!*q) return;

    n = ulen(q);
    memcpy(tmp, q, n);
    memmove(p + n, p, CLEN);
    memcpy(p, tmp, n);
}

static void move_cursor(char *s, char *to)
{
    char *p = cur(s);
    char c[CLEN];
    size_t n;

    if (!p || to == p) return;

    memcpy(c, p, CLEN);

    if (to < p) {
        n = p - to;
        memmove(to + CLEN, to, n);
        memcpy(to, c, CLEN);
    } else {
        n = to - (p + CLEN);
        memmove(p, p + CLEN, n);
        memcpy(p + n, c, CLEN);
    }
}

static int column(char *start, char *p)
{
    int col = 0;

    while (start < p) {
        start += ulen(start);
        col++;
    }
    return col;
}

static void up(char *s)
{
    char *p = cur(s);
    char *start;
    char *end;
    char *q;
    int col;

    if (!p) return;

    start = p;
    while (start > s && start[-1] != '\n')
        start--;
    if (start == s) return;

    col = column(start, p);
    end = start - 1;
    q = end;

    while (q > s && q[-1] != '\n')
        q--;
    while (col-- && q < end)
        q += ulen(q);

    move_cursor(s, q);
}

static void down(char *s)
{
    char *p = cur(s);
    char *start;
    char *end;
    char *q;
    int col;

    if (!p) return;

    start = p;
    while (start > s && start[-1] != '\n')
        start--;

    col = column(start, p);
    end = p + CLEN;

    while (*end && *end != '\n')
        end += ulen(end);
    if (!*end) return;

    q = end + 1;
    while (*q && *q != '\n' && col--)
        q += ulen(q);

    move_cursor(s, q);
}

static void redraw(const char *s)
{
    printf("\033[2J\033[H%s", s);
}

static void escape_key(char *s)
{
    int a = getchar_timeout(KEYWAIT);
    int b;

    if (a != '[' && a != 'O') return;

    b = getchar_timeout(KEYWAIT);
    if (b == 'A') up(s);
    if (b == 'B') down(s);
    if (b == 'C') right(s);
    if (b == 'D') left(s);

    if (b == '3' && getchar_timeout(KEYWAIT) == '~')
        delete_char(s);
}

int main(void)
{
    char s[SIZE] = CURSOR1;
    int c;

    printf("\033[?25l");
    redraw(s);

    for (;;) {
        c = getchar_timeout(BLINK);

        if (c < 0) {
            blink(s);
            redraw(s);
            continue;
        }

        if (c == 3)
            break;

        if (c == 27) {
            escape_key(s);
        } else if (c == 8 || c == 127) {
            backspace(s);
        } else if (c == '\r' || c == '\n') {
            insert_char(s, sizeof(s), '\n');
        } else if (c >= 0x20 && c < 0x7f) {
            insert_char(s, sizeof(s), c);
            romaji(s, sizeof(s));
            kanji(s, sizeof(s));
        }

        redraw(s);
    }

    printf("\033[?25h\n");
    return 0;
}
