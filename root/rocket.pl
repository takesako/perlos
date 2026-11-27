local $|=1;
my ($px,$y,$t,$step,$esc)=(0,3,0,0,0);
my $b="\0" x 24;
print "\e[2J\e[?25l";
GAME: for(;;++$t){
  my ($dx,$dy)=(0,0);
  for(1..8){
    my $c=getchar_timeout(0)//-1;
    last if $c<0;
    last GAME if $c==113 || $c==3;
    if($c==27){$esc=1;next;}
    if($esc==1){$esc=($c==91||$c==79)?2:0;next;}
    if($esc==2){
      $c=$c==65?119:$c==66?115:$c==67?100:$c==68?97:0;
      $esc=0;
    }
    $dy=-1 if $c==119;
    $dy= 1 if $c==115;
    $dx=-1 if $c==97;
    $dx= 1 if $c==100;
  }
  $px+=$dx if $px+$dx>=0 && $px+$dx<24;
  $y+=$dy if $y+$dy>=0 && $y+$dy<7;
  my $hit=vec($b,$px,8)&(1<<$y);
  if(!$hit && $t%2==0){
    my $m=$step++%8==0?1<<int(rand(7)):0;
    substr($b,0,1,''); $b.=chr($m);
    $hit=vec($b,$px,8)&(1<<$y);
  }
  print "\e[H ROCKET  SCORE:$t  WASD/arrows:move  q:quit\e[K\n";
  for my $r (0..6){
    my $line='';
    for my $x (0..23){
      my $star='  ';
      for my $i (0..4){
        my $n=($x+($t>>$i)+17*$r+23*$i)%251;
        next if $n*($n+17+13*$r+7*$i)%127>=3;
        $star="\e[38;5;".(255-4*$i)."m. \e[0m";
        last;
      }
      $line.=$x==$px && $r==$y?($hit?"💥":"🚀"):
        vec($b,$x,8)&(1<<$r)?"🪨":$star;
    }
    print "$line\e[K\n";
  }
  if($hit){print "\e[31mGAME OVER  SCORE:$t\e[0m\e[K\n";last;}
  select(undef,undef,undef,.05);
}
print "\e[0m\e[?25h\n";
return;
