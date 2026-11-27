local $|=1;
my ($w,$h,$t,$speed)=(78,22,0,60);
my $kana="ｱｲｳｴｵｶｷｸｹｺｻｼｽｾｿﾀﾁﾂﾃﾄﾅﾆﾇﾈﾉﾊﾋﾌﾍﾎﾏﾐﾑﾒﾓﾔﾕﾖﾗﾘﾙﾚﾛﾜﾝ";
print "\e[2J\e[?25l";
while(1){
  my $c=getchar_timeout(0);
  $c='' unless defined $c;
  $c=chr($c) if $c=~/^(?:3|\d{2,3})$/;
  last if $c eq 'q' || $c eq "\3";
  $speed-=10 if $c eq 'f' && $speed>20;
  $speed+=10 if $c eq 's' && $speed<250;
  print "\e[H\e[38;5;46m MATRIX ${w}x${h}  f:faster  s:slower  q:quit\e[K\n";
  my $color=46;
  for my $y (0..$h-1){
    for(my $x=0;$x<$w;$x+=2){
      my $len=7+($x*13+int($x/6))%15;
      my $cycle=$h+$len+($x*17)%11;
      my $p=int($t*(2+($x*7+int($x/8))%5)/4)+$x*37;
      my $d=$p%$cycle-$y;
      if($d<0 || $d>=$len){print '  ';next;}
      my $v=$d==0?231:$d==1?157:$d<4?46:$d*2<$len?40:$d*4<$len*3?34:$d+2<$len?28:22;
      print "\e[38;5;${v}m" if $v!=$color;
      $color=$v;
      my $n=$x*137+$y*71+int($p/$cycle)*53+int(($t+$x+$y)%23==0)*17;
      $n=($n*($n%97+13))%997;
      print(($n%4?substr($kana,3*($n%(length($kana)/3)),3):substr('0123456789ABCDEF',$n%16,1)), ' ');
    }
    print "\e[K\n";
  }
  ++$t;
  select(undef,undef,undef,$speed/1000);
}
print "\e[0m\e[?25h\n";
return;
