# soundcount.py (run from the repository root, with the Ghidra export in re/bonnies): for each function the port names by address, the original's calls to the
# sound system (0x41e760-0x41f2b0) and to the game's playSound helpers (0x401070, 0x406360, 0x4063e0), and the port's calls to the same; prints the functions where
# the counts differ. Like rngcount.py for the random draws: which sounds and music a function starts, stops or changes.
import re, glob, bisect, collections
# the original's callee -> the port's name; the two play()s and the three playSound() helpers that only pass their arguments on are one "play".
# Where the original's compiler merged a call shared by several branches into one (0x414400: one error sound for three cases, 0x41a810: one
# sound for two), the port has a call in each branch, and the counts differ by those.
CALLEES = {0x41e9c0: "play", 0x41eb40: "play", 0x41ec60: "setMusicVolume", 0x41ece0: "isMusicPlaying", 0x41ed30: "stopMusic",
           0x41edc0: "pause", 0x41edf0: "pauseMusicOnly", 0x41ee10: "resume", 0x41ee40: "shutdown", 0x41ee80: "loadSample", 0x41e990: "freeSample",
           0x41ef90: "playMusic", 0x41f060: "playStream", 0x41f2b0: "update",
           0x401070: "play", 0x406360: "play", 0x4063e0: "play",
           # two one-line helpers the port writes out where they are called: a stream at volume 1 (0x40dd00) and a sample at volume 1 (0x417b20)
           0x40dd00: "playStream", 0x417b20: "play"}
NAMES = sorted(set(CALLEES.values()))
addrs = set()
port = collections.Counter()
for f in glob.glob("games/bonnies/engine/*.cpp") + glob.glob("games/bonnies/game/*.cpp"):
    cur = None
    insound = f.replace("\\", "/").endswith("engine/Sound.cpp")
    for line in open(f, encoding="utf-8", errors="replace"):
        m = re.match(r"^// (0x4[0-9a-f]{5})", line)
        if m:
            cur = int(m.group(1), 16); addrs.add(cur); continue
        if cur is None or line.lstrip().startswith("//") or re.match(r"^[A-Za-z].*\)\s*$", line):
            continue
        port[(cur, "play")] += len(re.findall(r"\bplaySound\(", line))
        for n in NAMES:
            if n in ("pause", "pauseMusicOnly", "resume", "update", "shutdown", "play") and not insound:
                port[(cur, n)] += len(re.findall(r"\bgSound->" + n + r"\(", line))
            else:
                port[(cur, n)] += len(re.findall(r"(?:\bgSound->|SoundSystem::|(?<![\w.>])" + ("" if insound else "(?!x)") + r")" + n + r"\(", line))
addrs = sorted(addrs)
orig = collections.Counter()
for line in open("re/bonnies/game_disasm.txt"):
    m = re.match(r"\s+([0-9a-f]+):\s+calll\s+0x([0-9a-f]+)\b", line)
    if not m or int(m.group(2), 16) not in CALLEES:
        continue
    site = int(m.group(1), 16)
    if 0x40dd00 <= site < 0x40dd63 or 0x417b20 <= site < 0x417b40:  # inside the two helpers
        continue
    i = bisect.bisect_right(addrs, site) - 1
    fn = addrs[i] if i >= 0 else 0
    orig[(fn, CALLEES[int(m.group(2), 16)])] += 1
keys = sorted(k for k in set(orig) | set(port) if orig[k] or port[k])
bad = 0
for k in keys:
    if orig[k] != port[k]:
        bad += 1
        print(f"{k[0]:#x} {k[1]}: original {orig[k]}, port {port[k]}")
print(f"functions with sound calls: {len({k[0] for k in keys})}; original sites {sum(orig.values())}, port sites {sum(port.values())}; differing {bad}")
