"""One coverage run of the original Atomica (docs/COVERAGE.md): origrun with --coverage on the list that
`python -I tools/coverage.py list atomica` writes, on a scenario of games/atomica/tools/identical/ (the lines of compare.py:
"<update> <command> [args]", and "end <update>"; the "shot" lines of phase two are dropped, the host takes none).

    python -I games/atomica/tools/coverage/cov.py <name> <scenario> [seed] [timeout s] [clock: virtual|real] [reuse run]

`reuse run` names an earlier run (sv, which quits from a game and so leaves a SAVEGAME.DAT in its staged copy) whose copy this
run starts with, as compare.py's --savefile does for c1.

The virtual clock (the default) runs the scenario's updates without waiting; the real clock runs them in real time, with the
original's update counter (0x4d0048, the one sound_orig.sh passes) so that the scenario's `at` lines land on its updates, and
gives the sound code (the FMOD calls) its timing; both use origrun's silent stand-in for DirectSound. A scenario file that ends
in .orig is a script of origrun's own commands, used as it is. The hits go to
build/coverage/atomica/hits/<name>.txt, the run's log beside them. Run it from the worktree root, through run.sh (which waits for
room: at most 3 game processes on the machine).
"""
import os
import shutil
import subprocess
import sys

ROOT = os.getcwd()
REGKEY = r'HKLM\SOFTWARE\WOW6432Node\PopCap\Atomica'


def win(path):
    return os.path.abspath(path).replace('/', '\\')


def scenario_lines(path):
    """The origrun script of a scenario (compare.py's run_original)."""
    out, end = [], None
    for l in open(path):
        l = l.split('#')[0].strip()
        if not l:
            continue
        w = l.split()
        if w[0] == 'end':
            end = int(w[1])
        elif len(w) > 1 and w[1] == 'shot':
            continue
        else:
            out.append('at %d %s' % (int(w[0]), ' '.join(w[1:])))
    if end is None:
        end = max(int(x.split()[1]) for x in out) + 100
    out.append('at %d move 1 1' % end)
    out.append('quit')
    return out


def main():
    name, scen = sys.argv[1], sys.argv[2]
    seed = sys.argv[3] if len(sys.argv) > 3 else '1'
    timeout = sys.argv[4] if len(sys.argv) > 4 else '240'
    clock = sys.argv[5] if len(sys.argv) > 5 else 'virtual'
    reuse = sys.argv[6] if len(sys.argv) > 6 else ''     # the run whose staged copy (and SAVEGAME.DAT) this one starts with
    hits = os.path.join(ROOT, 'build', 'coverage', 'atomica', 'hits')
    os.makedirs(hits, exist_ok=True)
    run = 'cov' + (reuse or name)
    script = os.path.join(hits, name + '.script.txt')
    with open(script, 'w', newline='\n') as f:
        if scen.endswith('.orig'):
            f.write(open(scen).read())      # origrun commands as they are (the real clock's tour)
        else:
            f.write('\n'.join(scenario_lines(scen)) + '\n')
    # a fresh game: the hints and settings the original keeps in the registry (the user allowed it, CLAUDE.md)
    for value in ('GameHints', 'ShowSymbols', 'ShowTips'):
        subprocess.run(['reg', 'delete', REGKEY, '/v', value, '/f'], capture_output=True)
    if not reuse:
        shutil.rmtree('C:/byo-og/' + run, ignore_errors=True)
    shutil.rmtree(os.path.join(ROOT, 'build', 'origrun', run), ignore_errors=True)
    cmd = [os.path.join(ROOT, 'build', 'origrun-bin', 'origrun.exe'), '--game', 'atomica', '--run', run, '--stage-root', 'C:/byo-og',
           '--script', script, '--clock', clock, '--cursor', 'scripted', '--seed', seed, '--timeout', timeout,
           '--coverage', win(os.path.join(ROOT, 'build', 'coverage', 'atomica', 'functions.txt'))]
    if clock == 'real':
        cmd += ['--counter', '0x4d0048', '--update-ms', '10']     # the original's update counter, for the script's `at` lines
    with open(os.path.join(hits, name + '.log'), 'wb') as log:
        subprocess.run(cmd, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT)
    src = os.path.join(ROOT, 'build', 'origrun', run, 'coverage.txt')
    if not os.path.exists(src):
        print(name + ': no hits file')
        sys.exit(1)
    shutil.copy(src, os.path.join(hits, name + '.txt'))
    lines = [l for l in open(src) if l.strip()]
    marks = [l.strip() for l in lines if l.startswith('#')]
    print('%s: %d functions; %s' % (name, len([l for l in lines if not l.startswith('#')]), marks[-1] if marks else ''))


if __name__ == '__main__':
    main()
