"""toorig.py <port script> <origrun script>: a port script ("<update> <command>", 10 ms of game time per update) as
an origrun script for the virtual clock, each command after a wait of the game time since the one before (origrun's
`wait` counts game time from its hold). Screenshots are left out (phase one compares state)."""
import sys

KEYS = {"BACK": 8, "RETURN": 13, "ESCAPE": 27, "SPACE": 32, "LEFT": 37, "UP": 38, "RIGHT": 39, "DOWN": 40}
prev = 0
out = []
for line in open(sys.argv[1]):
	f = line.split()
	if not f or f[0].startswith("#"):
		continue
	u, cmd, args = int(f[0]), f[1], f[2:]
	if cmd == "screenshot":
		continue
	# origrun holds the game as it enters the sleep that would take its clock to the wait's end, before that sleep
	# moves the clock, so the game reads the next input 1 ms before the end (the trial's keys came 199 ms apart on
	# 200 ms waits): each wait but the first (which only lines up the start) is 1 ms longer
	if u > prev:
		out.append("wait %d" % ((u - prev) * 10 + (1 if prev else 0)))
	prev = u
	if cmd in ("click", "rclick", "move"):
		out.append("%s %s %s" % (cmd, args[0], args[1]))
	elif cmd == "char":
		out.append("text " + args[0])
	elif cmd == "key":
		out.append("key %d" % KEYS[args[0]])
	elif cmd == "quit":
		out.append("quit")
	else:
		sys.exit("toorig: no origrun form for: " + line.strip())
open(sys.argv[2], "w", newline="").write("\n".join(out) + "\n")
