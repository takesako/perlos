#!/usr/bin/perl
use strict;
use warnings;

if ($^O eq 'darwin') {
    require IO::Select;
    my $dev = shift || (glob('/dev/cu.usbmodem*'))[0] || die "Pico CDC not found\n";
    system('stty','-f',$dev,'115200','raw','-echo') == 0 or die "stty $dev failed\n";
    open my $p,'+<',$dev or die "$dev: $!\n";
    binmode $p; binmode STDIN; binmode STDOUT;
    system('stty','raw','-echo');
    $SIG{INT}=sub{system('stty','sane');exit};
    END { system('stty','sane') if -t STDIN }
    warn "$dev connected (ESC to exit)\n";
    my $s=IO::Select->new(\*STDIN,$p);
    while(1){for my $f($s->can_read){sysread($f,my $c,1024) or next;if(fileno($f)==fileno(STDIN)){exit if $c eq "\e";syswrite($p,$c)}else{syswrite(STDOUT,$c)}}}
}

require Encode; require MIME::Base64;
my $com=uc(shift//'COM3'); die "usage: $0 [COM3]\n" unless $com=~/^COM\d+$/;
my $ps=qq{
[Console]::OutputEncoding=[Text.UTF8Encoding]::new(\$false);
\$p=[IO.Ports.SerialPort]::new('$com',115200,[IO.Ports.Parity]::None,8,[IO.Ports.StopBits]::One);
\$p.Encoding=[Text.Encoding]::UTF8;
\$p.DtrEnable=\$true;\$p.Open();
[Console]::Error.WriteLine("$com connected (ESC to exit)");
while(\$true){
 \$r=\$p.ReadExisting();
 if(\$r){[Console]::Out.Write(\$r)}
 if([Console]::KeyAvailable){
  \$k=[Console]::ReadKey(\$true);
  if(\$k.Key -eq 'Escape'){break}
  if(\$k.KeyChar -ne [char]0){\$p.Write([string]\$k.KeyChar)}
 }
 Start-Sleep -Milliseconds 5
}
\$p.Close()
};
my $b64=MIME::Base64::encode_base64(Encode::encode('UTF-16LE',$ps),'');
system {'powershell.exe'} 'powershell.exe','-NoProfile','-EncodedCommand',$b64;
exit $? >> 8;
