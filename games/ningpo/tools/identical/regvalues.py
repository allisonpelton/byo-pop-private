"""regvalues.py reg|ini|toreg|toini: Ning Po's saved values between their three forms. "reg" reads `reg query`'s
output and "ini" the port's registry.ini, each from stdin, and both print "<name>=<hex>" lines in name order, for
the game's own values only. "toreg" turns such lines into "<name><tab><hex>" for `reg add`, and "toini" into the port's
registry.ini."""
import sys

GAME = ("Autohint", "Background", "Fullscreen", "Layout", "Music Volume", "Sound Volume", "Tileset")

def game_value(name):
	return name in GAME or name.startswith("HS:")

mode = sys.argv[1]
values = {}
if mode == "reg":
	for line in sys.stdin:
		f = line.strip().split("    ")
		if len(f) == 3 and f[1] == "REG_BINARY" and game_value(f[0]):
			values[f[0]] = f[2].lower()
elif mode == "ini":
	for line in sys.stdin:
		line = line.strip()
		if "=hex:" in line:
			name, data = line.split("=hex:", 1)
			if game_value(name):
				values[name] = data.lower()
else:
	for line in sys.stdin:
		name, data = line.strip().split("=", 1)
		values[name] = data
	if mode == "toreg":
		for name in sorted(values):
			print("%s\t%s" % (name, values[name]))
	else:
		print("; Settings (emulated Windows registry, HKEY_CURRENT_USER)\n\n[SOFTWARE\\Pyrogon\\NingPo]")
		for name in sorted(values):
			print("%s=hex:%s" % (name, values[name]))
	sys.exit(0)
for name in sorted(values):
	print("%s=%s" % (name, values[name]))
