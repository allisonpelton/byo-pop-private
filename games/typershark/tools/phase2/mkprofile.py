#!/usr/bin/env python3
"""mkprofile.py <ngc> [name]: a player Tester with a finished adventure, the same in both programs, for the medal screens
(GAME_NOTES section 18). The statistics screen's adventure panel and the end-of-adventure dialog draw a medal for the number of finished
adventures (NGC: 1 or 2 bronze, 3 or 4 silver, 5 or more gold); a profile with NGC set is the way to reach the panel without playing.

    python mkprofile.py 3        writes  build/ts-p1/fixtures/medal-ngc3        the port's user folder (registry.ini, users/)
                                         build/ts-p2/profiles/ngc3/orig.reg     the original's registry preset (--reg, sandbox)
                                         build/ts-p2/profiles/ngc3/orig.users/  its users folder (STAGE=, the same two files)
    python mkprofile.py 3 [saved] 3d   the port's fixture with Is3D on (named medal-ngc3-3d, medal-sv3-3d), for a 3D comparison
    python mkprofile.py 3 saved  the same with the folder of a saved adventure at the last site (mksaved.sh): the profiles are
                                 named medal-sv3 and sv3. The menu's Adventure then asks to continue the game, and YES goes to
                                 the map's final scroll and the end-of-adventure dialog (no adventure to play through)

The values are the registry values the game itself writes for a player (UserProfileMgr::WriteUserProfileToRegistry, the
original's 0x41f1d0): every field of the statistics screen is set to a distinct number, so that a difference between the programs
is seen by its digits. The two files of the users folder are those the port wrote for a Tester that played a tutor lesson
(a player's data file and the score table), the format the original reads (the restart tour carries them both ways).
"""
import os
import shutil
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", "..", ".."))

# (name, kind, value): kind "s" is a string value, "d" a dword
PROFILE = [
    ("Version", "s", "1.02"), ("UserNumber", "s", "0"), ("Seed", "s", "1163206238"), ("AnnoDomini", "s", "2650"),
    ("NumSharksKilled", "s", "1234"), ("NumPiranhasKilled", "s", "567"), ("NumBossesKilled", "s", "8"), ("LastLesson", "s", "1"),
    ("NumAbyssSharksKilled", "s", "321"), ("NumAbyssPiranhasKilled", "s", "45"), ("NBTL", "s", "-1"), ("SpacePauses", "d", "0"),
    ("PKTR", "s", "21"), ("SKTR", "s", "156"),
    ("WPM0", "s", "41"), ("WPM1", "s", "47"), ("WPM2", "s", "52"), ("WPM3", "s", "0"), ("WPM4", "s", "0"), ("WPM5", "s", "0"),
    ("WPM6", "s", "0"), ("WPM7", "s", "0"), ("WPM8", "s", "0"), ("WPM9", "s", "0"), ("LastWPMIndex", "s", "3"),
    ("SeenTutorial", "s", "1"), ("AbyssBoss", "s", "-1"), ("NGC", "s", "{ngc}"), ("RNGC", "d", "1"),
    ("GTT0", "s", "3"), ("GTT1", "s", "2"), ("GTT2", "s", "4"), ("GTT3", "s", "1"), ("GTT4", "s", "5"), ("GTT5", "s", "2"),
    ("SGTT0", "s", "1"), ("SGTT1", "s", "1"), ("SGTT2", "s", "1"), ("SGTT3", "s", "1"),
] + [("vl%d" % i, "d", "0") for i in range(36)] + [("vd", "s", "0"), ("svm", "d", "0")]

# the settings the original's own run A wrote (a restart tour's export), so that the second start is not a first run
SETTINGS_ORIG = [("InProgress", 0), ("MusicVolume", 0x55), ("SfxVolume", 0x55), ("Muted", 0), ("ScreenMode", 0), ("CustomCursors", 0),
                 ("ITSuccess", 0), ("TimesPlayed", 0), ("TimesExecuted", 1), ("WaitForVSync", 0)]
SETTINGS_PORT = [("ScreenMode", 0), ("Is3D", 0), ("MusicVolume", 85), ("SfxVolume", 85), ("Muted", 0), ("CustomCursors", 0),
                 ("InProgress", 0), ("TimesExecuted", 1), ("TimesPlayed", 0), ("WaitForVSync", 0)]


# the saved game's totals that the end-of-adventure dialog shows (the player's score, the typing time, the letters): set to distinct
# numbers, so that a digit that differs between the programs is seen (a saved adventure made by the port has none of them)
SAVED_SCORE, SAVED_GAME_TIME, SAVED_WRONG, SAVED_RIGHT, SAVED_TYPING_TIME = 47250, 754321, 37, 412, 305000


def set_saved_totals(path):
    """users/user0.dat is a list of numbers, each a two-byte length and its text (UserProfile::WriteDataToDisk). The saved game of
    mksaved (level 35 at the Normal difficulty, lives 2, score 4000, no letters yet) is found by its numbers: lives, score, the
    diver's charge, the level, the difficulty, the level number, the game time, the paused time, the wrong and right letters."""
    data = open(path, "rb").read()
    toks, pos = [], 0
    while pos < len(data):
        n = struct.unpack_from("<H", data, pos)[0]
        toks.append(data[pos + 2:pos + 2 + n].decode("latin1"))
        pos += 2 + n
    j = next(i for i in range(len(toks) - 6) if toks[i:i + 6] == ["2", "4000", "0", "35", "1", "2"])
    toks[j + 1] = str(SAVED_SCORE)
    toks[j + 6] = str(SAVED_GAME_TIME)
    toks[j + 8] = str(SAVED_WRONG)
    toks[j + 9] = str(SAVED_RIGHT)
    toks[-5] = str(SAVED_TYPING_TIME)          # the typing time comes before the four kill counts
    open(path, "wb").write(b"".join(struct.pack("<H", len(t)) + t.encode("latin1") for t in toks))


def main():
    ngc = int(sys.argv[1])
    saved = "saved" in sys.argv[2:]
    three_d = "3d" in sys.argv[2:]
    src_users = os.path.join(ROOT, "build", "ts-p2", "mk1" if saved else "g_tut18", "ud", "users")
    tag = ("sv%d" if saved else "ngc%d") % ngc
    port_dir = os.path.join(ROOT, "build", "ts-p1", "fixtures", "medal-" + tag + ("-3d" if three_d else ""))
    prof_dir = os.path.join(ROOT, "build", "ts-p2", "profiles", tag)
    for d in (port_dir, prof_dir):
        shutil.rmtree(d, ignore_errors=True)
    os.makedirs(os.path.join(port_dir, "users"))
    os.makedirs(os.path.join(prof_dir, "orig.users"))
    for f in ("user.idx", "user0.dat"):
        shutil.copy(os.path.join(src_users, f), os.path.join(port_dir, "users", f))
        shutil.copy(os.path.join(src_users, f), os.path.join(prof_dir, "orig.users", f))
    if saved:
        for d in (port_dir, prof_dir):
            set_saved_totals(os.path.join(d, "users" if d == port_dir else "orig.users", "user0.dat"))

    # the port's emulated registry (registry.ini)
    ini = ["; Settings (emulated Windows registry, HKEY_CURRENT_USER)", "", "[SOFTWARE\\PopCap\\TyperShark]"]
    ini += ["%s=dword:%d" % ((k, 1 if three_d and k == "Is3D" else v)) for k, v in SETTINGS_PORT]
    ini += ["", "[SOFTWARE\\PopCap\\TyperShark\\Users]", "User=sz:Tester", "", "[SOFTWARE\\PopCap\\TyperShark\\Users\\Tester]"]
    for name, kind, val in PROFILE:
        val = val.format(ngc=ngc)
        ini.append("%s=%s:%s" % (name, "sz" if kind == "s" else "dword", val))
    open(os.path.join(port_dir, "registry.ini"), "w", newline="\n").write("\n".join(ini) + "\n")

    # the original's registry preset (a regedit export: UTF-16 with a byte order mark, CR LF)
    reg = ["Windows Registry Editor Version 5.00", "", "[HKEY_LOCAL_MACHINE\\SOFTWARE\\PopCap]", "",
           "[HKEY_LOCAL_MACHINE\\SOFTWARE\\PopCap\\TyperShark]"]
    reg += ['"%s"=dword:%08x' % kv for kv in SETTINGS_ORIG]
    reg += ["", "[HKEY_LOCAL_MACHINE\\SOFTWARE\\PopCap\\TyperShark\\Users]", '"User"="Tester"', "",
            "[HKEY_LOCAL_MACHINE\\SOFTWARE\\PopCap\\TyperShark\\Users\\Tester]"]
    for name, kind, val in PROFILE:
        val = val.format(ngc=ngc)
        reg.append('"%s"=%s' % (name, '"%s"' % val if kind == "s" else "dword:%08x" % int(val)))
    text = "\r\n".join(reg) + "\r\n\r\n"
    open(os.path.join(prof_dir, "orig.reg"), "wb").write(b"\xff\xfe" + text.encode("utf-16-le"))
    print("%s: %s, %s" % (tag, port_dir, os.path.join(prof_dir, "orig.reg")))


if __name__ == "__main__":
    main()
