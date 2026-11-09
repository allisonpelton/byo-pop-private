#!/usr/bin/env python3
"""The coverage check: which of the original's functions a set of runs reaches, against what the port translates.

  python -I tools/coverage.py list <game> [-o FILE]
      writes the address list for origrun's --coverage (every function of the game's export but its thunks, one hex
      address a line) to FILE, or to build/coverage/<game>/functions.txt
  python -I tools/coverage.py report <game> <hits.txt>... [-o FILE]
      reads the hits files of origrun runs (each run writes <run>/coverage.txt) and the port's sources, and writes the
      report, in Markdown, to FILE or to stdout

A port function's counterpart is the address in the comment block just above its definition (the ports write
"// 0x4cb5bc" there); an address anywhere else in a comment is a mention. Only the game's own code is compared,
the range that games/<game>/coverage.txt names (the whole exe without one): the framework, the C runtime and the
libraries a game links are ported once for every game, not by the game's port. docs/COVERAGE.md has the method, the
settings file and how to read the report.
"""
import bisect, collections, datetime, glob, json, os, re, sys

REPO = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
ADDR = re.compile(r'\b0x([0-9a-fA-F]{5,8})\b')
FUNC_DEF = re.compile(r"^(?!\s)(?!(?:if|for|while|switch|return|else|do|case|default|namespace|class|struct|enum|"
                      r"union|typedef|using|template|public|private|protected|#)\b)"
                      r"[\w:<>,*&~ \[\]]*?\b((?:\w+::)*~?\w+|operator\s*\S+)\s*\(")
SOURCES = ('.cpp', '.c', '.h', '.hpp', '.inl', '.inc')
TEMPLATE_HEAD = re.compile(r'\s*template\s*<[^;{]*$')


def die(msg):
    sys.exit('coverage.py: ' + msg)


def settings(game):
    """games/<game>/coverage.txt: export <path>, src <dir> (more than one allowed), game <lo>-<hi> (more than one
    allowed), none <addr or lo-hi> <reason>, addrmap <tsv> <from column> <to column>. Paths are relative to the
    repository."""
    cfg = {'export': None, 'src': [], 'game': [], 'none': [], 'addrmap': None, 'unmapped': {}}
    path = os.path.join(REPO, 'games', game, 'coverage.txt')
    if os.path.isfile(path):
        for n, line in enumerate(open(path, encoding='utf-8'), 1):
            line = line.split('#', 1)[0].strip() if not line.lstrip().startswith('none') else line.strip()
            if not line or line.startswith('#'):
                continue
            key, _, rest = line.partition(' ')
            rest = rest.strip()
            if key == 'export':
                cfg['export'] = rest
            elif key == 'src':
                cfg['src'].append(rest)
            elif key == 'game':
                cfg['game'].append(span(rest, path, n))
            elif key == 'none':
                where, _, why = rest.partition(' ')
                if not why.strip():
                    die('%s:%d: a "none" line needs its reason' % (path, n))
                cfg['none'].append((span(where, path, n), why.strip()))
            elif key == 'addrmap':
                words = rest.split()
                if len(words) != 3 or not words[1].isdigit() or not words[2].isdigit():
                    die('%s:%d: addrmap <tsv> <from column> <to column>' % (path, n))
                cfg['addrmap'] = (words[0], int(words[1]), int(words[2]))
            else:
                die('%s:%d: unknown setting %r' % (path, n, key))
    if not cfg['export']:
        found = [p for d in ('game', 'gh', 'ghidra') for p in [os.path.join('re', game, d, 'functions.jsonl')]
                 if os.path.isfile(os.path.join(REPO, p))]
        if not found:
            die('no export for %s: give "export <path>" in games/%s/coverage.txt' % (game, game))
        cfg['export'] = found[0]
    if not cfg['src']:
        cfg['src'] = [os.path.join('games', game, 'src')]
    return cfg


def span(text, path, n):
    lo, _, hi = text.partition('-')
    try:
        lo = int(lo, 16)
        hi = int(hi, 16) if hi else lo
    except ValueError:
        die('%s:%d: %r is not an address or a range of them' % (path, n, text))
    return lo, hi


def load_export(cfg):
    fns = {}
    for line in open(os.path.join(REPO, cfg['export']), encoding='utf-8'):
        if line.strip():
            f = json.loads(line)
            if not f.get('thunk') and not f.get('external'):
                fns[int(f['addr'], 16)] = f
    return fns


def is_definition(lines, index):
    """A definition: its parameter list closes and a '{' (or ': init') follows before any ';'."""
    text = ''
    for k in range(index, min(index + 80, len(lines))):   # 80: a constructor's initializer list can be long
        text += lines[k] + '\n'
        brace, semi = text.find('{'), text.find(';')
        if brace >= 0 and (semi < 0 or brace < semi):
            head = text[:brace]
            return head.count('(') > 0 and '=' not in head.split(')')[-1].replace('==', '')
        if semi >= 0:
            return False
    return False


def split_line(line, in_block):
    """One line's code (comments and literals blanked) and its comment text, and whether a block comment is still
    open after it."""
    code, com, i, n = [], [], 0, len(line)
    while i < n:
        if in_block:
            j = line.find('*/', i)
            com.append(line[i:] if j < 0 else line[i:j])
            if j < 0:
                return ''.join(code), ' '.join(com), True
            i, in_block = j + 2, False
        elif line.startswith('//', i):
            com.append(line[i + 2:])
            break
        elif line.startswith('/*', i):
            i, in_block = i + 2, True
        elif line[i] in '"\'':
            q, i = line[i], i + 1
            while i < n and line[i] != q:
                i += 2 if line[i] == '\\' else 1
            i += 1
            code.append(q + q)
        else:
            code.append(line[i])
            i += 1
    return ''.join(code), ' '.join(com), in_block


# A function declared or defined, indented or not: a return type (or virtual, static ...) before the name, so that a
# call (no type before it) or an assignment is not one. Default arguments may follow ('=' after the '(').
DECL = re.compile(r"^\s*(?:(?:virtual|static|inline|explicit|friend|extern)\s+)*"
                  r"(?!(?:return|new|delete|throw|else|case|goto|if|while|for|switch|do|sizeof)\b)"
                  r"[\w:<>,*&]+(?:\s+[\w:<>,*&]+)*?\s*[\s*&]\s*((?:\w+::)*~?\w+|operator\s*\S+)\s*\(.*$")
# A constructor or destructor declared in its class (no return type): counted only when the name is the class's own
# and the line is at the class's own scope, so that a call statement in a function body is not one (Ning Po's spec,
# 2026-11-08).
CTOR = re.compile(r"^\s*(?:(?:virtual|explicit|inline)\s+)*(~?\w+)\s*\(.*$")
# The head of a class or struct (not a forward declaration): its name is the last word before ':' or '{'.
CLASS_HEAD = re.compile(r"^\s*(?:class|struct)\s+([^:{;()]*)")
# The end of a declaration: the parameter list closed, with no body; a pure virtual, deleted or defaulted function has
# no code of its own in the original.
DECL_END = re.compile(r"\)\s*(?:const\s*)?(?:override\s*)?;\s*$")


def declaration_end(codes, index):
    """The line where the declaration starting at index ends (the first with ';' or '{', up to 7 lines on), when it
    ends as a declaration does, else None."""
    for k in range(index, min(index + 8, len(codes))):
        if ';' in codes[k] or '{' in codes[k]:
            return k if DECL_END.search(codes[k]) else None
    return None


def load_addrmap(cfg):
    """The addrmap setting's table as {from address: to address}: a port whose comments carry another edition's addresses
    (Mahjong Escape: Ancient China's, which still has Ancient Japan's) is matched through it. Columns count from 1 and
    hold hex addresses with or without 0x; rows of data as well as functions belong in it, so that a global's address
    is not listed as unmapped."""
    if not cfg['addrmap']:
        return None
    tsv, fc, tc = cfg['addrmap']
    amap = {}
    for line in open(os.path.join(REPO, tsv), encoding='utf-8'):
        cols = line.rstrip('\r\n').split('\t')
        if len(cols) < max(fc, tc):
            continue
        try:
            amap[int(cols[fc - 1], 16)] = int(cols[tc - 1], 16)
        except ValueError:
            continue   # a header
    return amap


def port_sites(cfg, fns):
    """addr -> sites, where a site is (kind, path, line, name): kind 'def' for the comment block just above a function's
    definition or declaration (or the comment on its own line), 'inside' for an address within a function that is not
    its entry, 'mention' for any other comment."""
    starts = sorted(fns)
    amap = load_addrmap(cfg)

    def mapped(a, rel, line):
        if amap is None:
            return a
        if a in amap:
            return amap[a]
        cfg['unmapped'].setdefault(a, (rel, line))   # stays as it is, and the report lists it
        return a

    def owner(a):
        if a in fns:
            return a, True
        i = bisect.bisect_right(starts, a) - 1
        if i >= 0 and a < starts[i] + max(fns[starts[i]].get('size') or 1, 1):
            return starts[i], False
        return None, False

    sites = collections.defaultdict(list)
    for root in cfg['src']:
        for path in sorted(glob.glob(os.path.join(REPO, root, '**', '*'), recursive=True)):
            if not path.endswith(SOURCES) or not os.path.isfile(path):
                continue
            rel = os.path.relpath(path, REPO).replace('\\', '/')
            lines = open(path, encoding='utf-8', errors='replace').read().split('\n')
            codes, comments, in_block = [], [], False
            for line in lines:
                code, com, in_block = split_line(line, in_block)
                codes.append(code)
                comments.append(com)
            used = set()
            depth, classes, pending = 0, [], None   # classes: (name, the brace depth inside its body)
            for i, line in enumerate(lines):
                own = classes[-1][0] if classes and depth == classes[-1][1] else None
                defined = is_definition(lines, i)
                m = FUNC_DEF.match(line) if defined else None
                if not m:
                    d = DECL.match(codes[i])
                    if not d and own:
                        c = CTOR.match(codes[i])
                        d = c if c and c.group(1).lstrip('~') == own else None
                    if d and (defined or declaration_end(codes, i) is not None):
                        m = d
                code = codes[i]
                h = CLASS_HEAD.match(code)
                if h and not code.rstrip().endswith(';') and '(' not in code and h.group(1).split():
                    pending = h.group(1).split()[-1]
                for ch in code:
                    if ch == '{':
                        depth += 1
                        if pending:
                            classes.append((pending, depth))
                            pending = None
                    elif ch == '}':
                        if classes and depth == classes[-1][1]:
                            classes.pop()
                        depth -= 1
                if not m:
                    continue
                block = [i]   # the comment on the line itself counts too
                j = i - 1
                while j >= 0 and TEMPLATE_HEAD.match(codes[j]):   # a template's head is part of the declaration
                    block.append(j)
                    j -= 1
                while j >= 0 and not codes[j].strip() and comments[j].strip():
                    block.append(j)
                    j -= 1
                for k in block:
                    for h in ADDR.findall(comments[k]):
                        a, entry = owner(mapped(int(h, 16), rel, k + 1))
                        if a is not None:
                            sites[a].append(('def' if entry else 'inside', rel, i + 1, m.group(1)))
                            used.add((k, h))
            for k, c in enumerate(comments):
                for h in ADDR.findall(c):
                    if (k, h) in used:
                        continue
                    a, entry = owner(mapped(int(h, 16), rel, k + 1))
                    if a is not None:
                        sites[a].append(('mention' if entry else 'inside', rel, k + 1, ''))
    return sites


def read_hits(paths):
    hits, runs = {}, []
    for p in paths:
        n, ended, watched, code, stopped = 0, False, None, None, False
        for line in open(p, encoding='utf-8', errors='replace'):
            line = line.strip()
            if line.startswith('#'):
                ended = ended or 'ended normally' in line
                m = re.search(r'(\d+) functions watched', line)
                watched = int(m.group(1)) if m else watched
                m = re.search(r'exited with code 0x([0-9A-Fa-f]{8})', line)   # origrun's line, since 2026-11-09
                code = int(m.group(1), 16) if m else code
                stopped = stopped or 'stopped the game at its timeout' in line
                continue
            if not line:
                continue
            a = int(line.split()[0], 16)
            n += 1
            hits[a] = hits.get(a, 0) + 1
        if ended:
            how = 'the game exited'
        elif code is not None:
            how = 'it crashed (0x%08X)' % code if code >= 0xC0000000 else 'the game exited with code %d' % code
        elif stopped:
            how = 'origrun stopped it (a timeout)'
        else:   # a hits file from before origrun said how a run ended
            how = 'origrun stopped it (a timeout), or it crashed'
        runs.append((p, n, how, watched))
    return hits, runs


def made_elsewhere(f):
    """What a port gets from its own compiler, though the original's linker put it among the game's code: the C++
    library's templates and the compiler's deleting destructors. Its group's name, or ''."""
    full = name_of(f)
    if full.startswith('std::') or (f.get('ns') or '').startswith('std'):
        return 'the C++ library (std::)'
    if re.search(r'(?:scalar|vector)_deleting_destructor$', full):
        return "the compiler's deleting destructors"
    return ''


def in_spans(a, spans):
    return any(lo <= a <= hi for lo, hi in spans)


def name_of(f):
    return f.get('full') or f.get('name') or ''


def report(game, hit_paths, out):
    cfg = settings(game)
    fns = load_export(cfg)
    hits, runs = read_hits(hit_paths)
    sites = port_sites(cfg, fns)
    game_fns = [a for a in sorted(fns) if (not cfg['game'] or in_spans(a, cfg['game'])) and not made_elsewhere(fns[a])]
    gameset = set(game_fns)
    left_out = {a: why for a in game_fns for (s, why) in cfg['none'] if in_spans(a, [s])}

    def has_def(a):
        return any(k == 'def' for k, *_ in sites.get(a, []))

    def any_site(a):
        return bool(sites.get(a))

    reached = [a for a in game_fns if a in hits]
    with_def = [a for a in game_fns if has_def(a)]
    gaps = [a for a in reached if not has_def(a) and a not in left_out]
    unreached_defs = [a for a in with_def if a not in hits and a not in left_out]
    neither = [a for a in game_fns if a not in hits and not has_def(a) and a not in left_out]

    w = out.write
    w('# Coverage of %s\n\n' % game)
    w('Made by `tools/coverage.py report %s` on %s from %d run%s, against the export `%s` and the sources in %s.\n\n' % (
        game, datetime.date.today().isoformat(), len(runs), '' if len(runs) == 1 else 's', cfg['export'],
        ', '.join('`%s`' % s for s in cfg['src'])))
    w('| Run | Functions reached | How it ended |\n|---|---:|---|\n')
    for p, n, how, watched in runs:
        w('| `%s` | %d%s | %s |\n' % (os.path.relpath(p, REPO).replace('\\', '/'), n,
                                      ' of %d watched' % watched if watched else '', how))
    span_txt = ', '.join('%08x-%08x' % s for s in cfg['game']) if cfg['game'] else 'the whole exe'
    w('\nThe game\'s own code (%s) has %d functions. The runs reached %d of them, the port defines %d, and %d are '
      'left out on purpose (`games/%s/coverage.txt`).\n\n' % (span_txt, len(game_fns), len(reached), len(with_def),
                                                             len(left_out), game))

    starts = sorted(fns)

    def row(a, extra):
        f = fns[a]
        return '| %08x | %s | %d | %s |\n' % (a, name_of(f).replace('|', '\\|'), f.get('size') or 0, extra)

    def callers(a):
        names = []
        for c in fns[a].get('callers') or []:
            ca = int(c, 16)
            if ca in fns:
                names.append(name_of(fns[ca]))
            else:
                i = bisect.bisect_right(starts, ca) - 1
                if i >= 0:
                    names.append(name_of(fns[starts[i]]))
        names = sorted(set(names))
        return ', '.join(names[:3]) + (' and %d more' % (len(names) - 3) if len(names) > 3 else '')

    def where(a):
        s = sites.get(a, [])
        return ', '.join('%s:%d (%s)' % (p, ln, k) for k, p, ln, _ in s[:3]) + (' ...' if len(s) > 3 else '')

    w('## Reached in the original, with no definition in the port\n\n')
    w('Each is code the original ran that the port has no function for: translated inline somewhere (then the address '
      'belongs in the comment above the function that holds it), missing, or to be named in coverage.txt with its '
      'reason. Mentions in the port, if any, are listed.\n\n')
    if gaps:
        w('| Address | Name | Size | Called from | The port mentions it |\n|---|---|---:|---|---|\n')
        for a in gaps:
            w(row(a, '%s | %s' % (callers(a).replace('|', '\\|'), where(a) or '')))
    else:
        w('None.\n')
    w('\n## Defined in the port, never reached in these runs\n\n')
    w('No comparison has tested these: give each a run that reaches it, or read it again against the export. One that '
      'no play of the original can reach is a candidate for taking out.\n\n')
    if unreached_defs:
        w('| Address | Name | Size | Port definition |\n|---|---|---:|---|\n')
        for a in unreached_defs:
            w(row(a, where(a)))
    else:
        w('None.\n')
    w('\n## Never reached and not defined in the port\n\n')
    w('Code neither the runs nor the port touch: dead in the original, or reached only by input the runs never gave.\n\n')
    if neither:
        w('| Address | Name | Size | Called from |\n|---|---|---:|---|\n')
        for a in neither:
            w(row(a, callers(a).replace('|', '\\|')))
    else:
        w('None.\n')
    if cfg['addrmap']:
        un = cfg['unmapped']
        w('\n## Not in the address map\n\nThe port\'s addresses that `%s` has no row for, so they were matched as they '
          'are: code only this edition has, or functions of the other edition that this one lacks.\n\n' % cfg['addrmap'][0])
        if un:
            w('| Address | First at |\n|---|---|\n')
            for a in sorted(un):
                w('| %08x | %s:%d |\n' % (a, un[a][0], un[a][1]))
        else:
            w('None.\n')
    if left_out:
        w('\n## Left out on purpose\n\n| Address | Name | Size | Reached | Reason |\n|---|---|---:|---|---|\n')
        for a in sorted(left_out):
            w(row(a, '%s | %s' % ('yes' if a in hits else 'no', left_out[a])))
    others = [a for a in sorted(fns) if a not in gameset]
    if others:
        w('\n## Outside the game\'s own code\n\nNot compared (the framework, the C runtime and libraries); for the record, '
          'what the runs reached of each group:\n\n| Group | Functions | Reached |\n|---|---:|---:|\n')
        groups = collections.defaultdict(lambda: [0, 0])
        for a in others:
            f = fns[a]
            ns = (f.get('ns') or 'Global').split('::')[0]
            g = made_elsewhere(f) or ('unnamed (FUN_)' if (f.get('name') or '').startswith('FUN_') else ns)
            groups[g][0] += 1
            groups[g][1] += a in hits
        for g, (n, r) in sorted(groups.items(), key=lambda kv: -kv[1][0]):
            w('| %s | %d | %d |\n' % (g, n, r))
    stray = sorted(a for a in hits if a not in fns)
    if stray:
        w('\n%d hit addresses are not functions of this export (another version of the exe?): %s\n' % (
            len(stray), ', '.join('%08x' % a for a in stray[:10])))
    return len(gaps)


def main(argv):
    if len(argv) < 2 or argv[0] not in ('list', 'report'):
        sys.exit(__doc__.strip())
    out_path = None
    if '-o' in argv:
        i = argv.index('-o')
        out_path = argv[i + 1]
        argv = argv[:i] + argv[i + 2:]
    cmd, game = argv[0], argv[1]
    if cmd == 'list':
        cfg = settings(game)
        fns = load_export(cfg)
        out_path = out_path or os.path.join(REPO, 'build', 'coverage', game, 'functions.txt')
        os.makedirs(os.path.dirname(os.path.abspath(out_path)), exist_ok=True)
        with open(out_path, 'w', encoding='ascii', newline='\n') as f:
            f.write('# %s: every function of %s but its thunks (tools/coverage.py list)\n' % (game, cfg['export']))
            for a in sorted(fns):
                f.write('%08x\n' % a)
        print('%d functions -> %s' % (len(fns), out_path))
        return 0
    if len(argv) < 3:
        die('report needs at least one hits file')
    if out_path:
        os.makedirs(os.path.dirname(os.path.abspath(out_path)), exist_ok=True)
        with open(out_path, 'w', encoding='utf-8', newline='\n') as f:
            gaps = report(game, argv[2:], f)
        print('%d reached functions with no definition in the port -> %s' % (gaps, out_path))
    else:
        report(game, argv[2:], sys.stdout)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
