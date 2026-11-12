#!/usr/bin/env python3
"""A local page for judging pairs of similar sounds by ear (the user, 2026-11-12: "Make a page where I can compare the similar
sounds and press a button to decide if two sounds are similar"). Development only: it is never part of the site's build.

The pairs come from tools/soundmatch.py's results in build/soundmatch/: its near matches first, then the closest pairs it
turned down (turned.json, by the mean of its two scores) as a second list. Each sound is decoded once by ffmpeg to a PCM WAV
at its own rate under build/soundcompare/snd/ (git-ignored): a board sound from the staged files (tools/voicelines.py's
reader), an unported game's from the file soundmatch found in the user's install, which is only read. The decisions are
written to testnotes/AH-decisions.json on every click, and the page reads them back on a reload.

  python tools/web/soundcompare.py [--assets DIR] [--match DIR] [--port 18791] [--missed 24]
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
DECISIONS = os.path.join(ROOT, "testnotes", "AH-decisions.json")
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
    json.dump(pairs, open(os.path.join(OUT, "pairs.json"), "w", encoding="utf-8"), indent=1, ensure_ascii=False)
    return pairs


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
        if path == "/pairs.json":
            return self.send(200, open(os.path.join(OUT, "pairs.json"), "rb").read(), "application/json")
        if path == "/decisions":
            body = open(DECISIONS, "rb").read() if os.path.exists(DECISIONS) else b"{}"
            return self.send(200, body, "application/json")
        if path.startswith("/snd/"):
            name = os.path.basename(path)
            p = os.path.join(SND, name)
            if os.path.isfile(p):
                return self.send(200, open(p, "rb").read(), "audio/wav")
        self.send(404, b"not found", "text/plain")

    def do_POST(self):
        if self.path != "/decisions":
            return self.send(404, b"not found", "text/plain")
        body = self.rfile.read(int(self.headers.get("Content-Length", 0)))
        data = json.loads(body.decode("utf-8"))
        if not isinstance(data, dict):
            return self.send(400, b"expected an object", "text/plain")
        os.makedirs(os.path.dirname(DECISIONS), exist_ok=True)
        tmp = DECISIONS + ".tmp"
        open(tmp, "w", encoding="utf-8").write(json.dumps(data, indent=1, ensure_ascii=False, sort_keys=True))
        os.replace(tmp, DECISIONS)
        self.send(200, b"ok", "text/plain")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--assets", help="staged game files (default: this checkout's assets/)")
    ap.add_argument("--port", type=int, default=18791)
    ap.add_argument("--missed", type=int, default=24, help="how many of the closest turned-down pairs to list")
    ap.add_argument("--match", help="tools/soundmatch.py's results (default: this checkout's build/soundmatch/)")
    ap.add_argument("--prepare-only", action="store_true")
    a = ap.parse_args()
    if a.assets:
        vl.ASSETS = os.path.abspath(a.assets)
    if a.match:
        global MATCH
        MATCH = os.path.abspath(a.match)
    pairs = prepare(a.missed)
    print("pairs %d (near %d, missed %d), %d sounds not decoded" % (
        len(pairs), sum(p["list"] == "near" for p in pairs), sum(p["list"] == "missed" for p in pairs),
        sum(not p[s]["snd"] for p in pairs for s in "ab")), flush=True)
    if a.prepare_only:
        return
    print("http://127.0.0.1:%d/" % a.port, flush=True)
    http.server.ThreadingHTTPServer(("127.0.0.1", a.port), Handler).serve_forever()


if __name__ == "__main__":
    main()
