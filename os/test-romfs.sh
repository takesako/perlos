#!/bin/sh
set -eu
cd "$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)"
# Do not mix m33os FILE/headers with host glibc.
exec python3 scripts/test-host.py --only romfs romfs-validation
