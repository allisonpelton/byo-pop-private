#!/usr/bin/env python3
"""Lists the code-mod hooks in the translated game code (docs/CODE_MODS_DESIGN.md 1.2, principle 1).

Every line the code mods edit inside a translated function carries a "// port(variant):" tag. This prints
those lines grouped by function, with the function's original address, so the whole intrusion into the
decompiled code can be reviewed against the decompilation:

  tools/list_variant_hooks.py            the listing and the counts
  tools/list_variant_hooks.py --count    only the number of hook lines in translated functions

A translated function is a definition in src/game whose comment block names its address ("// 0x4cb5bc",
or "// Body of 0x4ccdfe" for a port helper split out of one). Tagged lines elsewhere in those files
(declarations, port-only helpers) are listed separately, and files that are variant code from their first
comment ("// port(variant): ...", e.g. GameRules.cpp) only by name: they are new code, not hooks.
"""

import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
SOURCES = ["src/game"]
TAG = "port(variant)"
DEFINITION = re.compile(r"^(?!\s)(?!//)(?!#)(?:[\w:<>*&,\s]+?\s+)?[*&]?(~?[\w]+::~?\w+)\s*\(")
ADDRESS = re.compile(r"0x[0-9a-fA-F]{6}")


def first_comment(lines):
    """The first comment line (after an include guard, for a header)"""
    for line in lines[:12]:
        if line.startswith("//"):
            return line
    return ""


def scan(path):
    """(variant_file, [(function, address, [(line number, text)])], [(line number, text) outside])"""
    with open(path, encoding="utf-8", errors="replace") as f:
        lines = f.read().split("\n")
    if first_comment(lines).startswith("// " + TAG):
        return True, [], []
    functions = []
    outside = []
    i = 0
    while i < len(lines):
        m = DEFINITION.match(lines[i]) if path.endswith(".cpp") else None
        if m and not lines[i].rstrip().endswith(";"):
            # the comment block right above the definition
            j = i - 1
            comments = []
            while j >= 0 and lines[j].startswith("//"):
                comments.insert(0, lines[j])
                j -= 1
            address = None
            for c in comments:
                a = ADDRESS.search(c)
                if a:
                    address = a.group(0)
                    break
            end = i
            while end < len(lines) and lines[end] != "}":
                end += 1
            tagged = [(k + 1, lines[k].strip()) for k in range(i, min(end + 1, len(lines))) if TAG in lines[k]]
            if address:
                functions.append((m.group(1), address, tagged))
            else:
                outside.extend(tagged)
            i = end + 1
            continue
        if TAG in lines[i]:
            outside.append((i + 1, lines[i].strip()))
        i += 1
    return False, functions, outside


def main(argv):
    count_only = "--count" in argv
    variant_files = []
    report = []
    hooks = 0
    hooked_functions = 0
    other = 0
    for folder in SOURCES:
        for name in sorted(os.listdir(os.path.join(ROOT, folder))):
            if not name.endswith((".cpp", ".h")):
                continue
            rel = folder + "/" + name
            is_variant, functions, outside = scan(os.path.join(ROOT, rel))
            if is_variant:
                variant_files.append(rel)
                continue
            hooked = [f for f in functions if f[2]]
            if not hooked and not outside:
                continue
            report.append(rel)
            for function, address, tagged in hooked:
                hooks += len(tagged)
                hooked_functions += 1
                report.append("  %s  %s  (%d)" % (function, address, len(tagged)))
                for number, text in tagged:
                    report.append("    %5d  %s" % (number, text))
            if outside:
                other += len(outside)
                report.append("  outside translated functions (declarations, port code):")
                for number, text in outside:
                    report.append("    %5d  %s" % (number, text))

    if count_only:
        print(hooks)
        return 0
    print("Variant hooks: tagged lines in translated functions (docs/CODE_MODS_DESIGN.md 1.2)")
    print()
    for line in report:
        print(line)
    print()
    print("Variant-only files (new code, not hooks): " + (", ".join(variant_files) if variant_files else "none"))
    print("%d hook lines in %d translated functions; %d other tagged lines" % (hooks, hooked_functions, other))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
