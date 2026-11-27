local $|=1;
my ($w,$h,$t)=(38,18,0);
my $now;
sub rnd { int(rand($_[0])) }
sub clamp { $_[0]<$_[1]?$_[1]:$_[0]>$_[2]?$_[2]:$_[0] }
sub bg { $_[0]==$h-1?94:$_[0]==$h-2?58:(31,24,25,18,17)[int($_[0]*5/($h-2))] }
my @glyph=('  ',"\e[38;5;117m~ \e[39m","\e[38;5;74m. \e[39m","\e[38;5;81mo \e[39m",
  "🫧","🌿","🪸","🐚","🪨","🐟","🐠","🐡","🦈","🪼","🐙","🦀");
sub put {
  my ($x,$y,$g)=@_;
  return if $x<-.5 || $y<-.5 || $x>=$w-.5 || $y>=$h-.5;
  vec($now,int($x+.5)+$w*int($y+.5),8)=$g;
}
my @plant=map { int(($_+.5)*$w/6)+rnd(3)-1 } 0..5;
my $old=$now="\0" x ($w*$h);
for my $p (@plant){ put($p,$h-1,5); put($p+1,$h-1,7+rnd(2)); }
put(2,$h-2,6); put($w-3,$h-2,6);
my $base=$now;
my @school=([rand($w),4,.24,rand(6.28),9],[rand($w),8,.17,rand(6.28),10]);
my @fish;
for my $spec ([9,2,11,.16,.6],[10,3,11,.14,.7],[10,2,10,.21,.8],
  [11,6,12,.075,.9],[12,2,7,.42,.45],[13,4,12,.045,1.5],[14,12,14,.055,.4],[15,17,17,.09,0]){
  my ($g,$lo,$hi,$v,$amp)=@$spec;
  push @fish,[rand($w-1),$lo+rand($hi-$lo),-$v,-$v,rand(6.28),$g,$lo,$hi,$amp,$v];
}
my @bubble=map { [$plant[rnd(scalar @plant)],1+rand($h-5),.09+rand(.12),rand(6.28)] } 1..12;
print "\e[0m\e[2J\e[H\e[?25l\e[38;5;117m AQUARIUM ${w}x${h}  q:quit\e[0m\e[K";
for my $y (0..$h-1){ print "\e[",$y+2,";1H\e[48;5;",bg($y),"m",'  ' x $w; }
while(1){
  my $c=getchar_timeout(0); last if $c==113 || $c==3;
  $now=$base;
  for my $x (0..$w-1){
    put($x,0,1) if sin($x*.38-$t*.13)+.5*sin($x*.8+$t*.09)>.6;
    put($x,1,2) if sin($x*.35-$t*.075)>.91;
  }
  for my $i (0..6){ put(($i*13+$t*.045+2*sin($t*.028+$i))%$w,2+3*sin($i*.67)**2,2); }
  for my $b (@bubble){
    $b->[1]-=$b->[2];
    @$b=($plant[rnd(scalar @plant)],$h-4+rand(2),.09+rand(.12),rand(6.28)) if $b->[1]<.7;
    put($b->[0]+sin($t*.065+$b->[3]),$b->[1],$b->[1]>10?2:$b->[1]>5?3:4);
  }
  for my $p (@plant){
    for my $j (1..2+$p%3){ put($p+$j*.42*sin($t*.06+$p*.24),$h-1-$j,5); }
  }
  for my $s (@school){
    $s->[0]-=$s->[2]*(1+.22*sin($t*.035+$s->[3]));
    ($s->[0],$s->[1])=($w+rand(4),3+rand(7)) if $s->[0]<-6;
    for my $i (0..4){
      put($s->[0]+2*int(($i+1)/2),$s->[1]+($i?($i%2?-1:1):0)+.55*sin($t*.065+$s->[3]+$i*.5),$s->[4]);
    }
  }
  for my $f (@fish){
    $f->[3]=(rnd(2)?1:-1)*$f->[9]*(.65+rand(.7)) if rand()<.008;
    $f->[3]=abs($f->[3]) if $f->[0]<.8;
    $f->[3]=-abs($f->[3]) if $f->[0]>$w-1.8;
    $f->[2]+=.075*($f->[3]-$f->[2]);
    $f->[0]=clamp($f->[0]+$f->[2],0,$w-1);
    $f->[1]=clamp($f->[1]+.025*sin($t*.027+$f->[4]),$f->[6],$f->[7]);
    put($f->[0],clamp($f->[1]+$f->[8]*sin($t*.055+$f->[4]),$f->[6],$f->[7]),$f->[5]);
  }
  my ($diff,$out,$last,$row)=($old ^ $now,'',-2,-1);
  while($diff=~/[^\x00]/g){
    my $k=pos($diff)-1; my $y=int($k/$w);
    $out.="\e[".($y+2).';'.(2*($k%$w)+1).'H' if $k!=$last+1 || $row!=$y;
    $out.="\e[48;5;".bg($y).'m' if $row!=$y;
    $out.=$glyph[vec($now,$k,8)]; ($last,$row)=($k,$y);
  }
  print $out if length($out);
  $old=$now; ++$t;
  select(undef,undef,undef,.05);
}
print "\e[0m\e[",$h+2,";1H\e[?25h\n";
return;
