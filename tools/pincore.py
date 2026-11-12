"""Run a command on one physical performance core of its own, for processor-time measurements.

    python -I tools/pincore.py [--core N] -- <command> [args...]

A measurement on a busy machine moves for reasons that have nothing to do with the game: the scheduler hops a thread
across every logical processor (a test thread here visited all 20 in 8 s, starting each time with cold L1 and L2
caches), sometimes onto an efficiency core, and the other hyperthread of the same core may be running another
process, which takes about a third of the core's throughput while the game is still charged full time. This script
claims one P-core that no other pincore run holds (a named mutex per core, held while the command runs), sets its own
affinity to that core's two logical processors, and starts the command, which inherits the affinity, as do its
children: rundesk, origrun and the game they start. Other sessions' runs that use pincore then never share the core.
Programs that don't use it, the user's included, may still land on it, so take alternating runs and medians as before.

Priority is not raised: rundesk's below-normal priority still lets the user's work go first.
"""

import ctypes
import ctypes.wintypes as W
import os
import shutil
import subprocess
import sys
import time

kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
kernel32.CreateMutexW.restype = W.HANDLE
kernel32.CreateMutexW.argtypes = [ctypes.c_void_p, W.BOOL, W.LPCWSTR]
kernel32.CloseHandle.argtypes = [W.HANDLE]
kernel32.GetCurrentProcess.restype = W.HANDLE
kernel32.SetProcessAffinityMask.argtypes = [W.HANDLE, ctypes.c_size_t]
ERROR_ALREADY_EXISTS = 183


def performance_cores():
    """{core index: [logical processors]} of the cores with the highest efficiency class (the P-cores)."""
    size = W.ULONG(0)
    kernel32.GetSystemCpuSetInformation(None, 0, ctypes.byref(size), None, 0)
    buf = (ctypes.c_ubyte * size.value)()
    if not kernel32.GetSystemCpuSetInformation(buf, size, ctypes.byref(size), None, 0):
        raise OSError(ctypes.get_last_error(), "GetSystemCpuSetInformation")
    entries = []
    off = 0
    while off < size.value:
        entry_size = int.from_bytes(bytes(buf[off:off + 4]), "little")
        # SYSTEM_CPU_SET_INFORMATION: Size, Type (4 bytes each), Id (4), Group (2), LogicalProcessorIndex (1),
        # CoreIndex (1), LastLevelCacheIndex (1), NumaNodeIndex (1), EfficiencyClass (1)
        group = int.from_bytes(bytes(buf[off + 12:off + 14]), "little")
        logical, core, efficiency = buf[off + 14], buf[off + 15], buf[off + 18]
        if group == 0:
            entries.append((efficiency, core, logical))
        off += entry_size
    best = max(e for e, _, _ in entries)
    cores = {}
    for efficiency, core, logical in entries:
        if efficiency == best:
            cores.setdefault(core, []).append(logical)
    return cores


def claim(cores, wanted=None):
    """Hold the named mutex of a free core; wait while every core is held. Returns (core, handle)."""
    # the highest cores first: Windows sends many interrupts to logical processor 0
    order = [wanted] if wanted is not None else sorted(cores, reverse=True)
    while True:
        for core in order:
            handle = kernel32.CreateMutexW(None, False, "Local\\byopop-pincore-%d" % core)
            if handle and ctypes.get_last_error() != ERROR_ALREADY_EXISTS:
                return core, handle
            if handle:
                kernel32.CloseHandle(handle)
        time.sleep(5)


def main(argv):
    wanted = None
    if argv[:1] == ["--core"]:
        wanted = int(argv[1])
        argv = argv[2:]
    if argv[:1] != ["--"] or len(argv) < 2:
        sys.stderr.write(__doc__)
        return 2
    command = argv[1:]
    # CreateProcess does not search a relative path written with forward slashes (build/native/rundesk.exe)
    found = shutil.which(command[0]) or (os.path.abspath(command[0]) if os.path.exists(command[0]) else None)
    if found:
        command[0] = found
    cores = performance_cores()
    if wanted is not None and wanted not in cores:
        sys.stderr.write("pincore: core %d is not a performance core (they are %s)\n" % (wanted, sorted(cores)))
        return 2
    core, handle = claim(cores, wanted)
    mask = 0
    for logical in cores[core]:
        mask |= 1 << logical
    if not kernel32.SetProcessAffinityMask(kernel32.GetCurrentProcess(), mask):
        sys.stderr.write("pincore: SetProcessAffinityMask failed (%d)\n" % ctypes.get_last_error())
        return 1
    sys.stderr.write("pincore: core %d (logical processors %s)\n" % (core, ",".join(map(str, cores[core]))))
    try:
        return subprocess.call(command)
    finally:
        kernel32.CloseHandle(handle)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
