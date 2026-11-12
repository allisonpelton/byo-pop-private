#!/usr/bin/env python3
"""Finds the voice clips among every game's sound effects, for the Sounds boards (the user, 2026-11-12).

Each sound a board in web/soundboards.js names is read from the game's staged files (assets/<game>/, a resource of a
Windows module, an entry of a zip or of Tip Top's archive, as web/music.js reads them), turned into 16 kHz mono with
ffmpeg and scored two ways:
  - its pitch: the share of its sounding frames with a voiced pitch of 80 to 400 Hz (an autocorrelation peak), broken
    into syllables (runs of voiced frames) and moving as speech does (the pitch's spread in semitones), with numpy;
  - Whisper (whisper.cpp, base.en, 2 threads), run once over every clip whose pitch looks like it may be speech: its
    words and the mean probability of their tokens (-ojf).
A clip is "sure" when both agree, "unsure" when one does, and left out otherwise. Results go to build/voicelines/:
voicelines.json (every clip scored) and unsure.txt (the clips a person should listen to). Nothing on the boards changes:
the sure clips' words go onto the boards by hand.

  python tools/voicelines.py [--games a,b] [--no-whisper]

The decoded clips and Whisper's outputs are cached under build/voicelines/; delete the folder to start over.
"""
import argparse
import json
import os
import re
import struct
import subprocess
import sys
import zipfile
import io

import numpy as np

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "build", "voicelines")
FFMPEG = "E:/retools/ffmpeg/ffmpeg-9.0.2-essentials_build/bin/ffmpeg.exe"
WHISPER = "E:/retools/whisper/Release/whisper-cli.exe"
MODEL = "E:/retools/whisper/models/ggml-base.en.bin"
ASSETS = os.path.join(ROOT, "assets")   # --assets: another checkout's staged files
RATE = 16000
FRAME, HOP = 640, 160            # 40 ms frames every 10 ms
LAG_MIN, LAG_MAX = RATE // 400, RATE // 80


# ---------- the boards ----------
def boards():
    """{game: [(file, label, when)]} from web/soundboards.js's groups (presets are left out)."""
    text = open(os.path.join(ROOT, "web", "soundboards.js"), encoding="utf-8").read()
    starts = [(m.group(1), m.start()) for m in re.finditer(r"\n  ([a-z0-9]+): \{", text)]
    out = {}
    for i, (game, at) in enumerate(starts):
        body = text[at: starts[i + 1][1] if i + 1 < len(starts) else len(text)]
        g = body.find("groups:")
        if g < 0:
            continue
        rows = re.findall(r'\[\s*"((?:[^"\\]|\\.)*)",\s*"((?:[^"\\]|\\.)*)"(?:,\s*"((?:[^"\\]|\\.)*)")?\s*\]', body[g:])
        out[game] = [(json.loads('"%s"' % f), json.loads('"%s"' % l), json.loads('"%s"' % (w or ""))) for f, l, w in rows]
    return out


# ---------- reading a sound out of the staged files ----------
_index = {}


def staged(game, rel):
    """The path of assets/<game>/<rel>, its case ignored, or None."""
    if game not in _index:
        base = os.path.join(ASSETS, game)
        idx = {}
        for d, _, files in os.walk(base):
            for f in files:
                p = os.path.join(d, f)
                idx[os.path.relpath(p, base).replace("\\", "/").lower()] = p
        _index[game] = idx
    return _index[game].get(rel.replace("\\", "/").lower())


def pe_resource(data, rtype, rid):
    """A resource of a Windows module by type and id (names or numbers as strings), or None."""
    if data[:2] != b"MZ":
        return None
    pe = struct.unpack_from("<I", data, 0x3c)[0]
    nsec, optsize = struct.unpack_from("<H", data, pe + 6)[0], struct.unpack_from("<H", data, pe + 20)[0]
    opt = pe + 24
    dirs = opt + (112 if struct.unpack_from("<H", data, opt)[0] == 0x20b else 96)
    rsrc = struct.unpack_from("<I", data, dirs + 16)[0]
    secs = []
    for s in range(nsec):
        h = opt + optsize + s * 40
        vsize, va, rsize, raw = struct.unpack_from("<IIII", data, h + 8)
        secs.append((va, max(vsize, rsize), raw))

    def at(rva):
        for va, size, raw in secs:
            if va <= rva < va + size:
                return rva - va + raw
        return -1
    base = at(rsrc)
    if base < 0:
        return None

    def name(v):
        if not v & 0x80000000:
            return str(v)
        o = base + (v & 0x7fffffff)
        n = struct.unpack_from("<H", data, o)[0]
        return data[o + 2: o + 2 + n * 2].decode("utf-16le")

    def walk(d, path):
        n = sum(struct.unpack_from("<HH", data, base + d + 12))
        for i in range(n):
            e = base + d + 16 + i * 8
            nm, to = struct.unpack_from("<II", data, e)
            nm = name(nm)
            if to & 0x80000000:
                if len(path) < 2:
                    r = walk(to & 0x7fffffff, path + [nm])
                    if r is not None:
                        return r
                continue
            if path == [rtype, rid]:
                off = at(struct.unpack_from("<I", data, base + to)[0])
                size = struct.unpack_from("<I", data, base + to + 4)[0]
                return data[off: off + size]
        return None
    return walk(0, [])


TIPTOP_SRC = [2, 6, 0, 7, 5, 4, 1, 3]


def tiptop_entry(data, name):
    """An entry of Tip Top's archive (its bytes stored with their bits permuted; see web/music.js), or None."""
    dec = bytearray(256)
    for c in range(256):
        enc = 0
        for b in range(8):
            enc |= ((c >> TIPTOP_SRC[b]) & 1) << b
        dec[enc] = c
    want = name.replace("/", "\\").lower()

    def u32(o):
        return dec[data[o]] | dec[data[o + 1]] << 8 | dec[data[o + 2]] << 16 | dec[data[o + 3]] << 24
    end, p = 4 + u32(0), 4
    while p < end:
        e = p
        while dec[data[e]]:
            e += 1
        n = bytes(dec[x] for x in data[p:e]).decode("latin-1")
        if not n:
            break
        off, size = u32(e + 1), u32(e + 5)
        if n.lower() == want:
            return bytes(data[off: off + size]).translate(bytes(dec))
        p = e + 9
    return None


def sound_bytes(game, ref):
    """A board's sound as (bytes, extension), or (None, why)."""
    if "|" not in ref:
        p = staged(game, ref)
        return (open(p, "rb").read(), os.path.splitext(p)[1]) if p else (None, "not staged")
    host, pick = ref.split("|", 1)
    p = staged(game, host)
    if not p:
        return None, "not staged"
    data = open(p, "rb").read()
    if pick.startswith("pe:"):
        _, rtype, rid = pick.split(":", 2)
        b = pe_resource(data, rtype, rid)
        return (b, ".wav") if b else (None, "no such resource")
    if pick.startswith("tiptop:"):
        b = tiptop_entry(data, pick[7:])
        return (b, os.path.splitext(pick)[1]) if b else (None, "no such entry")
    if pick.startswith("zip:"):
        want = pick[4:].lower()
        with zipfile.ZipFile(io.BytesIO(data)) as z:
            for n in z.namelist():
                if n.lower() == want:
                    return z.read(n), os.path.splitext(n)[1]
        return None, "no such entry"
    return None, "unknown container"


def safe(ref):
    return re.sub(r"[^A-Za-z0-9._-]+", "_", ref)


def to_wav(game, ref):
    """The clip as a 16 kHz mono WAV under build/voicelines/wav/<game>/ (made once), or (None, why)."""
    wav = os.path.join(OUT, "wav", game, safe(ref) + ".wav")
    if os.path.exists(wav):
        return wav, ""
    data, ext = sound_bytes(game, ref)
    if data is None:
        return None, ext
    os.makedirs(os.path.dirname(wav), exist_ok=True)
    src = wav[:-4] + ".src" + (ext or ".bin")
    open(src, "wb").write(data)
    r = subprocess.run([FFMPEG, "-v", "error", "-y", "-i", src, "-ar", str(RATE), "-ac", "1", "-c:a", "pcm_s16le", wav],
                       capture_output=True, text=True)
    os.remove(src)
    if r.returncode or not os.path.exists(wav):
        return None, "ffmpeg: " + (r.stderr.strip().splitlines() or ["failed"])[-1][:120]
    return wav, ""


# ---------- pitch ----------
def pitch_features(wav):
    """{seconds, voiced (share of sounding frames voiced at 80-400 Hz), syllables (voiced runs of 50 ms or more),
    spread (the voiced pitch's spread in semitones), score (0-1: how much the pitch looks like speech)}"""
    raw = open(wav, "rb").read()
    d = raw.find(b"data")
    x = np.frombuffer(raw[d + 8:], dtype="<i2").astype(np.float32) / 32768.0
    secs = len(x) / RATE
    if len(x) < FRAME:
        return {"seconds": round(secs, 2), "voiced": 0.0, "syllables": 0, "spread": 0.0, "score": 0.0}
    n = 1 + (len(x) - FRAME) // HOP
    frames = np.lib.stride_tricks.as_strided(x, (n, FRAME), (x.strides[0] * HOP, x.strides[0])) * np.hanning(FRAME)
    rms = np.sqrt((frames ** 2).mean(axis=1))
    sounding = rms > max(rms.max() * 0.06, 0.004)
    spec = np.fft.rfft(frames, 2 * FRAME)
    ac = np.fft.irfft(np.abs(spec) ** 2)[:, :LAG_MAX + 1]
    ac = ac / np.maximum(ac[:, :1], 1e-9)
    seg = ac[:, LAG_MIN:LAG_MAX + 1]
    peak = seg.max(axis=1)
    lag = seg.argmax(axis=1) + LAG_MIN
    voiced = sounding & (peak > 0.45)
    f0 = RATE / lag
    nsound = int(sounding.sum())
    share = float(voiced.sum()) / nsound if nsound else 0.0
    runs, run = 0, 0
    for v in voiced:
        if v:
            run += 1
        else:
            runs += run >= 5
            run = 0
    runs += run >= 5
    spread = float(np.std(12 * np.log2(f0[voiced] / 100.0))) if voiced.sum() > 3 else 0.0
    score = share * min(1.0, runs / 2.0) * min(1.0, spread / 1.0) if 0.15 < secs < 15 else 0.0
    return {"seconds": round(secs, 2), "voiced": round(share, 3), "syllables": runs, "spread": round(spread, 2),
            "score": round(score, 3)}


# ---------- Whisper ----------
def whisper(wavs):
    """Runs Whisper once over the clips that have no output yet; {wav: (words, mean token probability)}."""
    todo = [w for w in wavs if not os.path.exists(w + ".json")]
    for i in range(0, len(todo), 200):
        cmd = [WHISPER, "-m", MODEL, "-t", "2", "-l", "en", "-ojf", "-np", "-nt"]
        for w in todo[i:i + 200]:
            cmd += ["-f", w]
        subprocess.run(cmd, capture_output=True)
        print("whisper: %d of %d" % (min(i + 200, len(todo)), len(todo)), flush=True)
    out = {}
    for w in wavs:
        try:
            j = json.load(open(w + ".json", encoding="utf-8", errors="replace"))
        except (OSError, ValueError):
            out[w] = ("", 0.0)
            continue
        words, probs = [], []
        for seg in j.get("transcription", []):
            words.append(seg.get("text", ""))
            for t in seg.get("tokens", []):
                tx = t.get("text", "")
                if tx.startswith("[_") or not re.search(r"[A-Za-z0-9]", tx):
                    continue
                probs.append(float(t.get("p", 0)))
        text = " ".join(" ".join(words).split())
        out[w] = (text, round(sum(probs) / len(probs), 3) if probs else 0.0)
    return out


def is_words(text):
    """Whether Whisper heard words, not its stand-ins for noise ("[Music]", "(whooshing)", "*sighs*") or nothing."""
    t = re.sub(r"[\[(*][^\])*]*[\])*]", "", text)
    t = re.sub(r"[^A-Za-z0-9' ]", " ", t).strip()
    return bool(t) and t.lower() not in ("you", "thank you", "bye", "oh", "uh", "um", "hmm", "mm", "ah")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--games", help="only these games (comma-separated board ids)")
    ap.add_argument("--no-whisper", action="store_true", help="score by pitch alone")
    ap.add_argument("--assets", help="the staged game files to read (default: this checkout's assets/)")
    a = ap.parse_args()
    global ASSETS
    if a.assets:
        ASSETS = os.path.abspath(a.assets)
    want = set(a.games.split(",")) if a.games else None
    os.makedirs(OUT, exist_ok=True)
    clips, missing = [], []
    for game, rows in boards().items():
        if want and game not in want:
            continue
        seen = set()
        for ref, label, when in rows:
            if ref in seen:
                continue
            seen.add(ref)
            wav, why = to_wav(game, ref)
            if not wav:
                missing.append((game, ref, why))
                continue
            clips.append(dict(game=game, file=ref, label=label, wav=wav, **pitch_features(wav)))
    cand = [c for c in clips if c["score"] >= 0.08 or c["label"].startswith('"')]
    heard = {} if a.no_whisper else whisper([c["wav"] for c in cand])
    for c in clips:
        text, p = heard.get(c["wav"], ("", 0.0))
        c["words"], c["confidence"] = text, p
        words = is_words(text)
        if words and p >= 0.6 and c["score"] >= 0.15:
            c["class"] = "sure"
        elif (words and p >= 0.35) or (c["score"] >= 0.3 and words):
            c["class"] = "unsure"
        else:
            c["class"] = "no"
        del c["wav"]
    out = os.path.join(OUT, "voicelines%s.json" % ("-" + "-".join(sorted(want)) if want else ""))
    json.dump({"clips": clips, "missing": [dict(game=g, file=f, why=w) for g, f, w in missing]},
              open(out, "w", encoding="utf-8"), indent=1, ensure_ascii=False)
    with open(os.path.join(OUT, "unsure.txt"), "w", encoding="utf-8") as f:
        f.write("# Clips the pitch and Whisper disagree on: game, file, board label, Whisper's words (confidence), pitch score\n")
        for c in clips:
            if c["class"] == "unsure":
                f.write("%s\t%s\t%s\t\"%s\" (%.2f)\t%.2f\n" % (c["game"], c["file"], c["label"], c["words"], c["confidence"], c["score"]))
    by = {}
    for c in clips:
        by.setdefault(c["game"], {}).setdefault(c["class"], 0)
        by[c["game"]][c["class"]] += 1
    for g in sorted(by):
        print("%-14s sure %3d  unsure %3d  no %4d" % (g, by[g].get("sure", 0), by[g].get("unsure", 0), by[g].get("no", 0)))
    print("clips %d, not readable %d; build/voicelines/voicelines.json, build/voicelines/unsure.txt" % (len(clips), len(missing)))


if __name__ == "__main__":
    main()
