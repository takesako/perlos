use Benchmark qw(timethis cmpthese);

Benchmark::disablecache();
Benchmark::clearallcache();

my $start = gettick();
my $x = sqrt($x + 1) for 1 .. 100000;
printf "100000 x sqrt(): %u ms\n", gettick_diff($start);

$x = 1;
timethis(-1, sub { $x = sqrt($x + 1) });
cmpthese(-1, {
    add  => sub { $x += 1 },
    sqrt => sub { $x = sqrt($x + 1) },
});
return;