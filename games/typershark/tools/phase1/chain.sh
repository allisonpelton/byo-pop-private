#!/usr/bin/env bash
# Run a jobs file with queue.sh once an earlier job has its verdict:  chain.sh <tag to wait for> <jobs file>
# (waits for "compared" in build/ts-p1/<tag>.out; the earlier queue and this one would share the original's copy otherwise)
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
until grep -q "compared" "$ROOT/build/ts-p1/$1.out" 2>/dev/null; do sleep 5; done
exec bash "$ROOT/games/typershark/tools/phase1/queue.sh" "$2"
