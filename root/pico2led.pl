# Raspberry Pi Pico 2 LED blinking
if (PerlOS::load(0x40000000) & 0x0fffffff == 0x4927) {
  PerlOS::store(0x40020000, 0x40, CLR_MASK); # Clear IO_BANK0 reset
  1 until PerlOS::load(0x40020008) & 0x40;   # Wait until reset done

  PerlOS::store(0x400280cc, 5);              # GPIO25 = SIO
  PerlOS::store(0x40038068, 0x40, SET_MASK); # IE=1
  PerlOS::store(0x40038068, 0x180, CLR_MASK);# OD=0, ISO=0
  PerlOS::store(0xd0000038, 1 << 25);        # GPIO25 output enable

  for (1..10) {
    PerlOS::store(0xd0000018, 1 << 25);    # LED high
    select(undef, undef, undef, 0.5);      # Sleep 500 ms

    PerlOS::store(0xd0000020, 1 << 25);    # LED low
    select(undef, undef, undef, 0.5);      # Sleep 500 ms
  }
} else {
  print "Error: Raspberry Pi Pico 2 required.\n";
}
return;