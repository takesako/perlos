#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <dirent.h>
#include <romfs.h>
#include <io.h>
#include <heap.h>
#include <timer.h>

_Static_assert(sizeof(NV)==4, "PerlOS requires binary32 NV");
_Static_assert(sizeof(IV)==4 && sizeof(void *)==4, "PerlOS requires ILP32");

#define PTR(t,n) ((volatile t *)(uintptr_t)SvUV(ST(n)))

#define PEEK(n,t) \
XS(XS_##n){ \
    dXSARGS; \
    if(items!=1) XSRETURN_UNDEF; \
    ST(0)=sv_2mortal(newSVuv(*PTR(t,0))); \
    XSRETURN(1); \
}

#define POKE(n,t) \
XS(XS_##n){ \
    dXSARGS; \
    if(items!=2) XSRETURN_UNDEF; \
    *PTR(t,0)=(t)SvUV(ST(1)); \
    XSRETURN_EMPTY; \
}

PEEK(peek8, uint8_t)
PEEK(peek16,uint16_t)
PEEK(peek32,uint32_t)

POKE(poke8, uint8_t)
POKE(poke16,uint16_t)
POKE(poke32,uint32_t)

XS(XS_bits32)
{
    dXSARGS;
    volatile uint32_t *p;
    uint32_t mask;

    if(items<2||items>3) XSRETURN_UNDEF;

    p=PTR(uint32_t,0);
    mask=(uint32_t)SvUV(ST(1));

    if(items==2) {
        ST(0)=sv_2mortal(newSVuv(*p&mask));
        XSRETURN(1);
    }

    *p=(*p&~mask)|((uint32_t)SvUV(ST(2))&mask);
    XSRETURN_EMPTY;
}

XS(XS_flip32)
{
    dXSARGS;
    if(items!=2) XSRETURN_UNDEF;
    *PTR(uint32_t,0)^=(uint32_t)SvUV(ST(1));
    XSRETURN_EMPTY;
}

XS(XS_pwd)
{
    dXSARGS;
    char s[256];
    if(items) XSRETURN_UNDEF;
    if(getcwd(s,sizeof s)) printf("%s\n",s);
    XSRETURN_EMPTY;
}

static void ls_add(const char *s, void *p)
{
    av_push((AV *)p,newSVpv(s,0));
}

XS(XS_ls)
{
    dXSARGS;
    AV *a=newAV();
    I32 n,i;

    if(items) XSRETURN_UNDEF;
    romfs_each(ls_add,a);

    n=av_len(a)+1;
    EXTEND(SP,n);
    for(i=0;i<n;i++) XPUSHs(sv_2mortal(newSVsv(*av_fetch(a,i,0))));
    SvREFCNT_dec(a);
    XSRETURN(n);
}

XS(XS_getchar_timeout)
{
    dXSARGS;
    if(items!=1) XSRETURN_UNDEF;
    ST(0)=sv_2mortal(newSViv(getchar_timeout((int)SvIV(ST(0)))));
    XSRETURN(1);
}

XS(XS_gettick)
{
    dXSARGS;
    if (items) croak("Usage: gettick()");
    ST(0)=sv_2mortal(newSVuv((UV)gettick()));
    XSRETURN(1);
}

XS(XS_gettick_diff)
{
    dXSARGS;
    if (items != 1) croak("Usage: gettick_diff(start)");
    unsigned start=(unsigned)SvUV(ST(0));
    ST(0)=sv_2mortal(newSVuv((UV)(unsigned)(gettick()-start)));
    XSRETURN(1);
}

XS(XS_meminfo)
{
    dXSARGS;
    struct heap_stats s;
    if (items) croak("Usage: meminfo()");
    if (heap_get_stats(&s)) croak("heap metadata corrupt");
    HV *h=newHV();
#define FIELD(n) hv_store(h,#n,sizeof(#n)-1,newSVuv((UV)s.n),0)
    FIELD(total); FIELD(used); FIELD(free); FIELD(largest); FIELD(overhead);
    FIELD(peak_used); FIELD(peak_occupied); FIELD(used_blocks); FIELD(free_blocks);
    FIELD(malloc_calls); FIELD(calloc_calls); FIELD(realloc_calls); FIELD(free_calls);
    FIELD(realloc_inplace); FIELD(realloc_moved); FIELD(failures);
#undef FIELD
    ST(0)=sv_2mortal(newRV_noinc((SV *)h));
    XSRETURN(1);
}

void perlos_init(pTHX)
{
    newXSproto("main::pwd",    XS_pwd,    __FILE__,"");
    newXSproto("main::ls",     XS_ls,     __FILE__,";*");
    newXSproto("main::peek8",  XS_peek8,  __FILE__,"$");
    newXSproto("main::peek16", XS_peek16, __FILE__,"$");
    newXSproto("main::peek32", XS_peek32, __FILE__,"$");
    newXSproto("main::poke8",  XS_poke8,  __FILE__,"$$");
    newXSproto("main::poke16", XS_poke16, __FILE__,"$$");
    newXSproto("main::poke32", XS_poke32, __FILE__,"$$");
    newXSproto("main::bits32", XS_bits32, __FILE__,"$$;$");
    newXSproto("main::flip32", XS_flip32, __FILE__,"$$");
    newXSproto("main::getchar_timeout",XS_getchar_timeout,__FILE__,"$");
    newXSproto("main::gettick",        XS_gettick,        __FILE__, "");
    newXSproto("main::gettick_diff",   XS_gettick_diff,   __FILE__, "$");
    newXSproto("main::meminfo",        XS_meminfo,        __FILE__, "");
}

static void dump_str(SV *v)
{
    STRLEN n; const char *s=SvPV(v,n);
    putchar('"');
    while(n--) {
        unsigned char c=*s++;
        if(c=='\n') fputs("\\n",stdout);
        else if(c=='\r') fputs("\\r",stdout);
        else if(c=='\t') fputs("\\t",stdout);
        else if(c=='"'||c=='\\') { putchar('\\'); putchar(c); }
        else putchar(c);
    }
    putchar('"');
}

static void dump_sv(SV *v)
{
    if(!v||!SvOK(v)) { fputs("undef",stdout); return; }
    if(!SvROK(v)) {
        if(SvIOK(v)||SvNOK(v)) fputs(SvPV_nolen(v),stdout);
        else dump_str(v);
        return;
    }
    v=SvRV(v);
    if(SvTYPE(v)==SVt_PVAV) {
        AV *a=(AV *)v; I32 n=av_len(a);
        fputc('[',stdout);
        for(I32 i=0;i<=n;i++) {
            if(i) fputs(", ",stdout);
            SV **x=av_fetch(a,i,0); dump_sv(x?*x:0);
        }
        fputc(']',stdout);
    } else if(SvTYPE(v)==SVt_PVHV) {
        HV *h=(HV *)v; HE *e; int first=1;
        fputc('{',stdout); hv_iterinit(h);
        while((e=hv_iternext(h))) {
            if(!first) fputs(", ",stdout); first=0;
            dump_str(hv_iterkeysv(e)); fputs(" => ",stdout);
            dump_sv(hv_iterval(h,e));
        }
        fputc('}',stdout);
    } else {
        fputc('\\',stdout); dump_sv(v);
    }
}

int perlos_readline(char *s, size_t cap)
{
    static int skip_lf;
    size_t n=0; int too_long=0, escape=0;
    if (!cap) return -3;
    for (;;) {
        int c=getchar();
        if(c<0) { s[n]=0; return too_long?-3:n?(int)n:-1; }
        if(skip_lf) { skip_lf=0; if(c=='\n')continue; }
        if(c=='\r'||c=='\n') {
            skip_lf=c=='\r'; s[n]=0; fputs("\r\n",stdout);
            return too_long?-3:(int)n;
        }
        if(c==3) { fputs("^C\r\n",stdout); s[0]=0; return -2; }
        if(c==4) {
            if(too_long) { s[0]=0; return -3; }
            s[n]=0; if(n)fputs("\r\n",stdout); return n?(int)n:-1;
        }
        /* Consume CSI/SS3 navigation keys; no history or cursor editing. */
        if(escape) {
            if(escape==1 && (c=='['||c=='O')) escape=2;
            else if(escape==1 || (c>=0x40&&c<=0x7e)) escape=0;
            continue;
        }
        if(c==27) { escape=1; continue; }
        if(c==8||c==127) {
            if(n && !too_long) { n--; fputs("\b \b",stdout); }
            continue;
        }
        if((c<32 && c!='\t') || c>255)continue;
        if(too_long)continue;
        if(n+1>=cap) { too_long=1; continue; }
        s[n++]=(char)c; putchar(c);
    }
}

static PerlInterpreter *my_perl;
static char line[4096];
static int monitor_heap;

static void evaluate(const char *text, size_t len)
{
    dSP;
    ENTER; SAVETMPS;
    SV *src=newSVpvn(text,(STRLEN)len);
    PUTBACK;
    int n=eval_sv(src,G_ARRAY|G_EVAL);
    SvREFCNT_dec(src);
    SPAGAIN;
    if(SvTRUE(ERRSV)) {
        STRLEN l;
        const char *s=SvPV(ERRSV,l);
        fwrite(s,1,l,stderr);
    } else {
        SV **p=SP-n+1;
        for(int i=0;i<n;i++) {
            dump_sv(p[i]);
            putchar('\n');
        }
    }
    SP-=n;
    PUTBACK;
    FREETMPS; LEAVE;
    if (monitor_heap) heap_dump("after eval");
}

int perlos_repl(void)
{
    int argc=4;
    char arg0[]="picoperl", arg1[]="-Ilib", arg2[]="boot.pl", arg3[]="";
    char *args[]={arg0,arg1,arg2,arg3,0};
    char **argv=args, *empty[]={0}, **env=empty;
    int status=0, multiline=0;
    char *block=0; size_t used=0;
    PERL_SYS_INIT3(&argc,&argv,&env);
    my_perl=perl_alloc();
    if(!my_perl) { fputs("perl_alloc failed\n",stderr); PERL_SYS_TERM(); return 1; }
    perl_construct(my_perl);
    PL_exit_flags |= PERL_EXIT_DESTRUCT_END;
    PL_perl_destruct_level=1;
    status=perl_parse(my_perl,perlos_init,argc,argv,env);
    if(!status) status=perl_run(my_perl);
    if(status) goto done;
    for(;;) {
        fputs(multiline?"....> ":"perl> ",stdout); fflush(stdout);
        int n=perlos_readline(line,sizeof line);
        if(n==-1) { fputc('\n',stdout); break; }
        if(n<0) {
            if(n==-3) puts("input too long; discarded");
            free(block); block=0; used=0; multiline=0; continue;
        }
        if(!strcmp(line,":quit"))break;
        if(!multiline && !strncmp(line,":mem",4) && (!line[4] || line[4]==' ')) {
            if(!strcmp(line,":mem on")) monitor_heap=1;
            else if(!strcmp(line,":mem off")) monitor_heap=0;
            else if(!strcmp(line,":mem reset")) heap_reset_peak();
            else if(!strcmp(line,":mem check")) {
                puts(heap_check()?"heap check: CORRUPT":"heap check: OK"); continue;
            } else if(strcmp(line,":mem")) {
                puts("Usage: :mem [on|off|reset|check]"); continue;
            }
            heap_dump("repl"); continue;
        }
        if(!strcmp(line,":cancel")) { free(block); block=0; used=0; multiline=0; continue; }
        if(!multiline && !strcmp(line,":help")) {
            puts(":mem [on|off|reset|check] monitors the C heap without allocating.");
            puts("ticks(), elapsed_ms(start), meminfo() are available from Perl.");
            puts("Evaluate one line in list context. Results are printed automatically.");
            puts("Globals persist; lexical my variables do not persist between evaluations.");
            puts("Use :{ and :} for multiline input. Type :quit or Ctrl-D to exit.");
            continue;
        }
        if(!multiline && !strcmp(line,":{")) { multiline=1; continue; }
        if(multiline && !strcmp(line,":}")) {
            if(used)evaluate(block,used);
            free(block); block=0; used=0; multiline=0; continue;
        }
        if(multiline) {
            if(used+(size_t)n+2>65536) {
                puts("multiline limit exceeded; discarded");
                free(block); block=0; used=0; multiline=0; continue;
            }
            char *p=realloc(block,used+(size_t)n+2);
            if(!p) {
                heap_dump_failure("REPL multiline input",0);
                puts("out of memory; input discarded");
                free(block); block=0; used=0; multiline=0; continue;
            }
            block=p; memcpy(block+used,line,(size_t)n); used+=(size_t)n;
            block[used++]='\n'; block[used]=0;
        } else if(n) evaluate(line,(size_t)n);
    }
done:
    free(block);
    { int r=perl_destruct(my_perl); if(!status)status=r; }
    perl_free(my_perl); my_perl=0; PERL_SYS_TERM(); return status;
}
