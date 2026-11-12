# stagefree.py <stage name>: exit code 0 when no BookwormGame.exe runs from a folder with that name in its path
import subprocess
import sys

BACKSLASH = chr(92)
name = sys.argv[1].lower()
query = "Get-CimInstance Win32_Process -Filter \"Name like 'BookwormGame%'\" | ForEach-Object { $_.ExecutablePath }"
out = subprocess.run(['powershell', '-NoProfile', '-Command', query], capture_output=True, text=True).stdout
needle = BACKSLASH + name + BACKSLASH
sys.exit(1 if any(needle in l.lower().replace('/', BACKSLASH) for l in out.splitlines()) else 0)
