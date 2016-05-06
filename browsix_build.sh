#!/bin/bash
set -e

BIN=~/plasma/browsix/fs/usr/bin

export EMFLAGS='--memory-init-file 0 -s EMTERPRETIFY=1 -s EMTERPRETIFY_ASYNC=1 -s EMTERPRETIFY_BROWSIX=1 -O1'

make PLATFORM=js-pc-browsix EXT=.js CC="emcc $EMFLAGS"

# for f in bin/linux-x86_64/*.js; do
# 	cp "$f" $BIN/"`basename ${f%.*}`"
# done
