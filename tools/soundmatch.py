#!/usr/bin/env python3
"""Finds the same sound in two games, for the Sounds boards (the user, 2026-11-12: "Check if sounds unused in a sequel were
used in the previous game"; "We might be able to figure out some unused sounds ... by their appearances in other games").

Every sound a board in web/soundboards.js names is decoded once to 16 kHz mono (tools/voicelines.py's reader and cache,
build/voicelines/wav/), and so is every sound file under the extra folders given with --extra (games not yet ported, read
only, never written). Each sound gets a fingerprint: its loudness every 10 ms, normalised. Two sounds of about the same
length (within 15%) are compared by the peak of their envelopes' cross-correlation and then their waveforms'
(normalised, at 8 kHz, within a 50 ms shift): "same" when the waveforms correlate at 0.97 or more (the same recording,
re-encoded or renamed), "near" when the envelopes do at 0.90 or more and the waveforms at 0.75 (a variant: trimmed,
pitched, mixed). Only pairs with at least one side the board marks unused, or one side from --extra, are listed unless
--all. Results: build/soundmatch/matches.json and a readable matches.txt.

  python tools/soundmatch.py [--assets DIR] [--extra NAME=DIR ...] [--all] [--pair A,B]
"""
import argparse
import json
import os
import re
import subprocess
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import voicelines as vl  # noqa: E402
import popcap_pak  # noqa: E402

OUT = os.path.join(vl.ROOT, "build", "soundmatch")
SOUND_EXT = (".ogg", ".wav", ".mp3", ".au")


def samples(wav):
    raw = open(wav, "rb").read()
    d = raw.find(b"data")
    return np.frombuffer(raw[d + 8:], dtype="<i2").astype(np.float32) / 32768.0


def extra_wav(name, path, base):
    rel = os.path.relpath(path, base).replace("\\", "/")
    wav = os.path.join(vl.OUT, "wav", "_extra", name, vl.safe(rel) + ".wav")
    if not os.path.exists(wav):
        os.makedirs(os.path.dirname(wav), exist_ok=True)
        r = subprocess.run([vl.FFMPEG, "-v", "error", "-y", "-i", path, "-ar", str(vl.RATE), "-ac", "1", "-c:a", "pcm_s16le", wav],
                           capture_output=True)
        if r.returncode or not os.path.exists(wav):
            return None, rel
    return wav, rel


class Clip:
    def __init__(self, game, ref, label, when, wav, extra=False):
        self.game, self.ref, self.label, self.when, self.extra = game, ref, label, when, extra
        x = samples(wav)
        # trim leading and trailing silence, so a padded copy still lines up
        loud = np.nonzero(np.abs(x) > 0.004)[0]
        x = x[loud[0]:loud[-1] + 1] if len(loud) else x[:0]
        self.secs = len(x) / vl.RATE
        self.wave = x[::2] / (np.sqrt((x[::2] ** 2).sum()) + 1e-9)          # 8 kHz, unit energy
        n = len(x) // 160
        env = np.sqrt((x[:n * 160].reshape(n, 160) ** 2).mean(axis=1)) if n else np.zeros(1)
        env = env - env.mean()
        self.env = env / (np.sqrt((env ** 2).sum()) + 1e-9)
        # unused: said on the button, or (on a board not yet relabelled) only in its description
        self.unused = label.endswith("(unused)") or bool(re.match(r"(Never played|Loaded, never played|The game never plays|Unused)", when))


def xcorr_peak(a, b, max_shift):
    """The highest normalised correlation of a and b over shifts up to max_shift samples (FFT)."""
    n = len(a) + len(b)
    size = 1 << (n - 1).bit_length()
    c = np.fft.irfft(np.fft.rfft(a, size) * np.conj(np.fft.rfft(b, size)), size)
    c = np.concatenate([c[-max_shift:], c[:max_shift + 1]]) if max_shift < size // 2 else c
    return float(np.max(np.abs(c)))


def compare(p, q):
    if min(p.secs, q.secs) < 0.03 or abs(p.secs - q.secs) > 0.15 * max(p.secs, q.secs):
        return None
    e = xcorr_peak(p.env, q.env, 5)
    if e < 0.9:
        return None
    w = xcorr_peak(p.wave, q.wave, 400)
    if w >= 0.97:
        return ("same", e, w)
    if w >= 0.75:
        return ("near", e, w)
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--assets", help="staged game files (default: this checkout's assets/)")
    ap.add_argument("--extra", action="append", default=[], help="NAME=DIR: an unported game's folder, read only")
    ap.add_argument("--all", action="store_true", help="list every cross-game pair, not only those touching an unused sound")
    ap.add_argument("--pair", help="compare just these two board sounds: game:file,game:file")
    a = ap.parse_args()
    if a.assets:
        vl.ASSETS = os.path.abspath(a.assets)
    os.makedirs(OUT, exist_ok=True)
    clips = []
    for game, rows in vl.boards().items():
        seen = set()
        for ref, label, when in rows:
            if ref in seen:
                continue
            seen.add(ref)
            wav, _ = vl.to_wav(game, ref)
            if wav:
                clips.append(Clip(game, ref, label, when, wav))
    for spec in a.extra:
        name, base = spec.split("=", 1)
        bases = [base]
        # a PopCap main.pak: its sound entries are unpacked under build/soundmatch/extra/<name>/ (the copy is only read)
        pak = os.path.join(base, "main.pak")
        if os.path.exists(pak):
            unpacked = os.path.join(OUT, "extra", name)
            if not os.path.isdir(unpacked):
                data = open(pak, "rb").read()
                plain = (np.frombuffer(data, dtype=np.uint8) ^ 0xF7).tobytes()
                try:
                    for rel, off, size in popcap_pak.read_records(data, 0xF7):
                        if rel.lower().endswith(SOUND_EXT):
                            dst = os.path.join(unpacked, rel)
                            os.makedirs(os.path.dirname(dst), exist_ok=True)
                            open(dst, "wb").write(plain[off:off + size])
                except ValueError as e:
                    print(name, "main.pak:", e)
            bases.append(unpacked)
        for b in bases:
          for d, _, files in os.walk(b):
            for f in files:
                if f.lower().endswith(SOUND_EXT):
                    wav, rel = extra_wav(name, os.path.join(d, f), b)
                    if wav:
                        clips.append(Clip(name, rel, rel, "", wav, extra=True))
    print("clips", len(clips), flush=True)
    if a.pair:
        want = [s.split(":", 1) for s in a.pair.split(",")]
        two = [next(c for c in clips if c.game == g and c.ref == r) for g, r in want]
        p, q = two
        print(p.secs, q.secs, "env", xcorr_peak(p.env, q.env, 5), "wave", xcorr_peak(p.wave, q.wave, 400))
        return
    clips.sort(key=lambda c: c.secs)
    hits = []
    for i, p in enumerate(clips):
        j = i + 1
        while j < len(clips) and clips[j].secs <= p.secs * 1.15 + 0.01:
            q = clips[j]
            j += 1
            if p.game == q.game or (p.extra and q.extra):
                continue
            if not (a.all or p.unused or q.unused or p.extra or q.extra):
                continue
            r = compare(p, q)
            if r:
                hits.append(dict(kind=r[0], env=round(r[1], 3), wave=round(r[2], 3),
                                 a=dict(game=p.game, file=p.ref, label=p.label, when=p.when),
                                 b=dict(game=q.game, file=q.ref, label=q.label, when=q.when)))
    hits.sort(key=lambda h: (h["kind"] != "same", -h["wave"]))
    json.dump(hits, open(os.path.join(OUT, "matches.json"), "w", encoding="utf-8"), indent=1, ensure_ascii=False)
    with open(os.path.join(OUT, "matches.txt"), "w", encoding="utf-8") as f:
        for h in hits:
            f.write("%s %.2f/%.2f  %s %s [%s]  =  %s %s [%s]\n" % (h["kind"], h["env"], h["wave"], h["a"]["game"], h["a"]["file"],
                    h["a"]["label"], h["b"]["game"], h["b"]["file"], h["b"]["label"]))
    print("pairs %d (same %d, near %d); build/soundmatch/matches.txt" % (len(hits), sum(h["kind"] == "same" for h in hits),
                                                                       sum(h["kind"] == "near" for h in hits)))


if __name__ == "__main__":
    main()
