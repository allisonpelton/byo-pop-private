#!/usr/bin/env python3
"""winbad.py stage <backup dir> <start.reg> <file.bwa>... | winbad.py restore <backup dir>: the start state of a host run
of the original (run.sh with PILOT_HOST=on), which reads the user's own registry key and save folder.

stage backs up HKCU\\Software\\PopCap\\WinBAD (reg export) and C:\\ProgramData\\PopCap Games\\WinBAD\\users into <backup dir>,
deletes the key and imports <start.reg> in its place, and empties the folder and copies the given saves into it. A
backup dir that already exists is an earlier run not put back: stage refuses, and restore puts that one back first.
restore deletes the key and the folder and puts the backup's back, then removes the backup."""
import os
import shutil
import subprocess
import sys

KEY = r'HKCU\Software\PopCap\WinBAD'
USERS = r'C:\ProgramData\PopCap Games\WinBAD\users'


def reg(*args):
    return subprocess.run(['reg'] + list(args), capture_output=True, text=True)


def stage(backup, start, saves):
    if os.path.exists(backup):
        sys.exit('winbad: %s exists: an earlier run was not put back (winbad.py restore)' % backup)
    os.makedirs(backup)
    had_key = reg('export', KEY, os.path.join(backup, 'key.reg'), '/y').returncode == 0
    had_users = os.path.isdir(USERS)
    if had_users:
        shutil.copytree(USERS, os.path.join(backup, 'users'))
    open(os.path.join(backup, 'manifest.txt'), 'w').write('%d %d\n' % (had_key, had_users))
    reg('delete', KEY, '/f')
    r = reg('import', start)
    if r.returncode != 0:
        restore(backup)
        sys.exit('winbad: reg import %s failed: %s' % (start, r.stderr.strip()))
    shutil.rmtree(USERS, ignore_errors=True)
    os.makedirs(USERS)
    for f in saves:
        shutil.copyfile(f, os.path.join(USERS, os.path.basename(f)))


def restore(backup):
    mf = os.path.join(backup, 'manifest.txt')
    if not os.path.exists(mf):
        return
    had_key, had_users = (int(v) for v in open(mf).read().split())
    reg('delete', KEY, '/f')
    if had_key:
        r = reg('import', os.path.join(backup, 'key.reg'))
        if r.returncode != 0:
            sys.exit('winbad: reg import failed, the backup stays in %s: %s' % (backup, r.stderr.strip()))
    shutil.rmtree(USERS, ignore_errors=True)
    if had_users:
        shutil.copytree(os.path.join(backup, 'users'), USERS)
    shutil.rmtree(backup)


def main():
    if len(sys.argv) >= 4 and sys.argv[1] == 'stage':
        stage(sys.argv[2], sys.argv[3], sys.argv[4:])
    elif len(sys.argv) == 3 and sys.argv[1] == 'restore':
        restore(sys.argv[2])
    else:
        print(__doc__)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
