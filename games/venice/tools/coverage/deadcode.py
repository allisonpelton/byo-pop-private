#!/usr/bin/env python3
"""Sorts the game's functions that no coverage run reached and the port does not define (the report's third list) by who
calls them in the export: a function with a caller that was reached is a branch of reached code (a run should look at it),
the others are dead clusters (every caller is itself never reached, or nothing calls it at all) grouped under the root
that starts each cluster.

    python games/venice/tools/coverage/deadcode.py [-v]       # reads build/coverage/venice/hits/*.txt and report.md's lists
    python games/venice/tools/coverage/deadcode.py --none     # the `none` lines for the dead clusters, for coverage.txt
"""
import collections
import glob
import json
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))
COVERAGE_DIR = os.path.join(ROOT, 'build', 'coverage', 'venice')


def load_reached():
    reached = set()
    for path in glob.glob(os.path.join(COVERAGE_DIR, 'hits', '*.txt')):
        for line in open(path):
            if not line.startswith('#') and line.strip():
                reached.add(line.split()[0])
    return reached


def load_report_section(title):
    """The addresses in the table under the heading `title` of report.md."""
    rows = []
    inside = False
    for line in open(os.path.join(COVERAGE_DIR, 'report.md'), encoding='utf-8'):
        if line.startswith('## '):
            inside = line.strip() == '## ' + title
        elif inside:
            found = re.match(r'\| ([0-9a-f]{8}) \|', line)
            if found:
                rows.append(found.group(1))
    return rows


LIBRARY_RANGE = (0x43a580, 0x43f800)  # the C++ library's stream, locale and string instantiations
EDITOR_RANGE = (0x42dbf0, 0x434b70)  # the level editor's classes and everything only they use
EDITOR_TAG_WRITERS = ('00439ec0', '00439e80')  # what the editor's save functions write an object's tag with


def reason_of_cluster(root, functions):
    """Why a dead cluster is left out, from where it lies and what it calls."""
    entry = functions[root]
    address = int(root, 16)
    if LIBRARY_RANGE[0] <= address < LIBRARY_RANGE[1]:
        return 'C++ library: an instantiation of the stream, locale or string classes that no run reaches; the port uses the standard library'
    if EDITOR_RANGE[0] <= address < EDITOR_RANGE[1] or address in (0x441800, 0x441a40, 0x441c20):
        return 'level editor: its classes and what only they use; nothing in play creates the editor (no caller of its constructor)'
    if address < 0x434d90 and address >= 0x434b70:
        return 'level editor: the destructors of its MFC frame window class, which the linker kept'
    editor_texts = ('Change Win Message', 'Clear Slots', 'Make Slot 0', 'make_slot_', 'add_slot_')
    if (any(c in EDITOR_TAG_WRITERS for c in entry['callees']) or any(text in entry['strings'] for text in editor_texts)
            or any(text.startswith('<') for text in entry['strings'])):
        return 'level editor: writes an object, a stage, an attribute, a filter or an event as a tag, or edits a level; only the editor calls it'
    if not entry['callers'] and entry['size'] <= 13:
        return 'one-line function (empty, a setter or a getter) that nothing in play calls'
    if not entry['callers']:
        return 'no caller in the export and no run reached it: a helper, a destructor or an editor function that nothing in play calls'
    return 'only functions that no run reaches call it'


def main():
    verbose = '-v' in sys.argv
    functions = {}
    for line in open(os.path.join(ROOT, 're', 'venice', 'gh', 'functions.jsonl')):
        entry = json.loads(line)
        functions[entry['addr']] = entry
    reached = load_reached()
    unreached = load_report_section('Never reached and not defined in the port')
    unreached_set = set(unreached)
    branches = []
    roots = collections.OrderedDict()
    for address in unreached:
        callers = functions[address]['callers']
        live = [c for c in callers if c in reached]
        if live:
            branches.append((address, live))
        elif not [c for c in callers if c in unreached_set and c != address]:
            roots[address] = []
    # every other function belongs to the cluster of a root that reaches it through the dead functions' calls
    owner = {}
    for root in roots:
        stack = [root]
        while stack:
            address = stack.pop()
            if address in owner:
                continue
            owner[address] = root
            stack.extend(c for c in functions[address]['callees'] if c in unreached_set)
    for address in unreached:
        if address in owner and address not in roots:
            roots[owner[address]].append(address)
    leftover = [a for a in unreached if a not in owner and a not in [b[0] for b in branches]]
    if '--none' in sys.argv:
        for root, members in roots.items():
            reason = reason_of_cluster(root, functions)
            for address in [root] + members:
                print('none %s %s' % (address, reason))
        for address in leftover:
            print('none %s %s' % (address, reason_of_cluster(address, functions)))
        return 0
    print('%d functions never reached and not defined in the port' % len(unreached))
    print('%d with a caller that a run reached (to look at):' % len(branches))
    for address, live in branches:
        print('  %s size %d called from %s' % (address, functions[address]['size'], ', '.join(live[:4])))
    print('%d dead clusters (a root that nothing reached calls, and what only it and its cluster call):' % len(roots))
    for root, members in roots.items():
        size = functions[root]['size'] + sum(functions[m]['size'] for m in members)
        print('  %s %s: %d functions, %d bytes' % (root, functions[root]['name'], 1 + len(members), size))
        if verbose:
            print('     ' + ' '.join(members))
    if leftover:
        print('%d in no cluster: %s' % (len(leftover), ' '.join(leftover)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
