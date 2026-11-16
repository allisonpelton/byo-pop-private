"""Writes the "Latest phase done" column of README.md's progress table from docs/game_status.json.

    python -I tools/readme_status.py            rewrites README.md in place and prints the rows it changed
    python -I tools/readme_status.py --check    changes nothing; exits 1 when a row is out of date

A game's cell is the furthest phase it has finished with every phase before it finished too, in the order of the status
file's "steps" (port, identical play, registration out, looks identical, fixes mod, coverage, performance, lite mod,
unneeded code, readability, the Collection check, the site's features). A phase finished out of that order (the site's
features are often added early) shows once the phases before it are done, and a phase in the game's "na" list (the
Collection check of a game with no Steam copy) counts as done. A game with every phase done reads "Every
phase but modding"; one with none, "Porting" while its port is under way and "Not started" otherwise. A row is matched to
its game by the games/<id>/ path of its link, or else by the game's title; rows of several games are left alone.
"""
import json
import re
import sys

README = 'README.md'
STATUS = 'docs/game_status.json'
HEADER = re.compile(r'^\| Game \| Model \| (?:Status|Latest phase done) \|')
COLUMN = 'Latest phase done'
ALL_DONE = 'Every phase but modding'


def latest_phase(game, steps):
    """The label of the furthest step done with every step before it done, or the cell for a game with none."""
    done = set(game['done']) | set(game.get('na', []))  # a step that does not apply to the game counts as done
    reached = None
    for step in steps:
        if step['id'] not in done:
            break
        reached = step
    else:
        return ALL_DONE
    if reached:
        return reached['label']
    return 'Porting' if game.get('now') == 'port' else 'Not started'


def row_game(first_cell, games, by_title):
    """The id of the game a row names, or None for a row of several games or none."""
    link = re.search(r'\]\(games/([a-z0-9]+)/', first_cell)
    if link and link.group(1) in games:
        return link.group(1)
    title = re.sub(r'\[([^\]]*)\]\([^)]*\)', r'\1', first_cell).strip()
    return by_title.get(title)


def main():
    check_only = '--check' in sys.argv[1:]
    status = json.load(open(STATUS, encoding='utf-8'))
    steps, games = status['steps'], status['games']
    by_title = {game['title']: game_id for game_id, game in games.items()}
    text = open(README, encoding='utf-8', newline='').read()
    newline = '\r\n' if '\r\n' in text else '\n'
    lines = text.split(newline)
    start = next((i for i, line in enumerate(lines) if HEADER.match(line)), None)
    if start is None:
        sys.exit('readme_status: no progress table in %s' % README)
    lines[start] = HEADER.sub('| Game | Model | %s |' % COLUMN, lines[start])
    changed, seen = [], set()
    for i in range(start + 2, len(lines)):
        if not lines[i].startswith('|'):
            break
        cells = lines[i].split('|')
        game_id = row_game(cells[1], games, by_title)
        if not game_id:
            continue
        seen.add(game_id)
        cell = ' %s ' % latest_phase(games[game_id], steps)
        if cells[3] != cell:
            cells[3] = cell
            lines[i] = '|'.join(cells)
            changed.append('%s:%s' % (game_id, cell.rstrip()))
    missing = sorted(set(games) - seen)
    if check_only:
        for line in changed:
            print('out of date: ' + line)
        sys.exit(1 if changed else 0)
    open(README, 'w', encoding='utf-8', newline='').write(newline.join(lines))
    for line in changed:
        print(line)
    if missing:
        print('no row of its own: ' + ' '.join(missing))


if __name__ == '__main__':
    main()
