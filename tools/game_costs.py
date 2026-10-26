#!/usr/bin/env python3
"""The API cost of the work on each game, split into port, mod and other work, for README.md's progress table.

  python -I tools/game_costs.py scan        reads every byo-pop transcript (and those from before the folder was
                                            renamed from wasm-bejeweled), cuts the sessions into pieces at their
                                            commits and writes build/costs/pieces.json, plus build/costs/todo<N>.txt
                                            with the pieces that still need a classification
  python -I tools/game_costs.py rules       prints the instructions for the agents that classify the todo files
  python -I tools/game_costs.py classify <result.json>...
                                            adds their classifications ({"items": [{"id", "game", "kind"}]}) to
                                            build/costs/classes.json
  python -I tools/game_costs.py table       prints each game's cost: port, mods, other, total

Calls are priced at docs/TOKEN_ECONOMY.md's API prices, each model at its own; each API response is counted once,
even when a transcript repeats it, and a session's subagents and workflow agents count with it. A piece is the work
between two commits (or merges) of one transcript, with the agents' calls placed by their time. Pieces of a game's own
session count for that game: when the game has no mods yet, they are port work without a classification. The rest
(the coordinator's pieces, and the sessions of games with mods) are classified by agents from the instruction, the
commits, the agents started and the files edited. The first session (before COORD_FROM) ported Bejeweled itself.
classes.json is keyed by transcript and piece number, so a later scan classifies only new pieces; a wrong one is
fixed by editing its entry there.
"""
import bisect, collections, json, os, re, sys

ROOT = os.path.expanduser('~/.claude/projects')
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'build', 'costs')
PRICES = {  # per million tokens: input, cache read, cache write 5 minutes, cache write 1 hour, output
    'claude-opus-5-5': (4, .20, 5, 8, 20),
    'claude-sonnet-5-5': (2, .20, 2.5, 4, 10),
    'claude-haiku-5-5': (1, .10, 1.25, 2, 5),
}
COORD_FROM = '2026-10-20T04:55'  # "Start the Bejeweled session": from here on the first session is the coordinator
# Session titles of single-game sessions (the app's titles); the coordinator's start with "byo-pop coordinator".
TITLES = {
    'Bejeweled Deluxe': 'bejeweled', 'Bejeweled Deluxe: mod menu M5, then code mods 6+': 'bejeweled',
    'Big Money Deluxe': 'bigmoney', 'Decompile and port Big Money Deluxe': 'bigmoney',
    'BookWorm Deluxe': 'bookworm', 'Decompile and port BookWorm Deluxe': 'bookworm',
    'Bookworm Adventures word finder page': 'bookworm',
    "Noah's Ark Deluxe": 'noahsark', 'Seven Seas Deluxe': 'sevenseas', 'Atomica Deluxe': 'atomica',
    'Mummy Maze Deluxe': 'mummymaze', 'TipTop Deluxe': 'tiptop', 'Ning Po Mahjong Deluxe': 'ningpo',
    'Pixelus Deluxe': 'pixelus', 'Alchemy Deluxe': 'alchemy', 'Bookworm Adventures Deluxe': 'bookwormadv',
    'Word Harmony': 'wordharmony', 'Cosmic Bugs': 'cosmicbugs', 'Rocket Mania Deluxe': 'rocketmania',
    'Water Bugs': 'waterbugs', 'Bookworm Adventures Volume 2': 'bookwormadv2', 'Dynomite Deluxe': 'dynomite',
    "Bonnie's Bookstore Deluxe": 'bonnies', 'Venice Deluxe': 'venice', 'Heavy Weapon Deluxe': 'heavyweapon',
    'Hammer Heads Deluxe': 'hammerheads', 'Bejeweled 2 Deluxe': 'bejeweled2', 'AstroPop Deluxe': 'astropop',
    'Mahjong Escape: Ancient Japan': 'mahjongjapan', 'Mahjong Escape: Ancient China': 'mahjongchina',
}
RULES = """You are classifying pieces of work from Claude Code sessions on byo-pop, a repository that decompiles PopCap games, ports them to native and WebAssembly builds on a shared framework, and makes mods for them. Each piece ("#<id> [<group>] <time> $<cost>") is the work between two commits: the instruction in force (USER = the human, PEER <name> = another Claude session), the commits or merges that ended it, the agents it started and the files it edited.

For every piece in the file, give:
- game: the game the work was for: one of the games/ folder names (bejeweled, bigmoney, bookworm, noahsark, sevenseas, atomica, mummymaze, tiptop, ningpo, pixelus, alchemy, bookwormadv, wordharmony, cosmicbugs, ...) or the session's own game slug, or shared (work for several games or for the repository as a whole).
  - A piece whose group is a game is that game's own session: use that game.
  - Pieces in [coord] are the coordinator. Before 2026-10-20T04:55 that session was itself porting Bejeweled: its framework, platform, oracle, replay and web-player work then was Bejeweled's (game bejeweled), and so were its code-mods stages and the Bejeweled 2-style gem and audio mods, unless the piece is clearly about another game or about setting up the repository and the sessions (shared). After that time, a review, audit, merge or fix of one game's work goes to that game (the PEER name, the merge subject or games/<folder> in the edits tell which); shared framework, tools, docs and rules go to shared.
- kind:
  - decomp: reverse engineering and porting the original game natively and to the web, fixing the port to match the original, the game's tests and oracle checks of fidelity, the framework/platform/web player/build support that ports need, notes about the original, reviews and merges of port work, the survey of which games to port, setting up game sessions.
  - mods: anything beyond the original game: mods and their mod.json, the mod menu and mod settings, rules and tunables, code mods (Bejeweled's code-mods stages, curated and example mods, board variants, new gems and modes), replays, LiveSplit and autosplitters, solvers, undo, daily boards, word journals, widescreen, HD upscales and generated art, extra word lists, the mod policy and mod roadmap, and the tests, reviews and merges of that work.
  - other: neither: publishing snapshots, the website and launch plan, licences, terms and legal questions, token and usage measurement, repository housekeeping, rules about how sessions work, memory notes.
Judge by the commits and edits first, then the instruction. When a piece mixes kinds, choose the one that most of its work served.
Return one item for every piece in the file, with its id number."""
KINDS = ('decomp', 'mods', 'other')


def call_cost(m, r, seen):
    """The call's price, or None when it is no API call or was counted already."""
    u, mod = m.get('usage'), m.get('model')
    if not u or mod not in PRICES:
        return None
    k = (m.get('id'), r.get('requestId'))
    if k in seen:
        return None
    seen.add(k)
    cc = u.get('cache_creation') or {}
    w5, w1 = cc.get('ephemeral_5m_input_tokens', 0), cc.get('ephemeral_1h_input_tokens', 0)
    w5 += max(0, u.get('cache_creation_input_tokens', 0) - w5 - w1)
    p = PRICES[mod]
    return (u.get('input_tokens', 0) * p[0] + u.get('cache_read_input_tokens', 0) * p[1] + w5 * p[2] + w1 * p[3]
            + u.get('output_tokens', 0) * p[4]) / 1e6


def records(path):
    for line in open(path, encoding='utf-8', errors='replace'):
        try:
            yield json.loads(line)
        except ValueError:
            pass


def text_of(c):
    if isinstance(c, list):
        return ' '.join(b.get('text', '') for b in c if isinstance(b, dict) and b.get('type') == 'text')
    return c if isinstance(c, str) else ''


def instruction(r):
    """The text of a message from the user or another session, or None."""
    if r.get('type') != 'user' or r.get('isMeta') or r.get('toolUseResult') is not None:
        return None
    c = (r.get('message') or {}).get('content')
    if isinstance(c, list) and any(isinstance(b, dict) and b.get('type') == 'tool_result' for b in c):
        return None
    t = text_of(c).strip()
    if not t or t.startswith(('This session is being continued', '[Request interrupted')):
        return None
    if 'cross-session-message' in t[:200] or t.startswith('Another Claude session'):
        nm = re.search(r'name="([^"]+)"', t)
        body = re.sub(r'^Another Claude session sent a message:\s*', '', re.sub(r'<[^>]+>', ' ', t)).strip()
        return 'PEER ' + (nm.group(1) if nm else '?') + ': ' + body
    return None if t.startswith('<') else 'USER: ' + t


def commits_in(cmd):
    """(is_merge, subject) of each commit or merge a shell command makes."""
    for mm in re.finditer(r'git(?: +-C +\S+)? +(commit|merge)\b([^\n]*)\n?([^\n]*)', cmd):
        kind, rest, nxt = mm.groups()
        if kind == 'merge' and not re.search(r'--no-ff|-m |--no-edit', rest):
            continue
        sm = re.search(r'-m\s+["\']([^"\'\n]{3,160})', rest)
        if sm:
            subj = sm.group(1)
        elif re.search(r"-m\s+@['\"]\s*$|-F\s+-\s+<<|<<\s*'?EOF", rest):
            subj = nxt[:160]
        else:
            subj = '(merge commit)' if '--no-edit' in rest else '(message from a file)'
        yield kind == 'merge' or subj.startswith('Merge'), subj


def short_path(p):
    p = p.replace('\\', '/')
    m = re.search(r'/((?:games|framework|platform|web|tools|cmake|docs|third_party)/.*)$', p)
    return m.group(1) if m else p.split('/')[-1]


def scan():
    folders = sorted(d for d in os.listdir(ROOT) if 'byo-pop' in d or 'wasm-bejeweled' in d)
    seen, pieces = set(), []
    for d in folders:
        full = os.path.join(ROOT, d)
        for name in sorted(os.listdir(full)):
            if not name.endswith('.jsonl'):
                continue
            stem = name[:-6]
            title = ''
            for r in records(os.path.join(full, name)):
                if r.get('type') == 'custom-title':
                    title = r.get('customTitle') or r.get('title') or title
            if title.startswith('byo-pop coordinator'):
                group = 'coord'
            elif title in TITLES:
                group = TITLES[title]
            else:
                group = 'bejeweled' if 'trusting-matsumoto' in d else 'bigmoney' if 'bigmoney' in d else '?' + title
            own = []
            cur, last = None, '(start)'
            for r in records(os.path.join(full, name)):
                ts = r.get('timestamp') or ''
                tg = instruction(r)
                if tg:
                    last = tg[:600]
                if tg or cur is None or cur['closed']:
                    cur = {'key': f'{stem}|{len(own)}', 'group': group, 'title': title, 't0': ts, 'instruction': last,
                           'cost': 0.0, 'before': 0.0, 'commits': [], 'merges': [], 'agents': [], 'edits': [],
                           'closed': False}
                    own.append(cur)
                if ts and not cur['t0']:
                    cur['t0'] = ts
                m = r.get('message') or {}
                c = call_cost(m, r, seen)
                if c:
                    cur['cost'] += c
                    cur['before'] += c if ts < COORD_FROM else 0
                if r.get('type') != 'assistant' or not isinstance(m.get('content'), list):
                    continue
                for b in m['content']:
                    if not isinstance(b, dict) or b.get('type') != 'tool_use':
                        continue
                    inp = b.get('input') or {}
                    if b.get('name') in ('Agent', 'Workflow'):
                        dm = re.search(r"description:\s*['\"]([^'\"]{0,120})", inp.get('script') or '')
                        cur['agents'].append((inp.get('description') or (dm.group(1) if dm else ''))[:100])
                    elif b.get('name') in ('Edit', 'Write', 'MultiEdit'):
                        p = short_path(inp.get('file_path') or '')
                        if p not in cur['edits']:
                            cur['edits'].append(p)
                    elif b.get('name') in ('Bash', 'PowerShell'):
                        for merge, subj in commits_in(inp.get('command') or ''):
                            (cur['merges'] if merge else cur['commits']).append(subj)
                            cur['closed'] = True
            starts = []
            for p in own:
                starts.append(max(p['t0'], starts[-1]) if starts else p['t0'])
            sub = os.path.join(full, stem)
            for r2, _, ns in (os.walk(sub) if os.path.isdir(sub) else ()):
                for n2 in sorted(ns):
                    if not n2.endswith('.jsonl'):
                        continue
                    for r in records(os.path.join(r2, n2)):
                        c = call_cost(r.get('message') or {}, r, seen)
                        if c:
                            ts = r.get('timestamp') or ''
                            p = own[max(0, bisect.bisect_right(starts, ts) - 1)]
                            p['cost'] += c
                            p['before'] += c if ts < COORD_FROM else 0
            pieces += own
    for i, p in enumerate(pieces):
        p['id'] = i
        del p['closed']
    os.makedirs(OUT, exist_ok=True)
    json.dump(pieces, open(os.path.join(OUT, 'pieces.json'), 'w', encoding='utf-8'))
    classes = load_classes()
    todo = []
    for p in pieces:
        if p['key'] in classes or p['cost'] < 0.05 and not p['commits'] and not p['merges']:
            continue
        g = p['group']
        if g != 'coord' and not g.startswith('?') and not os.path.isdir(os.path.join(OUT, '..', '..', 'games', g, 'mods')):
            continue  # a game without mods: port work
        s = f"#{p['id']} [{g}] {p['t0'][:16]} ${p['cost']:.1f}"
        if g == 'coord' and p['before'] > 0.01:
            s += f" (before the split: ${p['before']:.1f})"
        s += f"\n  instruction: {p['instruction'][:260]}"
        for label, xs, n in (('commits', p['commits'], 6), ('merges', p['merges'], 6), ('agents', p['agents'], 8),
                             ('edits', p['edits'], 10)):
            if xs:
                s += f"\n  {label}: " + (', ' if label == 'edits' else ' | ').join(xs[:n])
        todo.append(s.encode('ascii', 'replace').decode())
    for f in os.listdir(OUT):
        if f.startswith('todo'):
            os.remove(os.path.join(OUT, f))
    per, files, cur, size = sum(map(len, todo)) // 6 + 1, [], [], 0
    for s in todo:
        cur.append(s)
        size += len(s)
        if size > per:
            files.append(cur)
            cur, size = [], 0
    if cur:
        files.append(cur)
    for k, f in enumerate(files):
        open(os.path.join(OUT, f'todo{k}.txt'), 'w', encoding='utf-8').write('\n'.join(f))
    unknown = sorted({p['group'][1:] for p in pieces if p['group'].startswith('?')})
    print(f'{len(pieces)} pieces, ${sum(p["cost"] for p in pieces):.0f}; {len(todo)} to classify in {len(files)} '
          f'todo files' + (f'; titles not in TITLES: {unknown}' if unknown else ''))


def load_classes():
    path = os.path.join(OUT, 'classes.json')
    return json.load(open(path, encoding='utf-8')) if os.path.exists(path) else {}


def classify(files):
    pieces = json.load(open(os.path.join(OUT, 'pieces.json'), encoding='utf-8'))
    classes = load_classes()
    n = 0
    for f in files:
        data = json.load(open(f, encoding='utf-8'))
        for it in (data.get('result') or data)['items']:
            if it['kind'] in KINDS and 0 <= it['id'] < len(pieces):
                classes[pieces[it['id']]['key']] = {'game': it['game'], 'kind': it['kind']}
                n += 1
    json.dump(classes, open(os.path.join(OUT, 'classes.json'), 'w', encoding='utf-8'), indent=0, sort_keys=True)
    print(f'{n} classifications added; {len(classes)} in all')


def table():
    pieces = json.load(open(os.path.join(OUT, 'pieces.json'), encoding='utf-8'))
    classes = load_classes()
    t = collections.defaultdict(lambda: dict.fromkeys(KINDS, 0.0))
    missing = 0.0
    for p in pieces:
        g, cl = p['group'], classes.get(p['key'])
        if cl:
            kind = cl['kind']
            game = cl['game'] if g == 'coord' else g  # a game's own session works for that game
        elif g == 'coord':
            game, kind = ('bejeweled', 'decomp') if p['before'] > p['cost'] / 2 else ('shared', 'other')
            missing += p['cost'] if p['cost'] >= 0.05 else 0
        else:
            game, kind = g, 'decomp'
            has_mods = os.path.isdir(os.path.join(OUT, '..', '..', 'games', g, 'mods'))
            missing += p['cost'] if has_mods and p['cost'] >= 0.05 else 0
        t[game][kind] += p['cost']
    print(f"{'game':14} {'port':>8} {'mods':>8} {'other':>8} {'total':>8}")
    for g, d in sorted(t.items(), key=lambda x: -sum(x[1].values())):
        print(f"{g:14} {d['decomp']:8.0f} {d['mods']:8.0f} {d['other']:8.0f} {sum(d.values()):8.0f}")
    print(f"{'all':14} " + ' '.join(f"{sum(d[k] for d in t.values()):8.0f}" for k in KINDS)
          + f" {sum(sum(d.values()) for d in t.values()):8.0f}")
    if missing:
        print(f'${missing:.0f} of pieces that need a classification were counted by default: run scan and classify')


if __name__ == '__main__':
    cmd = sys.argv[1] if len(sys.argv) > 1 else ''
    if cmd == 'scan':
        scan()
    elif cmd == 'rules':
        print(RULES)
    elif cmd == 'classify':
        classify(sys.argv[2:])
    elif cmd == 'table':
        table()
    else:
        sys.exit(__doc__)
