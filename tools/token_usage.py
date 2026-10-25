#!/usr/bin/env python3
"""Token use of Claude Code sessions, from their transcripts (the .jsonl files under ~/.claude/projects).

  python tools/token_usage.py <transcript.jsonl or folder> [...]

A folder is searched recursively, so a session's subagent and workflow transcripts are counted with it.
For each argument, prints the API calls and the tokens by kind: fresh input, cache writes, cache reads
and output. Plan limits weigh these differently, and cache reads are the cheapest kind, so the kinds are
kept apart. Each API response is counted once, even when the transcript repeats it.
"""

import json
import os
import sys

KINDS = ("input_tokens", "cache_creation_input_tokens", "cache_read_input_tokens", "output_tokens")


def files(path):
    if os.path.isfile(path):
        yield path
        return
    for root, _, names in os.walk(path):
        for name in names:
            if name.endswith(".jsonl"):
                yield os.path.join(root, name)


def usage(path):
    totals = dict.fromkeys(KINDS, 0)
    calls = 0
    seen = set()
    models = {}
    for f in files(path):
        with open(f, encoding="utf-8", errors="replace") as fh:
            for line in fh:
                try:
                    rec = json.loads(line)
                except ValueError:
                    continue
                msg = rec.get("message") if isinstance(rec, dict) else None
                if not isinstance(msg, dict) or msg.get("role") != "assistant" or not isinstance(msg.get("usage"), dict):
                    continue
                key = msg.get("id") or rec.get("requestId") or rec.get("uuid")
                if key in seen:
                    continue
                seen.add(key)
                calls += 1
                model = msg.get("model", "?")
                models[model] = models.get(model, 0) + 1
                for k in KINDS:
                    totals[k] += int(msg["usage"].get(k) or 0)
    return calls, totals, models


def main(argv):
    if not argv:
        print(__doc__.strip())
        return 2
    for path in argv:
        calls, t, models = usage(path)
        m = ", ".join("%s %d" % (k, v) for k, v in sorted(models.items(), key=lambda kv: -kv[1]))
        print("%s: %d calls; input %s, cache writes %s, cache reads %s, output %s (%s)" % (
            path, calls, format(t["input_tokens"], ","), format(t["cache_creation_input_tokens"], ","),
            format(t["cache_read_input_tokens"], ","), format(t["output_tokens"], ","), m))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
