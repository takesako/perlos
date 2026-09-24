#!/usr/bin/perl
# Minimal ELF32/LE/Arm -> Pico 2 flash UF2 packer. Not the official picotool.
use strict;
use warnings;
if (@ARGV == 1 && $ARGV[0] eq 'version') { print "picotool.pl 0.2 (minimal Perl converter)\n"; exit; }
die "usage: perl picotool.pl uf2 convert in.elf out.uf2 --family 0xe48bff59\n"
    unless @ARGV == 6 && $ARGV[0] eq 'uf2' && $ARGV[1] eq 'convert' && $ARGV[4] eq '--family';
my ($in, $out, $fid) = @ARGV[2,3,5];
die "only RP2350 Secure Arm family is supported\n" unless lc($fid) eq '0xe48bff59';
die "input and output paths must differ\n" if lc($in) eq lc($out);
open my $f, '<:raw', $in or die "$in: $!\n";
local $/; my $elf = <$f>; close $f or die "$in: $!\n";
die "expected ELF32 little-endian Arm executable\n" unless length($elf) >= 52
    && substr($elf,0,7) eq "\x7fELF\x01\x01\x01"
    && unpack('v',substr($elf,16,2)) == 2 && unpack('v',substr($elf,18,2)) == 40;
my $phoff = unpack('V',substr($elf,28,4));
my ($phsize,$phnum) = unpack('v2',substr($elf,42,4));
die "bad program header table\n" if $phsize < 32 || !$phnum || $phoff+$phsize*$phnum > length($elf);
my (%page, %used);
for my $i (0 .. $phnum-1) {
    my ($type,$off,$vaddr,$paddr,$filesz,$memsz) = unpack('V6',substr($elf,$phoff+$i*$phsize,24));
    next unless $type == 1 && $filesz;
    die "bad ELF segment\n" if $filesz > $memsz || $off+$filesz > length($elf);
    die "non-flash load segment: use a Pico 2 flash linker script\n"
        if $paddr < 0x10000000 || $paddr+$filesz > 0x10400000;
    for (my $j=0; $j<$filesz; ++$j) {
        my $a=$paddr+$j; my $base=$a & 0xffffff00; my $b=substr($elf,$off+$j,1);
        $page{$base} = "\xff" x 256 unless exists $page{$base};
        die "conflicting load segments\n" if $used{$a} && substr($page{$base},$a-$base,1) ne $b;
        substr($page{$base},$a-$base,1) = $b; $used{$a}=1;
    }
}
my @addr = sort { $a <=> $b } keys %page;
die "no flash data\n" unless @addr;
open my $o, '>:raw', $out or die "$out: $!\n";
for my $i (0 .. $#addr) {
    my $block = pack('V8',0x0a324655,0x9e5d5157,0x2000,$addr[$i],256,$i,scalar(@addr),hex($fid))
              . $page{$addr[$i]} . ("\0" x 220) . pack('V',0x0ab16f30);
    print $o $block or die "$out: $!\n";
}
close $o or die "$out: $!\n";
printf "Wrote %s: %d blocks, %d bytes\n", $out,scalar(@addr),512*@addr;
