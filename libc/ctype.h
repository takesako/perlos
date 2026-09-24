#ifndef _CTYPE_H
#define _CTYPE_H

#define _BETWEEN(c,a,b) ((unsigned)(c)-(a)<=(unsigned)((b)-(a)))

static inline int isupper(int c){return _BETWEEN(c,'A','Z');}
static inline int islower(int c){return _BETWEEN(c,'a','z');}
static inline int isalpha(int c){return isupper(c)||islower(c);}
static inline int isdigit(int c){return _BETWEEN(c,'0','9');}
static inline int isalnum(int c){return isalpha(c)||isdigit(c);}
static inline int isspace(int c){return c==' '||_BETWEEN(c,'\t','\r');}
static inline int iscntrl(int c){return (unsigned)c<32||c==127;}
static inline int isprint(int c){return _BETWEEN(c,32,126);}
static inline int isgraph(int c){return _BETWEEN(c,33,126);}
static inline int ispunct(int c){return isgraph(c)&&!isalnum(c);}
static inline int isxdigit(int c){return isdigit(c)||_BETWEEN(c,'A','F')||_BETWEEN(c,'a','f');}
static inline int isascii(int c){return (unsigned)c<128;}
static inline int isblank(int c){return c==' '||c=='\t';}
static inline int toupper(int c){return islower(c)?c-'a'+'A':c;}
static inline int tolower(int c){return isupper(c)?c-'A'+'a':c;}

#endif
