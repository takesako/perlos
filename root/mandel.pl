$| = 1;
my @c = (0..9, 'A'..'E', ' ');
for ($f = 0; $f < 230; ++$f) {
  my $z  = 0.0001 + ($f % 180) / 50;
  my $sx = .0458 / $z;
  my $sy = .08333 / $z;
  my $out = "\e[H\e[2J";
  for my $y (-12 .. 12) {
    my $cb = $y * $sy;
    for my $x (-39 .. 39) {
      my $ca = $x * $sx - .23;
      my ($a, $b) = ($ca, $cb);
      my $i = 0;
      while ($i < 15) {
        my $aa = $a * $a;
        my $bb = $b * $b;
        last if $aa + $bb > 4;
        $b = 2 * $a * $b + $cb;
        $a = $aa - $bb + $ca;
        ++$i;
      }
      $out .= $c[$i];
    }
    $out .= "\n";
  }
  print $out;
  select(undef, undef, undef, 1/60);
}
return;