"""cmp_dirs.py <dirA> <dirB>: statelogs byte-for-byte, PNGs by hash, the run's last INFO lines (update counts)"""
import sys, os, hashlib, glob
a, b = sys.argv[1:3]
def h(p):
    return hashlib.sha256(open(p, 'rb').read()).hexdigest()
bad = 0
for f in sorted(os.listdir(a)):
    pa, pb = os.path.join(a, f), os.path.join(b, f)
    if os.path.isdir(pa) or f.endswith('.out'):
        continue
    if not os.path.exists(pb):
        print('MISSING in B:', f); bad += 1; continue
    ok = h(pa) == h(pb)
    if not ok:
        bad += 1
    if f.endswith('.sl') or not ok:
        print(('equal   ' if ok else 'DIFFERS ') + f, os.path.getsize(pa))
for f in sorted(os.listdir(b)):
    if not os.path.exists(os.path.join(a, f)):
        print('MISSING in A:', f); bad += 1
npng = len(glob.glob(os.path.join(a, '*.png')))
print(npng, 'png in A;', 'DIFFERENCES: %d' % bad if bad else 'ALL EQUAL (state logs and every PNG)')
# final lines of the .out files (exit state)
for f in sorted(glob.glob(os.path.join(a, '*.out'))):
    t = os.path.basename(f)
    la = [l for l in open(f, errors='replace') if 'Update Count' in l]
    lb = [l for l in open(os.path.join(b, t), errors='replace') if 'Update Count' in l] if os.path.exists(os.path.join(b, t)) else ['<none>']
    ua = la[-1].split('Update Count = ')[1].split(',')[0].strip() if la else '?'
    ub = lb[-1].split('Update Count = ')[1].split(',')[0].strip() if lb and '=' in lb[-1] else '?'
    print(t, 'updates', ua, ub, 'same' if ua == ub else 'DIFF')
