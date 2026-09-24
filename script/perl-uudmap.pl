#!/usr/bin/perl
use strict;
use warnings;

sub output {
    my ($f, $a) = @_;
    open my $o, '>', $f or die "$0: $f: $!\n";
    print $o "{\n    ";
    print $o "$a->[$_]" . ($_ == $#$a ? '' : ', ') .
        (($#$a - $_) % 16 == 0 && $_ < $#$a ? "\n    " : '')
        for 0 .. $#$a;
    print $o "\n}\n";
    close $o or die "$0: $f: $!\n";
}

my ($u, $b) = @ARGV;
$u ||= 'uudmap.h';
$b ||= 'bitcount.h';

my $m = '`!"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_';
my @u = (0) x 256;
$u[ord substr $m, $_, 1] = $_ for 0 .. length($m) - 1;
$u[32] = 0;
output $u, \@u;

my @b = map { unpack('%8b*', pack 'C', $_) } 0 .. 255;
output $b, \@b;
