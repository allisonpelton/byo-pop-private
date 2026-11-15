#!/usr/bin/env python3
"""Finds the same sound in two games, for the Sounds boards (the user, 2026-11-12: "Check if sounds unused in a sequel were
used in the previous game"; "We might be able to figure out some unused sounds ... by their appearances in other games").

Every sound a board in web/soundboards.js names is decoded once to 16 kHz mono (tools/voicelines.py's reader and cache,
build/voicelines/wav/), and so is every sound file under the extra folders given with --extra (games not yet ported, read
only, never written). Decoding first means a difference in sample rate, bit depth, channels or format never stops a match;
leading and trailing silence is trimmed, and every score is a normalised correlation, so padding and loudness do not either.

Any two sounds are compared whatever their lengths (the user, 2026-11-13, of Bookworm Adventures' unused praise: "Some ...
seems like it could be higher fidelity versions from Bookworm. May be worth removing that limit"; until then pairs more
than 15% apart in length were never compared). The shorter slides along the longer, every 10 ms, by their loudness
envelopes (RMS every 10 ms), scored at each place by the correlation over just the stretch they overlap, so a sound found
inside a longer one (a re-cut, a padded copy, a line within a longer take) scores as well as an exact copy; the shorter may
overhang either end by a fifth of its length, for a copy trimmed tighter. At the best places (the three highest within
0.1 of the best) the waveforms are compared the same way (8 kHz, within 50 ms of the envelopes' place, normalised over the
overlap). "same" when the waveforms correlate at 0.97 or more (the same recording, re-encoded, re-cut or renamed), "near"
when the envelopes do at 0.90 or more and the waveforms at 0.75 (a variant: trimmed, filtered, mixed). A floor keeps clicks
from matching anything inside a longer sound: the shorter must last 0.2 s once trimmed, and the stretch of the longer it lies
on must not be flat. A shorter sound, down to 0.03 s, is compared only with one of about its own length (within 15%), as
before the limit came out, so two copies of a click still pair. A pitch or speed change is not caught: it stretches the envelope, and a few percent is enough to lose a match.

Only pairs with at least one side the board marks unused, or one side from --extra, are listed unless --all. Results:
build/soundmatch/matches.json and a readable matches.txt (with where the shorter lies in the longer when it is less than
nine tenths of it); the 100 closest pairs turned down (envelopes at 0.8 or more) in turned.json, for
tools/web/soundcompare.html.

  python tools/soundmatch.py [--assets DIR] [--extra NAME=DIR ...] [--all] [--pair A,B]
  python tools/soundmatch.py --within     one game's two files of the same recording, every board (build/soundmatch/within.txt),
                                          and each board sound's peak level, quietest first (build/soundmatch/levels.txt)
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
HOP = 160            # the envelope's step: 10 ms at 16 kHz
WAVE_STEP = 80       # one envelope step in the 8 kHz waveform
MIN_FRAMES = 20      # the floor: the shorter sound lasts 0.2 s once trimmed
MIN_WHOLE = 3        # a shorter one, down to 0.03 s, is compared only with a sound of about its own length (a click with a click)
WHOLE = 0.85         # about its own length: the shorter is at least this much of the longer
OVERHANG = 0.2       # the shorter may hang off either end of the longer by this much of its own length
SHIFT = 400          # the waveforms are lined up within 50 ms (at 8 kHz) of the envelopes' place
ENV_NEAR, WAVE_NEAR, WAVE_SAME, ENV_SHOWN = 0.90, 0.75, 0.97, 0.80


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


def pow2(n):
    return 1 << max(1, int(n - 1).bit_length())


class Clip:
    def __init__(self, game, ref, label, when, wav, extra=False):
        self.game, self.ref, self.label, self.when, self.extra = game, ref, label, when, extra
        self.src = None  # an unported game's file, read only
        self.wav = wav   # its 16 kHz mono copy
        x = samples(wav)
        self.peak = float(np.abs(x).max()) if len(x) else 0.0   # its loudest sample (of the 16 kHz mono copy)
        # trim leading and trailing silence, so a padded copy still lines up
        loud = np.nonzero(np.abs(x) > 0.004)[0]
        x = x[loud[0]:loud[-1] + 1] if len(loud) else x[:0]
        self.secs = len(x) / vl.RATE
        self.wave = x[::2].astype(np.float64)                                # 8 kHz
        n = len(x) // HOP
        self.env = np.sqrt((x[:n * HOP].reshape(n, HOP) ** 2).mean(axis=1)).astype(np.float64) if n else np.zeros(0)
        # unused: said on the button, or (on a board not yet relabelled) only in its description
        self.unused = label.endswith("(unused)") or bool(re.match(r"(Never played|Loaded, never played|The game never plays|Unused)", when))
        self._long = None

    def as_long(self):
        """For the side that is longer: the envelope's FFT (at twice its length, so a shorter one's every place, overhanging
        ones included, comes out of one product without wrapping onto another) and its running sums, made once."""
        if not self._long:
            e = self.env
            size = pow2(2 * len(e) + 1)
            self._long = dict(size=size, f=np.fft.rfft(e, size), c1=np.concatenate([[0.0], np.cumsum(e)]),
                              c2=np.concatenate([[0.0], np.cumsum(e * e)]))
        return self._long


def pearson_slide(s, longs, pad):
    """The shorter envelope s against each longer one at every place from pad frames before its start to pad frames past its
    end (the overhang counts as silence): for each longer one an array, index place + pad, of the correlation over the
    overlap. The longer ones of one FFT size go through together."""
    n = len(s)
    s0 = s - s.mean()
    sn = np.sqrt((s0 * s0).sum())
    res = [np.zeros(0)] * len(longs)
    if sn == 0:
        return res
    by_size = {}
    for i, q in enumerate(longs):
        by_size.setdefault(q.as_long()["size"], []).append(i)
    for size, idx in by_size.items():
        fs = np.conj(np.fft.rfft(s0, size))
        c = np.fft.irfft(np.stack([longs[i].as_long()["f"] for i in idx]) * fs, size)
        for row, i in enumerate(idx):
            L = longs[i].as_long()
            m = len(longs[i].env)
            k = np.arange(-pad, m - n + pad + 1)
            if len(k) < 1:
                continue
            num = c[row, k % size]          # a place before the start wraps to the end of the product (zeros between)
            a, b = np.clip(k, 0, m), np.clip(k + n, 0, m)
            s1 = L["c1"][b] - L["c1"][a]
            s2 = L["c2"][b] - L["c2"][a]
            var = s2 - s1 * s1 / n          # n times the stretch's variance (the overhang counts as silence)
            r = num / np.maximum(sn * np.sqrt(np.maximum(var, 0.0)), 1e-12)
            # a flat stretch of the longer (silence, a held tone) scores nothing
            r[var <= 1e-3 * np.maximum(s2, 1e-12)] = 0.0
            res[i] = r
    return res


def wave_score(p, q, frame):
    """p's waveform (the shorter) against q's at envelope place frame (p's first frame lies on q's frame), within 50 ms either
    way: the highest correlation over the overlap."""
    n = len(p.wave)
    start = frame * WAVE_STEP - SHIFT
    lo, hi = max(start, 0), min(start + n + 2 * SHIFT, len(q.wave))
    if hi - lo < n // 2:
        return 0.0
    seg = np.zeros(n + 2 * SHIFT)
    seg[lo - start:hi - start] = q.wave[lo:hi]
    size = pow2(len(seg) + n)
    c = np.fft.irfft(np.fft.rfft(seg, size) * np.conj(np.fft.rfft(p.wave, size)), size)[:2 * SHIFT + 1]
    c2 = np.concatenate([[0.0], np.cumsum(seg * seg)])
    w = np.sqrt(c2[n:n + 2 * SHIFT + 1] - c2[:2 * SHIFT + 1])
    pn = np.sqrt((p.wave * p.wave).sum())
    r = np.where(w > 1e-6, c / np.maximum(w * pn, 1e-12), 0.0)
    # only places where most of the shorter lies on sound of the longer count (not a few samples at an edge)
    return float(np.max(np.abs(r)))


def judge(p, q, scores, pad):
    """The verdict for the shorter p in the longer q from p's sliding envelope scores: (kind or None, envelope score, waveform
    score, where p starts in q in seconds)."""
    if len(scores) == 0:
        return None, 0.0, 0.0, 0.0
    best = float(scores.max())
    if best < ENV_SHOWN:
        return None, best, 0.0, 0.0
    order = np.argsort(-scores)
    tried, w, at = [], 0.0, int(order[0])
    for k in order:
        if len(tried) == 3 or scores[k] < best - 0.1:
            break
        if any(abs(int(k) - t) < 5 for t in tried):
            continue
        tried.append(int(k))
        v = wave_score(p, q, int(k) - pad)
        if v > w:
            w, at = v, int(k)
    kind = "same" if w >= WAVE_SAME else "near" if best >= ENV_NEAR and w >= WAVE_NEAR else None
    return kind, best, w, (at - pad) * HOP / vl.RATE


def compare_one(p, q):
    """One pair, either order: (kind or None, envelope, waveform, start of the shorter in the longer, shorter, longer)."""
    s, l = (p, q) if len(p.env) <= len(q.env) else (q, p)
    if len(s.env) < MIN_WHOLE or (len(s.env) < MIN_FRAMES and len(s.env) < WHOLE * len(l.env)):
        return None, 0.0, 0.0, 0.0, s, l
    pad = int(OVERHANG * len(s.env))
    r = judge(s, l, pearson_slide(s.env, [l], pad)[0], pad)
    return r + (s, l)


def side(c):
    d = dict(game=c.game, file=c.ref, label=c.label, when=c.when, secs=round(c.secs, 3))
    if c.src:
        d["src"] = c.src
    return d


def load_clips(extra_specs):
    """Every board sound and every sound under the --extra folders, as Clips."""
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
    for spec in extra_specs:
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
                            c = Clip(name, rel, rel, "", wav, extra=True)
                            c.src = os.path.join(d, f).replace("\\", "/")
                            clips.append(c)
    return clips


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--assets", help="staged game files (default: this checkout's assets/)")
    ap.add_argument("--extra", action="append", default=[], help="NAME=DIR: an unported game's folder, read only")
    ap.add_argument("--all", action="store_true", help="list every cross-game pair, not only those touching an unused sound")
    ap.add_argument("--pair", help="compare just these two sounds: game:file,game:file")
    ap.add_argument("--within", action="store_true", help="same recordings inside one game, and every board sound's peak level")
    a = ap.parse_args()
    if a.assets:
        vl.ASSETS = os.path.abspath(a.assets)
    os.makedirs(OUT, exist_ok=True)
    clips = load_clips(a.extra)
    print("clips", len(clips), flush=True)
    if a.pair:
        want = [s.split(":", 1) for s in a.pair.split(",")]
        p, q = [next(c for c in clips if c.game == g and c.ref == r) for g, r in want]
        kind, e, w, at, s, l = compare_one(p, q)
        print("%s %.2fs in %s %.2fs at %.2fs: env %.3f wave %.3f -> %s" % (s.ref, s.secs, l.ref, l.secs, at, e, w, kind))
        return
    clips.sort(key=lambda c: (len(c.env), c.game, c.ref))
    if a.within:
        with open(os.path.join(OUT, "levels.txt"), "w", encoding="utf-8") as f:
            for c in sorted((c for c in clips if not c.extra), key=lambda c: c.peak):
                f.write("%7.1f dB  %s %s [%s]\n" % (20 * np.log10(c.peak + 1e-9), c.game, c.ref, c.label))

    def wanted(p, q):
        if a.within:
            return p.game == q.game and not p.extra and not q.extra
        if p.game == q.game or (p.extra and q.extra):
            return False
        return a.all or p.unused or q.unused or p.extra or q.extra

    hits, turned = [], []
    for i, p in enumerate(clips):
        if len(p.env) < MIN_WHOLE:
            continue
        # every longer clip (later in the order) that this pair rule allows; under the floor, only those of about its length
        longs = [q for q in clips[i + 1:] if wanted(p, q) and (len(p.env) >= MIN_FRAMES or len(p.env) >= WHOLE * len(q.env))]
        if not longs:
            continue
        pad = int(OVERHANG * len(p.env))
        for q, scores in zip(longs, pearson_slide(p.env, longs, pad)):
            if len(scores) == 0 or scores.max() < ENV_SHOWN:
                continue
            kind, e, w, at = judge(p, q, scores, pad)
            h = dict(kind=kind or "turned down", env=round(e, 3), wave=round(w, 3), at=round(at, 2), a=side(p), b=side(q))
            if kind and not (a.within and kind != "same"):
                hits.append(h)
            elif not kind:
                turned.append(h)
        if i % 200 == 0:
            print("  %d/%d clips, %d pairs" % (i, len(clips), len(hits)), flush=True)
    for c in clips:
        c._long = None

    def where(h):
        # the shorter's place in the longer, when it is clearly a part of it
        return "  (%.2fs of %.2fs, from %.2fs)" % (h["a"]["secs"], h["b"]["secs"], h["at"]) if h["a"]["secs"] < 0.9 * h["b"]["secs"] else ""

    if a.within:
        lines = ["%s  %s [%s]  =  %s [%s]  %.2f/%.2f%s" % (h["a"]["game"], h["a"]["file"], h["a"]["label"], h["b"]["file"],
                                                         h["b"]["label"], h["env"], h["wave"], where(h)) for h in hits]
        open(os.path.join(OUT, "within.txt"), "w", encoding="utf-8").write("".join(x + "\n" for x in sorted(lines)))
        print("within: %d pairs; build/soundmatch/within.txt and levels.txt" % len(hits))
        return
    hits.sort(key=lambda h: (h["kind"] != "same", -h["wave"]))
    json.dump(hits, open(os.path.join(OUT, "matches.json"), "w", encoding="utf-8"), indent=1, ensure_ascii=False)
    # the closest pairs turned down, by the mean of their two scores: for a person to catch a near match the thresholds missed
    turned.sort(key=lambda h: -(h["env"] + h["wave"]))
    json.dump(turned[:100], open(os.path.join(OUT, "turned.json"), "w", encoding="utf-8"), indent=1, ensure_ascii=False)
    with open(os.path.join(OUT, "matches.txt"), "w", encoding="utf-8") as f:
        for h in hits:
            f.write("%s %.2f/%.2f  %s %s [%s]  =  %s %s [%s]%s\n" % (h["kind"], h["env"], h["wave"], h["a"]["game"], h["a"]["file"],
                    h["a"]["label"], h["b"]["game"], h["b"]["file"], h["b"]["label"], where(h)))
    print("pairs %d (same %d, near %d); build/soundmatch/matches.txt" % (len(hits), sum(h["kind"] == "same" for h in hits),
                                                                       sum(h["kind"] == "near" for h in hits)))


if __name__ == "__main__":
    main()
