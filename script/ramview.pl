#!/usr/bin/perl
use strict;
use warnings;
use IO::Socket::INET;
use JSON::PP qw(encode_json decode_json);
use Time::HiRes qw(usleep);

use constant {
    ADDR => 0x80000000,
    SIZE => 520 * 1024,
    W    => 1024,
    H    => 520,
    FPS  => 30,
};

my $dump = "/tmp/perlos-ram.bin";
# ------------------------------------------------------------
# RAM visualization: byte values (0x00..0xFF) mapped to RGB.
#
# 00       black       Zero-filled memory
# 01..1f   green       ASCII control characters
# 09       light blue  TAB (horizontal tab)
# 0a,0d    blue        LF / CR (newline characters)
# 20       pink        ASCII space character
# 21..7f   red         Printable ASCII text (except DEL)
# 80..fe   cyan        Non-ASCII or binary data
# ff       white       All bits set (0xFF)
#
# Perl memory poisoning (debug patterns):
# ab       magenta     PoisonNew: newly allocated memory
# ef       yellow      PoisonFree: released memory
# ------------------------------------------------------------
my $r =
    "if(eq(val,9),173," .
    "if(eq(val,32),255," .
    "if(eq(val,171),255," . # AB PoisonNew
    "if(eq(val,239),255," . # EF PoisonFree
    "if(eq(val,255),255," .
    "if(lt(val,33),0," .
    "if(lt(val,128),val+128,0)))))))";

my $g =
    "if(eq(val,0),0," .
    "if(eq(val,9),216," .
    "if(eq(val,10)+eq(val,13),0," .
    "if(eq(val,32),192," .
    "if(eq(val,171),0," .
    "if(eq(val,239),255," .
    "if(eq(val,255),255," .
    "if(lt(val,32),val*4+128," .
    "if(lt(val,128),0,val)))))))))";

my $b =
    "if(eq(val,0),0," .
    "if(eq(val,9),230," .
    "if(eq(val,10)+eq(val,13),255," .
    "if(eq(val,32),203," .
    "if(eq(val,171),255," .
    "if(eq(val,239),0," .
    "if(eq(val,255),255," .
    "if(lt(val,128),0,val))))))))";

my $vf = "format=rgb24,lutrgb=r='$r':g='$g':b='$b'";

# ffplay
open my $video, "|-", "ffplay",
    "-loglevel", "warning",
    "-fflags", "nobuffer",
    "-f", "rawvideo",
    "-pixel_format", "gray",
    "-video_size", W . "x" . H,
    "-framerate", FPS,
    "-vf", $vf,
    "-window_title", "PerlOS RAM",
    "-"
    or die "ffplay: $!";
binmode $video;
$SIG{PIPE} = sub { exit };

# QMP
my $qmp;
for (1 .. 100) {
    $qmp = IO::Socket::INET->new(
        PeerAddr => "127.0.0.1", PeerPort => 4444, Proto => "tcp"
    );
    last if $qmp;
    usleep(100_000);
}
$qmp or die "QMP: unable to connect\n";
$qmp->autoflush(1);

# greeting
<$qmp>;

my $id = 0;
sub qmp {
    my ($command, $args) = @_;
    my $n = ++$id;
    my %req = (execute => $command, id => $n);
    $req{arguments} = $args if $args;
    print $qmp encode_json(\%req), "\r\n";
    while (my $line = <$qmp>) {
        my $res = decode_json($line);
        next unless defined $res->{id};
        next unless $res->{id} == $n;
        die "$res->{error}{desc}\n" if $res->{error};
        return $res->{return};
    }
    die "QMP disconnected\n";
}
qmp("qmp_capabilities");

# Main loop
while (1) {
    qmp("pmemsave", { val => ADDR, size => SIZE, filename => $dump });
    open my $fh, "<:raw", $dump or die "$dump: $!";
    my $buf = '';
    my $n = read $fh, $buf, SIZE;
    close $fh;
    die "short read: $n / " . SIZE . "\n"
        unless defined $n && $n == SIZE;
    print {$video} $buf or last;
}

1;