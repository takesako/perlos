sub is_pico2 { int(PerlOS::load(0x40000000) & 0x0fffffff == 0x4927); }
sub cat(*) {
  open my $f,'<',$_[0] or do { warn "cat: $_[0]: $!\n"; return };
  print while <$f>; return;
}
sub help {
  print <<'EOF';
PerlOS functions:
  help()                        show this help
  pwd()                         show current directory
  ls()                          list ROMFS files
  cat 'file'                    show file contents
  is_pico2()                    detect Raspberry Pi Pico 2
  getchar_timeout($ms)          read one byte with timeout
  gettick()                     current time in ms
  gettick_diff($start)          ms since $start
  meminfo()                     show C heap statistics
  PerlOS::load($addr)           read 32-bit memory/MMIO
  PerlOS::store($addr,$value)   write 32-bit memory/MMIO
  PerlOS::store($addr,$mask,SET_MASK)    set bits in $mask
  PerlOS::store($addr,$mask,CLR_MASK)  clear bits in $mask
  PerlOS::store($addr,$mask,XOR_MASK) toggle bits in $mask

REPL commands:
  :{ ... :}                     multiline input
  :mem [on|off]                 monitor heap after each eval
  :mem reset                    reset peak heap usage
  :mem check                    check heap integrity
  :quit                         exit PerlOS (Ctrl-D also exits)
EOF
  return;
}
print<<'EOF';
 ____           _  ___  ____
|  _ \ ___ _ __| |/ _ \/ ___|
| |_) / _ \ '__| | | | \\___\
|  __/  __/ |  | | |_| |___) |
|_|   \___|_|  |_|\___/|____/
EOF
print "PerlOS $PerlOS::VERSION (microperl $^V, NV=float, ROMFS2)\n";
print is_pico2() ? "Raspberry Pi Pico 2" : "Arm MPS2+ AN505";
print " / Cortex-M33 FPv5-SP-D16\n";
print "type> help; ls; cat 'boot.pl'; :{ ... :} :quit\n";
print "\n";
return;