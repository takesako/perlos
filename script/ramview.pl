#!/usr/bin/perl
use strict;
use warnings;
use IO::Socket::INET;
use JSON::PP qw(encode_json decode_json);

use constant {
    ADDR => 0x80000000, SIZE => 520 * 1024,
    W => 1024, H => 520, FPS => 30,
};

my $dump = "/dev/shm/perlos-ram.bin";

# 00       black
# 09       lightblue
# 0A, 0D   blue
# 01..1F   green
# 20       pink
# 21..7F   red
# 80..FE   cyan
# AB       magenta (PoisonNew)
# EF       yellow  (PoisonFree)
# FF       white

my $r = "if(eq(val,9),173," .
        "if(eq(val,32),255,if(eq(val,171),255,if(eq(val,239),255," .
        "if(eq(val,255),255,if(lt(val,33),0,if(lt(val,128),val+128,0)))))))";
my $g = "if(eq(val,0),0,if(eq(val,9),216,if(eq(val,10)+eq(val,13),0," .
        "if(eq(val,32),192,if(eq(val,171),0,if(eq(val,239),255," .
        "if(eq(val,255),255,if(lt(val,32),val*4+128,if(lt(val,128),0,val)))))))))";
my $b = "if(eq(val,0),0,if(eq(val,9),230,if(eq(val,10)+eq(val,13),255," .
        "if(eq(val,32),203,if(eq(val,171),255,if(eq(val,239),0," .
        "if(eq(val,255),255,if(lt(val,128),0,val))))))))";
my $vf = "format=rgb24,lutrgb=r='$r':g='$g':b='$b'";

open my $video, "|-", "ffplay",
    "-loglevel", "warning", "-fflags", "nobuffer",
    "-f", "rawvideo", "-pixel_format", "gray",
    "-video_size", W . "x" . H, "-framerate", FPS,
    "-vf", $vf, "-window_title", "PerlOS RAM", "-"
    or die "ffplay: $!";
binmode $video;
$SIG{PIPE} = sub { exit };

my $qmp = IO::Socket::INET->new(
    PeerAddr => "127.0.0.1", PeerPort => 4444, Proto => "tcp"
) or die "QMP: $!";
$qmp->autoflush(1);
<$qmp>;  # greeting

my $id = 0;
sub qmp {
    my ($command, $args) = @_;
    my %req = (execute => $command, id => ++$id);
    $req{arguments} = $args if $args;
    print $qmp encode_json(\%req), "\r\n";
    while (my $line = <$qmp>) {
        my $res = decode_json($line);
        next unless defined $res->{id} && $res->{id} == $id;
        die "$res->{error}{desc}\n" if $res->{error};
        return $res->{return};
    }
    die "QMP disconnected\n";
}

qmp("qmp_capabilities");

my $prev;

while (1) {
    qmp("pmemsave", { val => ADDR, size => SIZE, filename => $dump });
    open my $fh, "<:raw", $dump or die "$dump: $!";
    read $fh, my $buf, SIZE;
    close $fh;
    next if defined $prev && $buf eq $prev;
    print {$video} $buf or last;
    $prev = $buf;
}
1;