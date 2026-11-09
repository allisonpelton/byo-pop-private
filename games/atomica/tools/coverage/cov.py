"""One coverage run of the original Atomica (docs/COVERAGE.md): origrun with --coverage on the list that
`python -I tools/coverage.py list atomica` writes, on a scenario of games/atomica/tools/identical/ (the lines of compare.py:
"<update> <command> [args]", and "end <update>"; the "shot" lines of phase two are dropped, the host takes none).

    python -I games/atomica/tools/coverage/cov.py <name> <scenario> [seed] [timeout s] [clock: virtual|real] [reuse run]

`reuse run` names an earlier run (sv, which quits from a game and so leaves a SAVEGAME.DAT in its staged copy) whose copy this
run starts with, as compare.py's --savefile does for c1. A scenario with a line "# save: <kind>" starts from sv's copy with a
crafted saved game in place of its SAVEGAME.DAT: games/atomica/tools/coverage/mksave.py writes it into
build/coverage/atomica/save/<kind>.dat (a game with the element atoms, one a molecule from the end of a level, ...), which reaches
what the first levels never show without playing for hours.

The origrun files of the game (tools/origrun/atomica.cfg, patches/atomica.txt) are git-ignored and belong to each worktree:
setup_worktree.sh does not copy them.

The virtual clock (the default) runs the scenario's updates without waiting; the real clock runs them in real time, with the
original's update counter (0x4d0044, which every run passes: without it origrun skips the `at` lines) so that the scenario's `at` lines land on its updates, and
gives the sound code (the FMOD calls) its timing; both use origrun's silent stand-in for DirectSound. A scenario file that ends
in .orig is a script of origrun's own commands, used as it is. The hits go to
build/coverage/atomica/hits/<name>.txt, the run's log beside them. Run it from the worktree root, through run.sh (which waits for
room: at most 3 game processes on the machine).
"""
import os
import re
import shutil
import subprocess
import sys

ROOT = os.getcwd()
NL = '\n'
REGKEY = r'HKLM\SOFTWARE\WOW6432Node\PopCap\Atomica'


def win(path):
    return os.path.abspath(path).replace('/', '\\')


def scenario_lines(path, shift=0):
    """The origrun script of a scenario (compare.py's run_original), every update `shift` later."""
    out, end = [], None
    for l in open(path):
        l = l.split('#')[0].strip()
        if not l:
            continue
        w = l.split()
        if w[0] == 'end':
            end = int(w[1]) + shift
        elif len(w) > 1 and w[1] == 'shot':
            continue
        else:
            out.append('at %d %s' % (int(w[0]) + shift, ' '.join(w[1:])))
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
    run = 'atcov' + (reuse or name)
    save = ''
    if not scen.endswith('.orig'):
        for l in open(scen):                  # "# save: <kind>": the scenario continues a crafted saved game (mksave.py)
            if l.startswith('# save:'):
                save = os.path.join(ROOT, 'build', 'coverage', 'atomica', 'save', l.split(':')[1].strip() + '.dat')
    script = os.path.join(hits, name + '.script.txt')
    # a fresh game: the hints and settings the original keeps in the registry (the user allowed it, CLAUDE.md)
    for value in ('GameHints', 'ShowSymbols', 'ShowTips'):
        subprocess.run(['reg', 'delete', REGKEY, '/v', value, '/f'], capture_output=True)
    first_click = None
    if not scen.endswith('.orig'):
        for l in open(scen):
            w = l.split('#')[0].split()
            if len(w) > 3 and w[1] == 'click':
                first_click = (int(w[0]), w[2] + ' ' + w[3])
                break
    # The loading thread runs in real time while the virtual clock counts updates, so the update at which the title screen's link can
    # be clicked depends on the machine's load, and more so under --coverage: the same run finished loading by update 1500 one time
    # and not by 4400 the next. A run on the virtual clock watches the title screen (0x4d00fc, which the slam of the doors deletes 47
    # updates after the click) and, if it did not go at the first click, runs again with the whole scenario 3000 updates later.
    checked = first_click is not None and first_click[1] == '320 445' and clock == 'virtual'
    for shift in ((0, 3000, 6000, 9000, 12000) if checked else (0,)):
        with open(script, 'w', newline=NL) as f:
            if scen.endswith('.orig'):
                f.write(open(scen).read())      # origrun commands as they are (the real clock's tour)
            else:
                f.write(NL.join(scenario_lines(scen, shift)) + NL)
        if not reuse:
            shutil.rmtree('C:/byo-og/' + run, ignore_errors=True)
        if save:
            # the stage of a run that quit from a game (sv) is copied and its SAVEGAME.DAT replaced: origrun starts from a stage that exists
            if not os.path.isdir('C:/byo-og/atcovsv'):
                sys.exit('run sv first: its staged copy is the one a crafted saved game goes into')
            shutil.rmtree('C:/byo-og/' + run, ignore_errors=True)
            shutil.copytree('C:/byo-og/atcovsv', 'C:/byo-og/' + run)
            shutil.copy(save, 'C:/byo-og/' + run + '/game/SAVEGAME.DAT')
        shutil.rmtree(os.path.join(ROOT, 'build', 'origrun', run), ignore_errors=True)
        cmd = [os.path.join(ROOT, 'build', 'origrun-bin', 'origrun.exe'), '--game', 'atomica', '--run', run, '--stage-root', 'C:/byo-og',
               '--script', script, '--clock', clock, '--cursor', 'scripted', '--seed', seed, '--timeout', timeout,
               '--coverage', win(os.path.join(ROOT, 'build', 'coverage', 'atomica', 'functions.txt'))]
        # the original's update counter (app+0x3e4, the app object being static at 0x4cfc60: found by reading the running original twice, 102 a second), which the script's `at` lines wait for
        cmd += ['--counter', '0x4d0044', '--update-ms', '10']
        if checked:
            cmd += ['--watch', '0x4d00fc']
        with open(os.path.join(hits, name + '.log'), 'wb') as log:
            subprocess.run(cmd, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT)
        if not checked:
            break
        text = open(os.path.join(hits, name + '.log'), errors='replace').read()
        gone = re.search(r'watch 1: \d+ -> 0 at update (\d+)', text)
        if gone and abs(int(gone.group(1)) - (first_click[0] + shift + 47)) <= 3:
            break
        print('%s: the title screen was not clicked away at update %d (the loading was slower): again, 3000 updates later'
              % (name, first_click[0] + shift))
    else:
        print(name + ': the title screen never went; the run is not a good one')
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
