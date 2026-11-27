local $|=1;
my ($w,$h)=(32,16);
my ($b,$pause)=('',0);
sub init {
  $b="\0" x (($w*$h+7)>>3);
  vec($b,$_,1)=rand()<.25 for 0..$w*$h-1;
}
init();
print "\e[2J\e[?25l";
while(1){
  my $c=getchar_timeout(0);
  last if $c==113;
  $pause=!$pause if $c==32;
  init() if $c==114;
  print "\e[H LIFE GAME ${w}x${h}  ✨→🔥→💨  SPACE:pause  r:random  q:quit\e[K\n";
  my $next="\0" x (($w*$h+7)>>3);
  for my $y (0..$h-1){
    for my $x (0..$w-1){
      my $k=$x+$w*$y;
      my $n=-(my $a=vec($b,$k,1));
      for my $dy (-1..1){
        $n+=vec($b,($x+$_)%$w+$w*(($y+$dy)%$h),1) for -1..1;
      }
      my $v=vec($next,$k,1)=($n==3||($a&&$n==2))?1:0;
      print $a?($v?"🔥":"💨"):($v?"✨":'  ');
    }
    print "\e[K\n";
  }
  $b=$next unless $pause;
  select(undef,undef,undef,.15);
}
print "\e[?25h\n";
return;
