"""One line a run from pairs.sh's runs.txt: python -I summary.py <runs.txt>..."""
import json
import sys

for path in sys.argv[1:]:
    for line in open(path):
        kind, _, rest = line.partition(' ')
        if not rest.startswith('{'):
            print(line.rstrip())
            continue
        r = json.loads(rest)
        w, s = r.get('whole_run') or {}, r.get('scripted_stretch') or {}
        print('%-16s run %6s s | whole: cpu %5s s ws %5s priv %5s MB | stretch: cpu %5s s in %5s s, %7s upd/s (%s upd) | load %s | at load: ws %s priv %s cpu %s | others %s/%s busy %s' % (
            kind, r.get('run_s'), w.get('processor_s'), w.get('peak_working_set_mb'), w.get('peak_private_mb'),
            s.get('processor_s'), s.get('wall_s'), r.get('updates_per_s_in_stretch'), r.get('updates_in_stretch'),
            r.get('load_s'), r.get('working_set_mb_at_loaded', r.get('working_set_mb_at_title')),
            r.get('private_mb_at_loaded', r.get('private_mb_at_title')), r.get('processor_s_at_loaded', r.get('processor_s_at_title')),
            r.get('others_start'), r.get('others_end'), r.get('machine_busy')) + ('' if r.get('game_played', True) else ' NOT PLAYED') + (' state %s' % r['state_at_stretch_start_and_end'][-1] if r.get('state_at_stretch_start_and_end') else ''))
