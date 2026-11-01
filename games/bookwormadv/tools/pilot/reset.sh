#!/bin/bash
# reset.sh <port user dir>: the port's start state, a fresh copy of portuser0 (the original's comes in with its
# sandbox run, run.sh).
P=$(cd "$(dirname "$0")/../../../.." && pwd)/build/scratch/pilot
rm -rf "${1:?}"
cp -r "$P/portuser0" "$1"
