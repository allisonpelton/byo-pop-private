"""mksaves.py <out dir>: the saved games the coverage runs start from (run.sh's SAVE; docs/COVERAGE.md). The port writes the
original's savegame format, so each is made by the port's own test commands (docs/TEST_COMMANDS.md): a game on a level with a
score and lives set, left by the panel's Exit button and its dialog's Yes, which saves it. Four files:
bonus.dat (Average, level 10: Treasure Island), lose.dat (Average, level 3, one life), l9.dat (Average, level 9, three lives:
the crimson pirates fire) and l9empty.dat (the same with no ship left, so that the level is won at once: the next level is the
bonus level, and the map after it). Run it from the worktree root."""
import os, struct, subprocess, sys
out = sys.argv[1]
os.makedirs(out, exist_ok=True)
def make(name, mode, level, score, lives, warning_click):
    script = os.path.join(out, "mk_%s.txt" % name)
    open(script, "w").write("\n".join([
        "200 click 320 385", "300 goto %s" % mode, "1900 level %d" % level, "2000 set score %d" % score,
        "2100 set lives %d" % lives, "2200 click %s" % warning_click, "2250 state", "2300 click 87 441", "2400 click 330 200",
        "2500 state", "2600 quit"]) + "\n")
    ud = os.path.join(out, "ud_" + name)
    subprocess.run(["bash", "games/sevenseas/tools/run_script.sh", script, ud, "90"], check=True, stdout=subprocess.DEVNULL)
    return open(os.path.join(ud, "savegame.dat"), "rb").read()
# the dialog that is up when the level starts: the Treasure Island nag (button at 405,391) or the whirlpool warning (405,321)
bonus = make("bonus", "average", 10, 5000, 3, "405 391")
lose = make("lose", "average", 3, 5000, 1, "405 321")
l9 = make("l9", "average", 9, 5000, 3, "405 321")
n = struct.unpack_from("<i", l9, 20)[0]                      # the ship count follows the score and the kill count
empty = l9[:20] + struct.pack("<i", 0) + l9[24 + 16 * n:]    # every ship record is 4 ints
for name, data in (("bonus", bonus), ("lose", lose), ("l9", l9), ("l9empty", empty)):
    open(os.path.join(out, name + ".dat"), "wb").write(data)
