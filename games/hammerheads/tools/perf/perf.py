#!/usr/bin/env python3
"""Processor time of Hammer Heads like for like (docs/COORDINATION.md, "A performance pass"): the port and the original play the same
stretch of Classic Bash (the `classic1` scenario's inputs, level 1 in play) in real time, and the cycles of both are split into
update, draw and present between update FROM and update TO (default 4000 to 8000: 40 seconds of play).

    python games/hammerheads/tools/perf/perf.py port <label> [--from N] [--to N] [--port-arg A]...
    python games/hammerheads/tools/perf/perf.py orig <label> [--from N] [--to N] [--total] [--paced MS]
    (--total pauses the counting inside the window, for the process cycles as a player's run has them; without it the parts are split)
    python -I tools/cyclesplit.py build/perf/hammerheads/<label>/port.json build/perf/hammerheads/<label>/orig.json --from a --to b

The port runs under rundesk, windowed at the game's own size (-windowscale=1), with -cyclesplit and no -commands channel; the original
through tools/origrun with --count (games/hammerheads/tools/perf/count.txt) on the wall clock. Run each through tools/pincore.py,
one game process at a time (pairs.sh), and take medians of alternating pairs.
"""
import os
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'idplay'))
import idplay  # noqa: E402

ROOT = idplay.ROOT


CAVE = 4944704          # sixteen bytes of padding (int3) in the exe's code, where the pacing stub goes
STEP_CALL = 0x4d7e0c     # SexyAppBase::UpdateAppStep (0x4d7d60): mov ecx, esi; call 0x4d3290
SLEEP_IAT = 0x5511b8


def paced_patches(out, base, ms):
    """The patch list with a Sleep(ms) in each pass of the original's main loop: the stub `push ms; call [Sleep]; mov ecx, esi;
    jmp 0x4d3290` in the cave, called in place of UpdateAppStep's own call of 0x4d3290 (the hidden desktop's DirectDraw does not wait
    for a vertical blank, so the unpatched original spins about 67 passes an update where a 60 Hz screen's blank would give 0.6)."""
    stub = bytes([0x6a, ms, 0xff, 0x15]) + struct.pack('<I', SLEEP_IAT) + bytes([0x89, 0xf1, 0xe9])
    stub += struct.pack('<I', (0x4d3290 - (CAVE + len(stub) + 4)) & 0xffffffff)
    call = bytes([0xe8]) + struct.pack('<I', (CAVE - (STEP_CALL + 5)) & 0xffffffff) + bytes([0x90, 0x90])
    path = os.path.join(out, 'patches-paced.txt')
    with open(path, 'w') as f:
        f.write(open(base).read().rstrip('\n') + '\n')
        f.write('test HammerHeadsGame.exe va:0x%x %s %s a stub in unused padding: Sleep(%d), then the call of 0x4d3290 that UpdateAppStep made\n'
                % (CAVE, 'cc' * len(stub), stub.hex(), ms))
        f.write('test HammerHeadsGame.exe va:0x%x 8bcee87db4ffff %s UpdateAppStep calls the stub: each pass sleeps %d ms\n'
                % (STEP_CALL, call.hex(), ms))
    return path


def main():
    args = sys.argv[1:]
    kind, label = args[0], args[1]
    opts = {'--from': 4000, '--to': 8000, '--paced': 0}
    port_args = []
    total = '--total' in args
    args = [a for a in args if a != '--total']
    i = 2
    while i < len(args):
        if args[i] == '--port-arg':
            port_args.append(args[i + 1])
        else:
            opts[args[i]] = int(args[i + 1])
        i += 2
    first, last = opts['--from'], opts['--to']
    scenario = idplay.load_scenario('classic1')
    out = os.path.join(ROOT, 'build', 'perf', 'hammerheads', label)
    os.makedirs(out, exist_ok=True)
    if kind == 'port':
        lines = []
        for count, _, port_command in scenario.INPUTS:
            if port_command and count <= last:
                lines.append((count + 1, port_command))
        # --total: the readings are paused inside the window (the sites then cost one flag's test), so the process cycles are the player's
        marks = [(first, 'cyclesplit a')] + ([(first, 'cyclesplit pause')] if total else []) + ([(last, 'cyclesplit resume')] if total else []) + [(last, 'cyclesplit b'), (last + 10, 'quit')]
        lines += marks
        lines.sort(key=lambda item: item[0])  # stable: a before pause
        script = os.path.join(out, 'port.txt')
        open(script, 'w', newline='\n').write('\n'.join('%d %s' % item for item in lines) + '\n')
        user = os.path.join(out, 'user')
        subprocess.call(['cmd', '/c', 'rmdir', '/s', '/q', user], stderr=subprocess.DEVNULL)
        os.makedirs(user)
        # windowed, as the original's run is (its ScreenMode is 0 through origrun's --reg); a fresh port starts full screen, which is
        # the whole desktop under rundesk
        with open(os.path.join(user, 'registry.ini'), 'w') as registry:
            registry.write('[SOFTWARE\\PopCap\\HammerHeads]\nScreenMode=dword:0\n' + os.environ.get('PERF_REG', ''))
        command = [idplay.RUNDESK, idplay.PORT_EXE, '-seed=1', '-windowscale=1', '-userdir=' + user, '-script=' + script,
                   '-cyclesplit=' + os.path.join(out, 'port.json')] + port_args
        status = subprocess.call(command, stdout=open(os.path.join(out, 'port.log'), 'w'), stderr=subprocess.STDOUT, cwd=ROOT)
    else:
        lines = ['waitfor %s > 0 120000' % idplay.COUNTER]
        inputs = {}
        for count, original_command, _ in scenario.INPUTS:
            if original_command and count <= last:
                inputs.setdefault(count, []).append(original_command)
        for count in sorted(set(inputs) | {first, last}):
            for command in inputs.get(count, []):
                lines.append('at %d %s' % (count, command))
            if count == first:
                lines.append('at %d count a' % count)
                if total:
                    lines.append('at %d count pause' % count)
            if count == last:
                lines.append('at %d count b' % count)
        lines.append('at %d quit' % (last + 10))
        script = os.path.join(out, 'orig.txt')
        open(script, 'w', newline='\n').write('\n'.join(lines) + '\n')
        counts = os.path.join(ROOT, 'build', 'origrun', idplay.ORIGINAL_RUN, 'counts.jsonl')
        if os.path.exists(counts):
            os.remove(counts)
        subprocess.call(['cmd', '/c', 'rmdir', '/s', '/q', os.path.join(os.path.dirname(idplay.ORIGINAL_EXE), 'users')], stderr=subprocess.DEVNULL)
        command = [idplay.ORIGRUN, '--exe', idplay.ORIGINAL_EXE, '--run', idplay.ORIGINAL_RUN, '--stage-root', idplay.STAGE_ROOT,
                   '--script', script, '--clock', 'real', '--reg-map', 'popcap', '--cursor', 'scripted', '--counter', idplay.COUNTER,
                   '--patches', (paced_patches(out, idplay.patches_file(out, scenario), opts['--paced']) if opts['--paced'] else idplay.patches_file(out, scenario)), '--seed', '1', '--count', os.path.join(HERE, 'count.txt'),
                   '--timeout', str(120 + (last - first) // 100)]
        status = subprocess.call(command, stdout=open(os.path.join(out, 'orig.log'), 'w'), stderr=subprocess.STDOUT, cwd=ROOT)
        if os.path.exists(counts):
            import shutil
            shutil.copyfile(counts, os.path.join(out, 'orig.json'))
    print('%s %s: exit %d' % (kind, label, status))
    return 0


if __name__ == '__main__':
    sys.exit(main())
