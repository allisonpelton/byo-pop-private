#!/usr/bin/env python3
"""A local page for what only the user's ears can judge, development only: it is never part of the site's build. Three lists:

  Pairs   pairs of similar sounds (the user, 2026-11-12: "Make a page where I can compare the similar sounds and press a
          button to decide if two sounds are similar")
  Voices  the voice clips whose words neither the file name nor the port's code settles (the user, 2026-11-12: "Add unsure
          sounds and anything else that needs review to that page"; Whisper's guess shown only as a hint, since "It looks
          less accurate than just having me correct inferences from the file name")
  Seams   the seams of the tunes Music joins from pieces or cuts where the game starts another tune (TO_TEST 14i): a few
          seconds either side of each, rendered as the site plays them (tools/web/seamrender.js for modules)

The pairs come from tools/soundmatch.py's results in build/soundmatch/: its near matches first, then the closest pairs it
turned down (turned.json, by the mean of its two scores) as a second list. Each sound is decoded once by ffmpeg to a PCM WAV
at its own rate under build/soundcompare/snd/ (git-ignored): a board sound from the staged files (tools/voicelines.py's
reader), an unported game's from the file soundmatch found in the user's install, which is only read. The decisions are
written on every click to testnotes/AH-decisions.json, voicelines-decisions.json and seams-decisions.json, and the page reads
them back on a reload.

  python tools/web/soundcompare.py [--assets DIR] [--match DIR] [--unsure FILE] [--port 18791] [--missed 24]
  then open http://127.0.0.1:18791/
"""
import argparse
import http.server
import json
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import voicelines as vl  # noqa: E402

MATCH = os.path.join(ROOT, "build", "soundmatch")
OUT = os.path.join(ROOT, "build", "soundcompare")
SND = os.path.join(OUT, "snd")
DECISIONS = {"pairs": os.path.join(ROOT, "testnotes", "AH-decisions.json"),
             "voices": os.path.join(ROOT, "testnotes", "voicelines-decisions.json"),
             "seams": os.path.join(ROOT, "testnotes", "seams-decisions.json")}
UNSURE = os.path.join(ROOT, "testnotes", "voicelines-unsure.txt")
NODE = "E:/retools/emsdk/node/24.19.0_64bit/node.exe"
OPENMPT = os.path.join(ROOT, "build", "web", "openmpt.js")
RATE = 44100
AROUND = 4   # seconds heard either side of a seam

# The voice clips left open once the file names, the ports' code (the boards' descriptions) and the series a file belongs to
# have been read (the rest of testnotes/voicelines-unsure.txt is settled: words the file name spells, or a creature's or a
# person's cry the name describes). Each: the games that ship the file, the file, the label proposed (a quoted label is
# words), why, and whether Whisper's guess is worth showing as a hint.
VOICES = [
    (["bejeweled"], "sounds/levelup1.ogg", '"Level up!"', "File name; played as the level-won vortex closes", False),
    (["bejeweled"], "sounds/welcome1.ogg", '"Welcome!"', "File name only; never played (unused)", True),
    (["bejeweled2"], "sounds/Welcome.ogg", '"Welcome"', "File name; Whisper hears more words", True),
    (["bookwormadv", "bookwormadv2"], "sounds/heal2.ogg", "Health potion", "Plays as Lex drinks a health potion; words?", True),
    (["bookwormadv", "bookwormadv2"], "sounds/kubwa_greeting.ogg", "Kubwa greeting", "A thief's greeting in the Arena; words?", True),
    (["bookwormadv", "bookwormadv2"], "sounds/kubwa_greeting2.ogg", "Kubwa greeting 2", "Thieves 12-14 enter; words?", True),
    (["bookwormadv", "bookwormadv2"], "sounds/kubwa_barh.ogg", "Kubwa barh", "A thief's attack or hit; words?", True),
    (["bookwormadv", "bookwormadv2"], "sounds/codex_nyah.ogg", "Codex nyah", "The Codex taunts in the last chapter", False),
    (["bookwormadv2"], "sounds/timehelmet_reverb.ogg", "Time Helmet effect", "The Time Helmet's effect; a voice in it?", True),
    (["bookwormadv2"], "sounds/robot-x.ogg", '"X!"', "One step of Lex's dance in the book 6 dance battle", True),
    (["bookwormadv2"], "sounds/roboticattack1.ogg", "Robotic attack 1", "A robot enemy's attack or hit; words?", True),
    (["dynomite"], "sounds/xwhirleyscoming1.ogg", '"Whirleys coming!"', "File name; Endless, 2 seconds before the hourglass runs out", False),
    (["dynomite"], "sounds/xwhirleyscoming2.ogg", '"Whirleys coming!"', "File name; the other call, every second hourglass", True),
    (["noahsark"], "sounds/levelupvoice.ogg", '"Level up!"', "File name; the next-level screen opens", False),
    (["noahsark"], "sounds/abandon.ogg", "Abandon", "A pair abandoned when no move is left; words?", True),
    (["noahsark"], "sounds/winhippo.ogg", "Hippo voice", "A hippo pair is saved; a call or words?", True),
    (["sevenseas"], "sounds/talklandlubber.ogg", '"Landlubber!"', "File name; one of four pirate lines at a level's end", True),
    (["tiptop"], "TipTop.dat|tiptop:sounds/MouseClick.ogg", "Mouse click", "Any button press; Whisper hears words", True),
    (["tiptop"], "TipTop.dat|tiptop:sounds/VolumeAdjust.ogg", "Volume test", "The sound slider moves; Whisper hears words", True),
    (["tiptop"], "TipTop.dat|tiptop:sounds/ColorMatchGroup5.ogg", "Match of 5", "A match of five starts a chain; a voice?", True),
    (["tiptop"], "TipTop.dat|tiptop:sounds/ColorMatchGroup6.ogg", "Match of 6 or more", "A match of six or more starts a chain; a voice?", True),
]
TITLES = {"alchemy": "Alchemy", "astropop": "AstroPop", "atomica": "Atomica", "bejeweled": "Bejeweled", "bejeweled2": "Bejeweled 2",
          "bejeweledtwist": "Bejeweled Twist", "bigmoney": "Big Money", "bonnies": "Bonnie's Bookstore", "bookworm": "BookWorm",
          "bookwormadv": "Bookworm Adventures", "bookwormadv2": "Bookworm Adventures Volume 2", "cosmicbugs": "Cosmic Bugs",
          "dynomite": "Dynomite", "mahjongchina": "Mahjong Escape: Ancient China", "mahjongjapan": "Mahjong Escape: Ancient Japan",
          "mummymaze": "Mummy Maze", "ningpo": "Ning Po Mahjong", "noahsark": "Noah's Ark", "pixelus": "Pixelus",
          "rocketmania": "Rocket Mania", "sevenseas": "Seven Seas", "tiptop": "TipTop", "typershark": "Typer Shark",
          "waterbugs": "Water Bugs", "wordharmony": "Word Harmony",
          "Bejeweled3": "Bejeweled 3", "ChuzzleDeluxe": "Chuzzle Deluxe", "FeedingFrenzy2Deluxe": "Feeding Frenzy 2 Deluxe",
          "HammerHeadsDeluxe": "Hammer Heads Deluxe", "HeavyWeaponDeluxe": "Heavy Weapon Deluxe",
          "IgglePopDeluxe": "Iggle Pop Deluxe", "InsaniquariumDeluxe": "Insaniquarium Deluxe", "PeggleDeluxe": "Peggle Deluxe",
          "PeggleExtreme": "Peggle Extreme", "PeggleNights": "Peggle Nights", "PeggleWoW": "Peggle World of Warcraft Edition",
          "PizzaFrenzy": "Pizza Frenzy", "PlantsVsZombies": "Plants vs. Zombies", "TalismaniaDeluxe": "Talismania Deluxe",
          "TheWizardsPen": "The Wizard's Pen", "ZumaDeluxe": "Zuma Deluxe", "ZumasRevenge": "Zuma's Revenge",
          "Venice": "Venice", "Gyromancer": "Gyromancer", "BananaBugs": "Banana Bugs"}


def key(side):
    return side["game"] + ":" + side["file"]


def decode(side, n):
    """the sound as a PCM WAV the browser plays, made once; its file name under snd/, or None"""
    name = "%03d-%s.wav" % (n, vl.safe(side["game"] + "_" + side["file"])[-80:])
    dst = os.path.join(SND, name)
    if os.path.exists(dst):
        return name
    os.makedirs(SND, exist_ok=True)
    src, tmp = side.get("src"), None
    if not src:
        data, ext = vl.sound_bytes(side["game"], side["file"])
        if data is None:
            return None
        tmp = dst[:-4] + ".src" + (ext or ".bin")
        open(tmp, "wb").write(data)
        src = tmp
    r = subprocess.run([vl.FFMPEG, "-v", "error", "-y", "-i", src, "-c:a", "pcm_s16le", dst], capture_output=True)
    if tmp:
        os.remove(tmp)
    return name if r.returncode == 0 and os.path.exists(dst) else None


def prepare(missed):
    boards = vl.boards()
    labels = {(g, f): l for g, rows in boards.items() for f, l, w in rows}
    near = [h for h in json.load(open(os.path.join(MATCH, "matches.json"), encoding="utf-8")) if h["kind"] == "near"]
    listed = {frozenset((key(h["a"]), key(h["b"]))) for h in json.load(open(os.path.join(MATCH, "matches.json"), encoding="utf-8"))}
    # a pair whose scores equal an earlier one's is the same two recordings again (the four Peggles' button, two games'
    # identical click): only the first is listed
    turned, scores = [], set()
    for h in json.load(open(os.path.join(MATCH, "turned.json"), encoding="utf-8")):
        if frozenset((key(h["a"]), key(h["b"]))) in listed or (h["env"], h["wave"]) in scores or len(turned) >= missed:
            continue
        scores.add((h["env"], h["wave"]))
        turned.append(h)
    pairs, n = [], 0
    for lst, hs in (("near", near), ("missed", turned)):
        for h in hs:
            row = {"list": lst, "env": h["env"], "wave": h["wave"]}
            for s in ("a", "b"):
                side = h[s]
                n += 1
                row[s] = {"game": TITLES.get(side["game"], side["game"]), "id": side["game"], "file": side["file"],
                          "label": labels.get((side["game"], side["file"]), "") if side["game"] in boards else "",
                          "snd": decode(side, n)}
            row["key"] = key(h["a"]) + "|" + key(h["b"])
            pairs.append(row)
    return pairs


def prepare_voices(n):
    """the open voice clips: [{key, games, file, label, proposal, evidence, hint, snd}]"""
    boards = vl.boards()
    labels = {(g, f): l for g, rows in boards.items() for f, l, w in rows}
    heard = {}
    if os.path.exists(UNSURE):
        for line in open(UNSURE, encoding="utf-8"):
            f = line.rstrip("\n").split("\t")
            if len(f) >= 4 and not line.startswith("#"):
                heard[(f[0], f[1])] = f[3].split(" (")[0].strip()
    rows = []
    for games, ref, proposal, evidence, hint in VOICES:
        n += 1
        side = {"game": games[0], "file": ref}
        rows.append({"key": games[0] + ":" + ref, "games": [TITLES.get(g, g) for g in games], "ids": games, "file": ref,
                     "label": labels.get((games[0], ref), ""), "proposal": proposal, "evidence": evidence,
                     "hint": heard.get((games[0], ref), "") if hint else "", "snd": decode(side, n)})
    return rows


def pcm(path):
    """a sound file as float32 interleaved stereo at RATE (numpy), through ffmpeg"""
    import numpy as np
    r = subprocess.run([vl.FFMPEG, "-v", "error", "-i", path, "-f", "f32le", "-ac", "2", "-ar", str(RATE), "-"], capture_output=True)
    return np.frombuffer(r.stdout, dtype=np.float32)


def write_wav(path, a):
    import numpy as np
    import wave
    w = wave.open(path, "wb")
    w.setnchannels(2)
    w.setsampwidth(2)
    w.setframerate(RATE)
    w.writeframes((np.clip(a, -1, 1) * 32767).astype("<i2").tobytes())
    w.close()


def soundtracks():
    """web/soundtracks.js's table, through node"""
    js = ("const vm=require('vm'),fs=require('fs');const c={window:{}};vm.createContext(c);"
          "vm.runInContext(fs.readFileSync(process.argv[1],'utf8'),c);"
          "const T=Object.values(c).find(v=>v&&typeof v=='object'&&v.noahsark);process.stdout.write(JSON.stringify(T))")
    r = subprocess.run([NODE, "-e", js, os.path.join(ROOT, "web", "soundtracks.js")], capture_output=True, text=True, encoding="utf-8")
    return json.loads(r.stdout)


def playlist(path):
    """the words of a playlist's lines (web/music.js: three header lines, then a piece a line)"""
    lines = [l for l in open(path, encoding="utf-8", errors="replace").read().splitlines() if l.strip()]
    return [l.split()[0] for l in lines[3:]]


def prepare_seams(per_playlist=6):
    """the seams of joined and cut tunes: [{key, game, tune, what, snd, at}]; each WAV holds AROUND seconds either side"""
    os.makedirs(os.path.join(OUT, "seams"), exist_ok=True)
    table, rows, jobs = soundtracks(), [], []

    def seam_name(*parts):
        return vl.safe("_".join(str(p) for p in parts))[-90:] + ".wav"
    # cut subsongs and chained parts of modules: rendered by the site's own player
    for game, e in sorted(table.items()):
        mods = {}
        for pat in e.get("files", []):
            folder, glob_ = os.path.split(pat)
            base = os.path.join(vl.ASSETS, game, folder)
            if os.path.isdir(base):
                import fnmatch
                for f in os.listdir(base):
                    if fnmatch.fnmatch(f.lower(), glob_.lower()):
                        mods[os.path.splitext(f)[0].lower()] = os.path.join(base, f)
        for key, orders in sorted((e.get("cuts") or {}).items()):
            name, sub = key.rsplit("#", 1)
            src = mods.get(name.lower())
            for order in orders:
                out = seam_name(game, name, sub, "cut", order)
                if src:
                    jobs.append({"out": os.path.join(OUT, "seams", out), "file": src, "kind": "cut", "sub": int(sub), "order": order, "around": AROUND})
                rows.append({"snd_was": out, "key": "%s:%s@%d" % (game, key, order), "game": TITLES.get(game, game), "tune": key,
                             "what": "Cut at order %d, where the game starts another tune" % order,
                             "snd": out if src else None})
        for key, chain in sorted((e.get("chains") or {}).items()):
            name = key.rsplit("#", 1)[0]
            src = None
            for ins in e.get("inside", []):
                p = vl.staged(game, ins["file"])
                if p and name.startswith("Resource "):
                    data = vl.pe_resource(open(p, "rb").read(), "WAVE", name.split()[1])
                    if data:
                        src = os.path.join(OUT, "seams", vl.safe(game + "_" + name) + ".mo3")
                        open(src, "wb").write(data)
            parts = [key] + chain
            for a, b in zip(parts, parts[1:]):
                sa, sb = int(a.rsplit("#", 1)[1]), int(b.rsplit("#", 1)[1])
                out = seam_name(game, name, sa, "to", sb)
                if src:
                    jobs.append({"out": os.path.join(OUT, "seams", out), "file": src, "kind": "chain", "a": sa, "b": sb, "around": AROUND})
                rows.append({"key": "%s:%s>%s" % (game, a, b), "game": TITLES.get(game, game), "tune": key,
                             "what": "Joined: the end of part %d runs into part %d" % (sa + 1, sb + 1), "snd": out if src else None})
        # joined pieces of Ogg and WAV files: put one after the other, as Music does
        for j in e.get("joined", []):
            if "pieces" in j:
                seq, tune = list(j["pieces"]), j["id"]
                loop_from = j.get("loopFrom")
            else:
                pl = vl.staged(game, j["playlist"])
                if not pl:
                    continue
                seq = [j["piece"].replace("{}", w.lower()) for w in playlist(pl)]
                tune, loop_from = os.path.basename(j["playlist"]).rsplit(".", 1)[0], None
            paths = [vl.staged(game, r) for r in seq]
            if not all(paths):
                continue
            pcms = {}
            for p in set(paths):
                pcms[p] = pcm(p)
            import numpy as np
            whole = np.concatenate([pcms[p] for p in paths])
            starts, at = [], 0
            for p in paths:
                starts.append(at)
                at += len(pcms[p]) // 2
            seen, picked = set(), []
            for i in range(1, len(seq)):
                pair = (seq[i - 1], seq[i])
                if pair not in seen:
                    seen.add(pair)
                    picked.append(i)
            if "playlist" in j:
                picked = picked[:per_playlist]
            w = AROUND * RATE
            for i in picked:
                cut = starts[i]
                clip = whole[max(0, cut - w) * 2: (cut + w) * 2]
                out = seam_name(game, tune, i)
                write_wav(os.path.join(OUT, "seams", out), clip)
                rows.append({"key": "%s:%s#%d" % (game, tune, i), "game": TITLES.get(game, game), "tune": tune,
                             "what": "Joined: %s into %s" % (os.path.basename(seq[i - 1]), os.path.basename(seq[i])),
                             "snd": out, "at": min(cut, w) / RATE})
            if loop_from is not None:
                last = pcms[paths[-1]]
                clip = np.concatenate([last[-w * 2:], np.concatenate([pcms[p] for p in paths[loop_from:]])[:w * 2]])
                out = seam_name(game, tune, "loop")
                write_wav(os.path.join(OUT, "seams", out), clip)
                rows.append({"key": "%s:%s#loop" % (game, tune), "game": TITLES.get(game, game), "tune": tune,
                             "what": "Repeat: the end of %s goes back to %s" % (os.path.basename(seq[-1]), os.path.basename(seq[loop_from])),
                             "snd": out, "at": min(len(last) // 2, w) / RATE})
    if jobs:
        jf = os.path.join(OUT, "seams", "jobs.json")
        json.dump(jobs, open(jf, "w", encoding="utf-8"))
        r = subprocess.run([NODE, os.path.join(HERE, "seamrender.js"), OPENMPT, jf], capture_output=True, text=True)
        done = {}
        for line in r.stdout.splitlines():
            f = line.split()
            if len(f) >= 4 and f[-1] == "ok":
                done[os.path.basename(f[0])] = float(f[2])
        for row in rows:
            if row["snd"] and "at" not in row:
                if row["snd"] in done:
                    row["at"] = done[row["snd"]]
                else:
                    row["snd"] = None
        # a cut at an order the subsong never reaches makes no seam: the later piece starts there on its own (web/soundtracks.js)
        never = {os.path.basename(l.split()[0]) for l in r.stdout.splitlines() if "never reached" in l}
        rows = [row for row in rows if row.get("snd") is not None or row.get("snd_was") not in never]
    return rows


def prepare_all(missed):
    items = {"pairs": prepare(missed)}
    items["voices"] = prepare_voices(1000)
    items["seams"] = prepare_seams()
    json.dump(items, open(os.path.join(OUT, "items.json"), "w", encoding="utf-8"), indent=1, ensure_ascii=False)
    return items


class Handler(http.server.SimpleHTTPRequestHandler):
    def log_message(self, *a):
        pass

    def send(self, code, body, ctype):
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        path = self.path.split("?")[0]
        if path in ("/", "/index.html"):
            return self.send(200, open(os.path.join(HERE, "soundcompare.html"), "rb").read(), "text/html; charset=utf-8")
        if path == "/items.json":
            return self.send(200, open(os.path.join(OUT, "items.json"), "rb").read(), "application/json")
        if path.startswith("/decisions/") and path[11:] in DECISIONS:
            f = DECISIONS[path[11:]]
            body = open(f, "rb").read() if os.path.exists(f) else b"{}"
            return self.send(200, body, "application/json")
        for prefix, folder in (("/snd/", SND), ("/seam/", os.path.join(OUT, "seams"))):
            if path.startswith(prefix):
                p = os.path.join(folder, os.path.basename(path))
                if os.path.isfile(p) and p.endswith(".wav"):
                    return self.send(200, open(p, "rb").read(), "audio/wav")
        self.send(404, b"not found", "text/plain")

    def do_POST(self):
        which = self.path[11:] if self.path.startswith("/decisions/") else ""
        if which not in DECISIONS:
            return self.send(404, b"not found", "text/plain")
        body = self.rfile.read(int(self.headers.get("Content-Length", 0)))
        data = json.loads(body.decode("utf-8"))
        if not isinstance(data, dict):
            return self.send(400, b"expected an object", "text/plain")
        f = DECISIONS[which]
        os.makedirs(os.path.dirname(f), exist_ok=True)
        tmp = f + ".tmp"
        open(tmp, "w", encoding="utf-8").write(json.dumps(data, indent=1, ensure_ascii=False, sort_keys=True))
        os.replace(tmp, f)
        self.send(200, b"ok", "text/plain")

def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--assets", help="staged game files (default: this checkout's assets/)")
    ap.add_argument("--port", type=int, default=18791)
    ap.add_argument("--missed", type=int, default=24, help="how many of the closest turned-down pairs to list")
    ap.add_argument("--match", help="tools/soundmatch.py's results (default: this checkout's build/soundmatch/)")
    ap.add_argument("--unsure", help="the unsure voice clips with Whisper's guesses (default: testnotes/voicelines-unsure.txt)")
    ap.add_argument("--prepare-only", action="store_true")
    a = ap.parse_args()
    if a.assets:
        vl.ASSETS = os.path.abspath(a.assets)
    if a.match:
        global MATCH
        MATCH = os.path.abspath(a.match)
    if a.unsure:
        global UNSURE
        UNSURE = os.path.abspath(a.unsure)
    items = prepare_all(a.missed)
    pairs = items["pairs"]
    print("pairs %d (near %d, missed %d), %d sounds not decoded; voices %d (%d not decoded); seams %d (%d not rendered)" % (
        len(pairs), sum(p["list"] == "near" for p in pairs), sum(p["list"] == "missed" for p in pairs),
        sum(not p[s]["snd"] for p in pairs for s in "ab"), len(items["voices"]), sum(not v["snd"] for v in items["voices"]),
        len(items["seams"]), sum(not v["snd"] for v in items["seams"])), flush=True)
    if a.prepare_only:
        return
    print("http://127.0.0.1:%d/" % a.port, flush=True)
    http.server.ThreadingHTTPServer(("127.0.0.1", a.port), Handler).serve_forever()


if __name__ == "__main__":
    main()
