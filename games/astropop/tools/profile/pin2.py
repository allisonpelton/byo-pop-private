"""pincore.py with two performance cores instead of one, for a process whose busy threads would otherwise share one core's two hyperthreads.

    python -I pin2.py -- <command> [args...]

pincore (tools/pincore.py, the coordinator's) gives a command one physical core and both of its logical processors, so that no other
pincore run shares the core. That is right for a process with one busy thread, but the port in 3D has two (the game's and the graphics
driver's, about 35% of the process) beside the audio thread: when the two hyperthreads of a core run both at once each gets about two
thirds of the core, and the cycle counts, which count time on the core, go up by half for the same work, while the original's second
thread is light. This claims two cores' named mutexes (the same names as pincore's, so pincore runs keep off them) and sets the
affinity to the four logical processors. The comparison with a one-core run is in GAME_NOTES.md, section 11.
"""
import ctypes
import importlib.util
import os
import shutil
import subprocess
import sys
import time

PINCORE = next(p for p in (os.path.join(os.getcwd(), 'tools', 'pincore.py'),
                           'C:/Users/Administrator/Documents/claude/byo-pop/tools/pincore.py') if os.path.exists(p))
spec = importlib.util.spec_from_file_location('pincore', PINCORE)
pc = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pc)


def claim_two(cores):
    """Hold the named mutexes of two free cores (waits while fewer than two are free); returns ({core: handle})."""
    while True:
        held = {}
        for core in sorted(cores, reverse=True):
            handle = pc.kernel32.CreateMutexW(None, False, 'Local\\byopop-pincore-%d' % core)
            if handle and ctypes.get_last_error() != pc.ERROR_ALREADY_EXISTS:
                held[core] = handle
            elif handle:
                pc.kernel32.CloseHandle(handle)
            if len(held) == 2:
                return held
        for handle in held.values():
            pc.kernel32.CloseHandle(handle)
        time.sleep(5)


def main(argv):
    if argv[:1] != ['--'] or len(argv) < 2:
        sys.stderr.write(__doc__)
        return 2
    command = argv[1:]
    found = shutil.which(command[0]) or (os.path.abspath(command[0]) if os.path.exists(command[0]) else None)
    if found:
        command[0] = found
    cores = pc.performance_cores()
    held = claim_two(cores)
    mask = 0
    for core in held:
        for logical in cores[core]:
            mask |= 1 << logical
    if not pc.kernel32.SetProcessAffinityMask(pc.kernel32.GetCurrentProcess(), mask):
        sys.stderr.write('pin2: SetProcessAffinityMask failed (%d)\n' % ctypes.get_last_error())
        return 1
    sys.stderr.write('pin2: cores %s (logical processors %s)\n' % (sorted(held), ','.join(str(l) for c in sorted(held) for l in cores[c])))
    try:
        return subprocess.call(command)
    finally:
        for handle in held.values():
            pc.kernel32.CloseHandle(handle)


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
