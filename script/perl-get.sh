#!/bin/sh
set -eu

V=5.12.5
URL="https://www.cpan.org/src/5.0/perl-$V.tar.gz"
DEST="perl-$V"
TMP="${TMPDIR:-/tmp}/perl-$V-$$"

trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP" "$DEST"

cat > "$TMP/files" <<'EOF'
Artistic
cv.h
EXTERN.h
hv.h
mg.c
op.h
patchlevel.h
perliol.h
pp.c
pp_sys.c
regcomp.c
scope.h
time64.h
utf8.h
av.c
deb.c
form.h
INTERN.h
mg.h
opnames.h
perlapi.c
perlsdio.h
pp_ctl.c
proto.h
regcomp.h
sv.c
toke.c
util.c
av.h
doio.c
globals.c
intrpvar.h
mro.c
overload.c
perlapi.h
perlvars.h
pp.h
README
regexec.c
sv.h
uconfig.h
util.h
doop.c
gv.c
iperlsys.h
mydtrace.h
overload.h
perl.c
perly.act
pp_hot.c
README.micro
regexp.h
taint.c
uconfig.sh
config_h.SH
dump.c
gv.h
keywords.h
numeric.c
pad.c
perl.h
perly.c
pp_pack.c
reentr.c
regnodes.h
thread.h
universal.c
warnings.h
cop.h
embed.h
handy.h
locale.c
op.c
pad.h
perlio.c
perly.h
perly.tab
pp_proto.h
reentr.h
run.c
time64.c
unixish.h
XSUB.h
Copying
embedvar.h
hv.c
Makefile.micro
opcode.h
parser.h
perlio.h
pp_sort.c
regcharclass.h
scope.c
time64_config.h
utf8.c
EOF

echo "Downloading Perl $V..."
curl -fL "$URL" -o "$TMP/perl.tar.gz"

sed "s|^|perl-$V/|" "$TMP/files" > "$TMP/tar-files"

tar -xzf "$TMP/perl.tar.gz" \
    -C "$DEST" \
    --strip-components=1 \
    -T "$TMP/tar-files"

echo "Extracted $(wc -l < "$TMP/files") files to $DEST/"
