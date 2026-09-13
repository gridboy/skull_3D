#!/bin/sh
set -eu
cd "$(dirname "$0")"
make
exec ./bin/skull "$@"
