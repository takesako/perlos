sub cat(*) {
    open my $f,'<',$_[0] or do { warn "cat: $_[0]: $!\n"; return };
    print while <$f>;
    return;
}
sub is_pico2 { (peek32(0x40000000) & 0x0fffffff) == 0x4927; }
print<<'EOF';
 ____           _  ___  ____
|  _ \ ___ _ __| |/ _ \/ ___|
| |_) / _ \ '__| | | | \\___\
|  __/  __/ |  | | |_| |___) |
|_|   \___|_|  |_|\___/|____/
EOF
print "PerlOS (microperl $^V, NV=float, ROMFS2)\n";
print is_pico2() ? "Raspberry Pi Pico 2" : "Arm MPS2+ AN505";
print " / Cortex-M33 FPv5-SP-D16\n";
print "type> ls; cat 'boot.pl'; :help :{ ... :} :quit\n";
print "\n";
1;