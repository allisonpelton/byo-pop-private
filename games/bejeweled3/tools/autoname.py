#!/usr/bin/env python3
"""port: names for Bejeweled3.exe's functions from properties/default.xml.

ResourceGen's AUTOGEN pass wrote each of the game's texts into default.xml with the `Class::Method:line` that uses it
in a comment. The exe keeps the same texts as its built-in defaults, so the function that refers to a text is that
method, or a caller it was inlined into. A function whose texts all name one method takes that name; one whose texts
name several is listed as a conflict for reading.

    python games/bejeweled3/tools/autoname.py [--out re/bejeweled3/names/10_autogen.jsonl]

The output is derived from the user's game files, so it stays in the git-ignored re/ folder.
"""
import argparse
import bisect
import collections
import html
import json
import os
import re

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..'))
GH = os.path.join(ROOT, 're', 'bejeweled3', 'gh2')
if not os.path.isfile(os.path.join(GH, 'functions.jsonl')):
    GH = os.path.join(ROOT, 're', 'bejeweled3', 'gh')
DEFAULT_XML = os.path.join(ROOT, 'assets', 'bejeweled3', 'properties', 'default.xml')
ENTRY = re.compile(r'<String id="(\d+)">(.*?)</String><!-- ([A-Za-z_]\w*::[A-Za-z_~]\w*):\d+ -->')


def decode(text):
    text = text.replace('&cr;', '\n').replace('&nbsp;', ' ')
    return html.unescape(text)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--out', default=os.path.join(ROOT, 're', 'bejeweled3', 'names', '10_autogen.jsonl'))
    args = ap.parse_args()

    methods_by_text = collections.defaultdict(set)
    for line in open(DEFAULT_XML, encoding='utf-8'):
        for m in ENTRY.finditer(line):
            methods_by_text[decode(m.group(2))].add(m.group(3))

    starts = []
    for line in open(os.path.join(GH, 'functions.jsonl'), encoding='utf-8'):
        starts.append(int(json.loads(line)['addr'], 16))
    starts.sort()

    votes = collections.defaultdict(collections.Counter)
    unmatched = 0
    for line in open(os.path.join(GH, 'strings.jsonl'), encoding='utf-8'):
        j = json.loads(line)
        candidates = methods_by_text.get(j.get('value', ''))
        # a text several methods use, or one used in many places (a mode's name), names nothing
        if not candidates or len(candidates) > 1 or len(j.get('xrefs', [])) > 3:
            continue
        # a single short word ("Sound", "Font") is also a framework's word
        if ' ' not in j['value'].strip() and len(j['value']) < 10:
            continue
        for x in j.get('xrefs', []):
            if not re.fullmatch(r'[0-9a-fA-F]+', x):
                continue
            i = bisect.bisect_right(starts, int(x, 16)) - 1
            if i < 0:
                continue
            for method in candidates:
                votes['%08x' % starts[i]][method] += 1
    named = conflicts = 0
    os.makedirs(os.path.dirname(args.out), exist_ok=True)
    with open(args.out, 'w', encoding='utf-8') as out:
        out.write('// names from default.xml\'s AUTOGEN comments (games/bejeweled3/tools/autoname.py)\n')
        for addr, counter in sorted(votes.items()):
            if len(counter) == 1:
                out.write(json.dumps({'addr': addr, 'name': next(iter(counter))}) + '\n')
                named += 1
            else:
                out.write('// conflict %s: %s\n' % (addr, ', '.join('%s x%d' % kv for kv in counter.most_common())))
                conflicts += 1
    print('texts %d, functions named %d, conflicts %d -> %s' % (len(methods_by_text), named, conflicts, args.out))


if __name__ == '__main__':
    main()
