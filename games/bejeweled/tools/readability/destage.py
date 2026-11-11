"""Takes the development-stage labels out of the comments of games/bejeweled/src (CODE_STANDARDS.md rule 8). Line counts
stay the same. `python destage.py` prints what is left; `--write` writes the files."""
import re, glob, sys
S = r'[Ss]tage[ -]?\d+[a-c]?'
NAME = {'2': 'Per-game seeds', '2b': 'The LiveSplit observer', '3': 'Replays', '3b': 'Watching a replay', '5': 'The move counts',
        '6': 'Undo', '7': 'Board shapes', '7b': 'Dirt', '7c': 'Gravity', '8': 'More gem colours', '9': 'The match rules',
        '10': 'Twists and slides', '11': 'Flame and lightning gems', '12': 'The gold board', '13': 'Watching from the game',
        '14': 'Online scores', '17': 'Game statistics', '18': 'Arrow gems'}

HAND = [
 ("CODE_MODS_DESIGN.md 4.20, stage 18; the rules are in", "CODE_MODS_DESIGN.md 4.20; the rules are in"),
 ("as there (mods/stage4/overflow)", "as there (the oracle's overflow mod, mods/stage4/overflow)"),
 ("the kept invalid moves; stage 6:", "the kept invalid moves;"),
 ("with twists or slides (stage", "with twists or slides (moves.type:"),
 (" 10) INSTRUCTIONDIALOG_", " CODE_MODS_DESIGN.md 4.4 and 4.5) INSTRUCTIONDIALOG_"),
 ("7.3; stage 5's v2 save keeps it", "7.3; a variant board's v2 save keeps it"),
 ("or a save from before stage 3)", "or a save without a sidecar)"),
 ("not in this build (stage 14's online scores)", "not in this build (the online scores)"),
 ("The swaps: stage 9's FindMoveVariant,", "The swaps: the match rules' FindMoveVariant,"),
 ("it finds what stage 9's search found.", "it finds what the match rules' own search finds."),
 ("any other chunk: a later stage's, skipped", "any other chunk: a newer engine's, skipped"),
 ("(6.4). 2: stage 5, kept invalid moves,", "(6.4). 2: kept invalid moves,"),
 ("LOGIC hash. 3: stage 7, the board shape rules", "LOGIC hash. 3: the board shape rules"),
 ("(Board::RollScoreFast). 4: stage 7b, dirt cells", "(Board::RollScoreFast). 4: dirt cells"),
 ("slides. 5: stage 8, board.gemColors", "slides. 5: board.gemColors"),
 ("played as 7. 6: stage 9, the match rules", "played as 7. 6: the match rules"),
 ("as runs of 3. 7: stage 10,", "as runs of 3. 7:"),
 ("as swaps. 8: stage 11, flame", "as swaps. 8: flame"),
 ("never made. 9: stage 12, the gold board", "never made. 9: the gold board"),
 ("as score levels. 10: stage 18, arrow gems", "as score levels. 10: arrow gems"),
 ("(and in the stage-0 list of testhooks/StateHash.cpp's", "(and in the frozen baseline's list of testhooks/StateHash.cpp's"),
 ("FULL: the stage-0 -tracehash list's own-RNG fields", "FULL: the frozen baseline's -tracehash list's own-RNG fields"),
 ("bookkeeping, stage 2b: design 4.15 gives them no row", "bookkeeping: CODE_MODS_DESIGN.md 4.15 gives them no row"),
 ("Not FULL: that stays the stage-0", "Not FULL: that stays the frozen baseline's"),
 ("(so the replays' hashes of stage 6 still hold)", "(so the hashes of replays recorded before the board shapes still hold)"),
 ("the board part of the stage-0", "the board part of the frozen baseline's"),
 ("comparison with the frozen stage-0 build proves", "comparison with the frozen baseline (build/baseline-stage0) proves"),
 (" port(variant): stage 13 (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.10, 4.11, 8)", " port(variant): watching from the game and Extras (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.10, 4.11, 8)"),
 (" port(variant): stage 13", " port(variant): watching a replay from the game"),
 ("grown for more colours (stage 8,", "grown for more colours ("),
 ("stay equal to the stage-0 baselines.", "stay equal to the frozen baselines."),
 ("It stays the stage-0 list, since the oracle", "It stays the frozen baseline's list, since the oracle"),
 ("with the frozen stage-0 build; the replays'", "with the frozen baseline (build/baseline-stage0); the replays'"),
 ("FULL encoding (port(variant), stage 3:", "FULL encoding (port(variant):"),
 ("the same bytes the stage-0 field list hashed)", "the same bytes the frozen baseline's field list hashed)"),
 (" in the stage-0 build.", " in the frozen baseline."),
 ("hashed field by field in stage 0 (the field", "hashed field by field in the frozen baseline (the field"),
]
ALLOWED = ('mods/stage4/overflow', 'stage_assets.py', 'staged copy', 'baseline-stage0')
def up(m): return m.group(1).upper()
def named(m):
    return NAME[m.group(1)] + m.group(2)
def fix(line):
    if '//' not in line: return line
    code, sep, com = line.partition('//')
    o = com
    com = re.sub(r'\(' + S + r'[,;:] ', '(', com)
    com = re.sub(r'[,;] ' + S + r'\)', ')', com)
    com = re.sub(r' \(' + S + r'\)', '', com)
    com = re.sub(r'[Ss]tage (\d+[a-c]?)( \()', named, com) if re.search(r'(^|\. )\s*Stage \d+[a-c]? \(', com) else com
    com = re.sub(r'Stage \d+[a-c]?[:,] (\w)', up, com)
    com = re.sub(r'(?<=/ )stage \d+[a-c]?: ', '', ' ' + com)[1:] if re.match(r'\s*stage \d+[a-c]?: ', com) else com
    com = re.sub(r'; stage \d+[a-c]?: ', '; ', com)
    com = re.sub(r'^(\s*)stage \d+[a-c]?: ', r'\1', com)
    if com != o and 'CODE_MODS_DESIGN' not in com:
        com = re.sub(r'\((4\.\d+)([,):;])', r'(CODE_MODS_DESIGN.md \1\2', com)
    for a, b in HAND:
        com = com.replace(a, b)
    return code + sep + com
write = '--write' in sys.argv
left = 0
for f in sorted(glob.glob('games/bejeweled/src/**/*', recursive=True)):
    if not f.endswith(('.cpp', '.h', '.inc')): continue
    t = open(f, encoding='utf-8', newline='').read()
    lines = t.split('\n')
    new = [fix(l) for l in lines]
    for i, l in enumerate(new):
        k = l
        for a in ALLOWED: k = k.replace(a, '')
        if re.search('stage', k, re.I):
            left += 1; print('%s:%d:%s' % (f[22:], i + 1, l.strip()[:230]))
    if write and new != lines:
        open(f, 'w', encoding='utf-8', newline='').write('\n'.join(new))
print('left', left)
