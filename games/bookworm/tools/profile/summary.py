"""summary.py <out dir> [v]: the figures of pairs.sh's runs (p<n>-port.json, p<n>-orig.json, p<n>-portload.json,
p<n>-origload.json) as a median with its range, for GAME_NOTES.md "Performance"; with v, those of variants.sh's runs
(v<n>-<variant>.json) for each variant."""
import glob
import json
import statistics
import sys

out = sys.argv[1]
prefix = 'v' if len(sys.argv) > 2 and sys.argv[2] == 'v' else 'p'


def runs(suffix):
    rows = []
    for name in sorted(glob.glob('%s/%s*-%s.json' % (out, prefix, suffix))):
        for line in open(name):
            if line.startswith('{'):
                d = json.loads(line)
                if 'whole_run' in d or 'load_s' in d:
                    rows.append(d)
    return rows


def show(label, values, unit, digits=2):
    values = [v for v in values if v is not None]
    if not values:
        print('%-46s none' % label)
        return
    f = '%%.%df' % digits
    print('%-46s %s to %s %s (median %s, %d runs)' % (label, f % min(values), f % max(values), unit, f % statistics.median(values), len(values)))


for side in (('default', 'novsync', 'd3d9', 'gl', 'software') if prefix == 'v' else ('port', 'orig')):
    rows = runs(side)
    print('== %s, real time' % side)
    show('processor time of the 20 s stretch', [r['scripted_stretch']['processor_s'] for r in rows], 's')
    show('  as a share of one core', [100 * r['scripted_stretch']['processor_s'] / r['scripted_stretch']['wall_s'] for r in rows], '%', 1)
    show('peak private bytes of the run', [r['whole_run']['peak_private_mb'] for r in rows], 'MB', 1)
    show('updates a second in the stretch', [r.get('updates_per_s_in_stretch') for r in rows], '', 1)
    show('machine busy', [r['machine_busy'] for r in rows], '', 2)
    show('other game processes at the start', [r['others_start'] for r in rows], '', 0)
    if prefix == 'v':
        continue
    rows = runs(side + 'load')
    print('== %s, load' % side)
    show('load to the title', [r['load_s'] for r in rows], 's')
    show('private bytes at the title', [r['private_mb_at_title'] for r in rows], 'MB', 1)
    show('processor time to the title', [r['processor_s_at_title'] for r in rows], 's')
