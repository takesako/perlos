#!/usr/bin/perl
use strict;
use warnings;

sub append_once {
    my($file,$mark,$text)=@_;
    open my $f,'<',$file or die "$file: $!\n";
    local $/; my $s=<$f>; close $f;
    return if index($s,$mark)>=0;
    open $f,'>>',$file or die "$file: $!\n";
    print $f "\n" unless $s=~/\n\z/;
    print $f "\n$mark\n$text";
    close $f;
}


sub patch_pp_sys {
    my $file='pp_sys.c';
    my $mark='/* PERLOS_TIME64_SV_INT */';
    open my $f,'<',$file or die "$file: $!\n";
    local $/; my $s=<$f>; close $f;
    return if index($s,$mark)>=0;

    if($s=~/^([ \t]*)(?:NV|double)\s+input\s*=\s*Perl_floor\(POPn\);\n[ \t]*when\s*=\s*\(Time64_T\)input;\n[ \t]*if\s*\(when\s*!=\s*input\)\s*\{/m){
        my $i=$1;
        my $new=
            $i."/* PERLOS_TIME64_SV_INT */\n".
            $i."SV * const time_arg = POPs;\n".
            $i."NV input = 0;\n".
            $i."bool integer_input;\n".
            $i."SvGETMAGIC(time_arg);\n".
            $i."integer_input = SvIOK(time_arg);\n".
            $i."if (integer_input)\n".
            $i."    when = SvIsUV(time_arg) ? (Time64_T)SvUV(time_arg) : (Time64_T)SvIV(time_arg);\n".
            $i."else {\n".
            $i."    input = Perl_floor(SvNV(time_arg));\n".
            $i."    when = (Time64_T)input;\n".
            $i."}\n".
            $i."if (!integer_input && when != input) {";
        substr($s,$-[0],$+[0]-$-[0],$new);
    }else{
        die "$file: unexpected source\n";
    }

    open $f,'>',$file or die "$file: $!\n";
    print $f $s;
    close $f or die "$file: $!\n";
}

sub patch_time64 {
    my $file='time64.c';
    my $mark='/* PERLOS_TIME64_INT */';
    open my $f,'<',$file or die "$file: $!\n";
    local $/; my $s=<$f>; close $f;
    return if index($s,$mark)>=0;

    my @p=(
<<'OLD1', <<'NEW1',
    v_tm_sec  = (int)fmod(time, 60.0);
    time      = time >= 0 ? floor(time / 60.0) : ceil(time / 60.0);
    v_tm_min  = (int)fmod(time, 60.0);
    time      = time >= 0 ? floor(time / 60.0) : ceil(time / 60.0);
    v_tm_hour = (int)fmod(time, 24.0);
    time      = time >= 0 ? floor(time / 24.0) : ceil(time / 24.0);
    v_tm_tday = time;
OLD1
    /* PERLOS_TIME64_INT */
    v_tm_sec  = (int)(time % 60);
    time     /= 60;
    v_tm_min  = (int)(time % 60);
    time     /= 60;
    v_tm_hour = (int)(time % 24);
    time     /= 24;
    v_tm_tday = time;
NEW1
<<'OLD2', <<'NEW2',
    v_tm_wday = (int)fmod((v_tm_tday + 4.0), 7.0);
OLD2
    v_tm_wday = (int)((v_tm_tday % 7 + 4) % 7);
NEW2
<<'OLD3', <<'NEW3',
        cycles = (int)floor(m / (Time64_T) days_in_gregorian_cycle);
OLD3
        cycles = (int)(m / (Time64_T)days_in_gregorian_cycle);
NEW3
<<'OLD4', <<'NEW4',
        cycles = (int)ceil((m / (Time64_T) days_in_gregorian_cycle) + 1);
OLD4
        cycles = (int)(m / (Time64_T)days_in_gregorian_cycle) + 1;
NEW4
    );

    for(my $i=0;$i<@p;$i+=2){
        index($s,$p[$i])>=0 or die "$file: unexpected source\n";
    }
    for(my $i=0;$i<@p;$i+=2){
        $s=~s/\Q$p[$i]\E/$p[$i+1]/ or die "$file: patch failed\n";
    }

    open $f,'>',$file or die "$file: $!\n";
    print $f $s;
    close $f or die "$file: $!\n";
}

append_once('perl.h','/* PERLOS_FLOAT */',<<'END');
#ifndef FLT_DIG
#include <float.h>
#endif

#undef NV_DIG
#define NV_DIG FLT_DIG
#undef NV_MANT_DIG
#define NV_MANT_DIG FLT_MANT_DIG
#undef NV_MIN
#define NV_MIN FLT_MIN
#undef NV_MAX
#define NV_MAX FLT_MAX
#undef NV_MIN_10_EXP
#define NV_MIN_10_EXP FLT_MIN_10_EXP
#undef NV_MAX_10_EXP
#define NV_MAX_10_EXP FLT_MAX_10_EXP
#undef NV_EPSILON
#define NV_EPSILON FLT_EPSILON

#undef Perl_cos
#define Perl_cos cosf
#undef Perl_sin
#define Perl_sin sinf
#undef Perl_sqrt
#define Perl_sqrt sqrtf
#undef Perl_exp
#define Perl_exp expf
#undef Perl_log
#define Perl_log logf
#undef Perl_atan2
#define Perl_atan2 atan2f
#undef Perl_pow
#define Perl_pow powf
#undef Perl_floor
#define Perl_floor floorf
#undef Perl_ceil
#define Perl_ceil ceilf
#undef Perl_fmod
#define Perl_fmod fmodf
#undef Perl_modf
#define Perl_modf(x,y) modff(x,y)
#undef Perl_frexp
#define Perl_frexp(x,y) frexpf(x,y)
END

append_once('time64_config.h','/* PERLOS_TIME64 */',<<'END');
#undef INT_64_T
#define INT_64_T I64TYPE
END

patch_time64();
patch_pp_sys();
