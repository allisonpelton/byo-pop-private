#!/usr/bin/env python3
"""Finds sounds based on others, across games, for the Sounds boards (the user, 2026-11-13: "Some sounds are based on others,
and if it's possible to find those algorithmically that may be useful to report. I rejected many Bejeweled matches, when
the sounds were in fact similar but with extra effects added in Bejeweled 3"). tools/soundmatch.py finds the same recording
(or a near copy); this finds one sound inside another that was changed: pitched or sped up or slowed, given reverb or an
echo or a longer or shorter tail, filtered, or mixed with another sound on top.

How. Each sound (tools/soundmatch.py's clips: every board sound and the --extra folders, 16 kHz mono, silence trimmed, up
to 20 s) gets a spectrogram on a log-frequency axis, quarter-tone bins from 150 Hz to 7.6 kHz every 8 ms, so a change of
pitch only moves it up or down. Two steps, the first to find where two sounds might line up, the second to say how much of
one is in the other there.

Finding: the sound's peaks (local maxima, the strongest 30 a second in each quarter second, within 45 dB of its loudest)
are its landmarks, and each anchor peak with two of the next five after it makes a hash of the two pitch steps and the two
time steps (audio fingerprinting's landmark method, made pitch-free by keeping steps rather than pitches). The index keeps
two of the next ten, so a sound with peaks added still holds the base's hashes. A speed change scales the time steps, so
each sound is looked up at 65 speeds, an octave either way. A hash found in the other sound votes for a speed, a time and a
pitch offset; the six places with the most votes are kept, and one more at the same pitch and speed, at the time where the
most peaks line up, for the commonest change of all: a layer, reverb or a filter with no change of pitch.

Measuring: at each place, the share of sound A's peaks (within 30 dB of its loudest) that B has a peak beside, within two
frames and a bin; less the same share at every other time and a third of an octave off (its 90th percentile: chance,
which is high for a dense, noisy B); scaled so 1 is every peak and 0 is chance. A layer, reverb or an echo added to A
leaves its peaks there, which is why this holds up where the waveform and the envelope (tools/soundmatch.py) do not. Done
both ways at the best place; the one held more is the base, and a pair is listed when the base holds at least 0.50 in the
other. The user's turned-down Bejeweled 3 pairs (speedmatch1 to 9 against Bejeweled 2's combo32 and Twist's combo_2, the
first tenth of a second shared and a tone that climbs a step a level laid on top) score 0.51 to 0.74, the pairs the user
called similar 0.65 to 0.94, and two noisy controls that share nothing (firework against a car door, a stats chime) 0.43
and 0.44, which is what sets the line at 0.50. The derived sound must also give at least 0.15 back (its own peaks held
in the base, above chance): a sound with few peaks, a pure tone above all, is held in almost any busy sound at one of the
65 speeds and some pitch, while the busy sound gives nearly nothing back (0.00 to 0.09 in a sample of 250 clips, where
those made nine pairs in ten of the first run's); the pairs the user judged give 0.24 back and up. A short sound buried
in a much longer one is missed for it. Two more cuts, after the whole run: a sound in more than 10 pairs is a generic
hit (a click, a bump, an impact) that every other short hit holds about as well (the first full run's three busiest,
150 to 190 pairs each, at a median of 0.62), so its pairs stay only at 0.85 and up; and a pair that holds 0.95 both ways
with nothing changed is the same recording, left to soundmatch. Then the reasons, read from the two sounds lined up:

  pitched    the pitch offset, as a percentage, from two quarter-tone steps (5.9%) up; one step is within the peaks' play
  sped       the speed, when it is not the pitch's own (a resampled sound's speed follows its pitch; a sped-up one's does not)
  tail       the derived sound runs on past the base's end (reverb, an echo or a longer ring), or stops sooner
  layered    two in five or more of the derived sound's peaks during the base's own time are not the base's (a copy of the
             same recording has under one in four)
  filtered   the derived sound's top or bottom third of the band is 10 dB or more quieter than the base's (or louder:
             brighter, fuller)

Results: build/soundmatch/based.json and based.txt, for tools/web/soundcompare.html's own list, where the user accepts or
rejects each one; nothing goes on a board until then. --test scores the pairs the user already judged
(testnotes/AH-decisions.json) and says how many of the turned-down Bejeweled 3 pairs come out as "based on".

  python tools/soundbased.py [--assets DIR] [--extra NAME=DIR ...] [--test] [--min 0.50] [--back 0.15]
  python tools/soundbased.py --refilter [--min 0.50] [--back 0.15]    (the last run's pairs again, at new lines)

based-all.json keeps every pair at --min with no back line or cuts, which --refilter reads.
"""
import argparse
import json
import os
import sys

import numpy as np
from numpy.lib.stride_tricks import sliding_window_view

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import soundmatch as sm  # noqa: E402
import voicelines as vl  # noqa: E402

N_FFT, HOP = 1024, 128            # 64 ms windows every 8 ms at 16 kHz
F0, PER_OCT = 150.0, 24           # quarter-tone bins from 150 Hz
F_TOP = 7600.0
K = int(PER_OCT * np.log2(F_TOP / F0)) + 1
PEAKS_PER_SEC, FLOOR_DB, BLOCK = 30, 45, 32   # 32 frames: a quarter second
MAIN_DB = 30                      # the peaks a sound must keep to be held in another: those within 30 dB of its loudest
FAN, DT_MAX, DF_MAX = 5, 40, 36   # targets after an anchor: the next five within 320 ms and 1.5 octaves
FAN_INDEX = 10                    # the index keeps the next ten, so a sound with peaks added (reverb, a layer) still holds them
DT_Q = 2                          # time steps kept in 16 ms units, so a speed a little off still finds them
SPEEDS = 2.0 ** (np.arange(-32, 33) / 32.0)   # an octave either way, in steps of a 32nd of one
MAX_SECS = 20.0                   # longer files (tunes, ambiences) are left out
MIN_HASHES = 10
GAP = 64                          # frames of silence between clips on the strip
GATE = 0.45                       # the gate's share above chance, for a pair to be checked (the user's pairs: 0.52 up)
VOTED = 10                        # and each clip's this many others with the most hash votes for their size, for a change of pitch or speed
TOP = 6                           # the places the hashes vote for most, each checked by presence
HUB, HUB_STRONG = 10, 0.85        # a sound in more than HUB pairs keeps only those at HUB_STRONG and up
SAME = 0.95                       # both ways, nothing changed: the same recording
DECISIONS = os.path.join(vl.ROOT, "testnotes", "AH-decisions.json")


def logbins():
    """For each quarter-tone bin, the linear FFT bins that fall in it (at least the nearest one)."""
    lin = np.fft.rfftfreq(N_FFT, 1 / vl.RATE)
    out = []
    for k in range(K):
        f = F0 * 2 ** (k / PER_OCT)
        lo, hi = f * 2 ** (-0.5 / PER_OCT), f * 2 ** (0.5 / PER_OCT)
        idx = np.nonzero((lin >= lo) & (lin < hi))[0]
        out.append(idx if len(idx) else np.array([int(round(f / (vl.RATE / N_FFT)))]))
    return out


BINS = logbins()


def spectrogram(x):
    """Power in dB, frames by quarter-tone bins."""
    if len(x) < N_FFT:
        x = np.concatenate([x, np.zeros(N_FFT - len(x))])
    fr = sliding_window_view(x, N_FFT)[::HOP] * np.hanning(N_FFT)
    p = np.abs(np.fft.rfft(fr, axis=1)) ** 2
    s = np.stack([p[:, b].max(axis=1) for b in BINS], axis=1)
    return 10 * np.log10(s + 1e-12)


def peaks(s):
    """The landmarks: (frame, bin) of local maxima (7 frames by 7 bins) within FLOOR_DB of the loudest, the strongest
    PEAKS_PER_SEC a second in each quarter second, so a loud layer does not take all of them from a quieter part."""
    pad = np.pad(s, 3, constant_values=-1e9)
    mx = sliding_window_view(pad, (7, 7)).max(axis=(2, 3))
    t, f = np.nonzero((s == mx) & (s > s.max() - FLOOR_DB))
    per = max(1, int(PEAKS_PER_SEC * BLOCK * HOP / vl.RATE))
    keep = np.zeros(len(t), bool)
    for b in np.unique(t // BLOCK):
        m = np.nonzero(t // BLOCK == b)[0]
        keep[m[np.argsort(-s[t[m], f[m]])[:per]]] = True
    t, f = t[keep], f[keep]
    o = np.lexsort((f, t))
    return t[o], f[o]


def dense(s):
    """Every local maximum (5 frames by 3 bins) within FLOOR_DB of the loudest, and how far under the loudest it is."""
    pad = np.pad(s, ((2, 2), (1, 1)), constant_values=-1e9)
    mx = sliding_window_view(pad, (5, 3)).max(axis=(2, 3))
    t, f = np.nonzero((s == mx) & (s > s.max() - FLOOR_DB))
    return t, f, s.max() - s[t, f]


def presence(pr, a, b, speed, dt0, df0, chance=True):
    """How much of sound a is in sound b, a's frame t and bin f standing at t * speed + dt0 and f + df0 in b: the share of
    a's peaks within MAIN_DB of its loudest that b has a peak beside (2 frames, 1 bin), the best over a few frames and a bin
    either way of the place given, and the same share at every other time and a third of an octave off (chance). A layer or
    reverb added to a leaves its peaks there; a different sound has few more of them there than anywhere else."""
    ta, fa, da = pr.dense[a]
    m = da <= MAIN_DB
    ta, fa = ta[m], fa[m]
    nb = len(pr.spec[b])
    grid = pr.grid(b)
    if not len(ta):
        return 0.0, 0.0, dt0, df0

    def share(dt, df):
        t2 = np.rint(ta * speed + dt).astype(np.int64)
        f2 = fa + df
        ok = (t2 >= -2) & (t2 < nb + 2) & (f2 >= -1) & (f2 <= K)
        hit = np.zeros(len(ta), bool)
        hit[ok] = grid[t2[ok] + 2, f2[ok] + 1]
        return float(hit.mean())
    # the place the hashes gave, moved only when a nearby one is clearly better (a bin's wiggle is not a pitch change)
    best = (share(dt0, df0), dt0, df0)
    for ddf in (-1, 0, 1):
        for ddt in range(-6, 7, 2):
            s = share(dt0 + ddt, df0 + ddf)
            if s > best[0] + 0.05:
                best = (s, dt0 + ddt, df0 + ddf)
    if not chance:
        return best[0], 0.0, best[1], best[2]
    # chance: the same peaks at every other time, and a third of an octave off, the 90th percentile of those (a dense, noisy
    # b has a peak near anywhere, and the best of many tries is better than a typical one)
    lo, hi = -int(ta.max() * speed) - 2, nb + 2
    off = [share(dt, best[2]) for dt in range(lo, hi, 4) if abs(dt - best[1]) > 12]
    off += [share(dt, best[2] + df) for dt in range(lo, hi, 8) for df in (-8, 8)]
    chance = float(np.percentile(off, 90)) if off else 0.0
    return best[0], float(chance), best[1], best[2]
    for ddf in (-1, 0, 1):
        for ddt in range(-6, 7, 2):
            t2 = np.rint(ta * speed + dt0 + ddt).astype(np.int64)
            f2 = fa + df0 + ddf
            ok = (t2 >= -2) & (t2 < nb + 2) & (f2 >= -1) & (f2 <= K)
            hit = np.zeros(len(ta), bool)
            hit[ok] = grid[t2[ok] + 2, f2[ok] + 1]
            share = hit.mean()
            if share > best[0]:
                best = (float(share), dt0 + ddt, df0 + ddf)
    return best


def hashes(t, f, fan=FAN):
    """Each anchor with two of its next FAN peaks: (pitch step 1, pitch step 2, time step 1, time step 2, anchor frame,
    anchor bin), the steps exact (frames, bins)."""
    out = []
    n = len(t)
    for i in range(n):
        tg = []
        for j in range(i + 1, n):
            dt = t[j] - t[i]
            if dt > DT_MAX:
                break
            if dt >= 1 and abs(f[j] - f[i]) <= DF_MAX:
                tg.append(j)
                if len(tg) == fan:
                    break
        for a in range(len(tg)):
            for b in range(a + 1, len(tg)):
                j, k = tg[a], tg[b]
                out.append((f[j] - f[i], f[k] - f[i], t[j] - t[i], t[k] - t[i], t[i], f[i]))
    return np.array(out, dtype=np.int64).reshape(-1, 6)


def key(df1, df2, dt1, dt2):
    q1, q2 = dt1 // DT_Q, dt2 // DT_Q
    return (((df1 + DF_MAX) * (2 * DF_MAX + 1) + (df2 + DF_MAX)) * (DT_MAX // DT_Q + 2) + q1) * (DT_MAX // DT_Q + 2) + q2


class Prints:
    """Every clip's hashes in one sorted index."""

    def __init__(self, clips):
        self.clips, self.h, self.spec, self.dense, self._grids = [], [], [], [], {}
        keys, cid, ta, fa = [], [], [], []
        for c in clips:
            x = sm.samples(c.wav)
            loud = np.nonzero(np.abs(x) > 0.004)[0]
            x = x[loud[0]:loud[-1] + 1] if len(loud) else x[:0]
            if len(x) < vl.RATE * 0.05 or len(x) > vl.RATE * MAX_SECS:
                continue
            s = spectrogram(x.astype(np.float64))
            pk = peaks(s)
            h = hashes(*pk)
            hx = hashes(*pk, fan=FAN_INDEX)
            if len(h) < MIN_HASHES:
                continue
            i = len(self.clips)
            self.clips.append(c)
            self.h.append(h)
            self.spec.append(s)
            self.dense.append(dense(s))
            keys.append(key(hx[:, 0], hx[:, 1], hx[:, 2], hx[:, 3]))
            cid.append(np.full(len(hx), i))
            ta.append(hx[:, 4])
            fa.append(hx[:, 5])
        k = np.concatenate(keys)
        o = np.argsort(k, kind="stable")
        self.key, self.cid, self.t, self.f = k[o], np.concatenate(cid)[o], np.concatenate(ta)[o], np.concatenate(fa)[o]

    def grid(self, b):
        """Where clip b has a peak within 2 frames and a bin, as a (frames + 4) by (bins + 2) array, made once."""
        g = self._grids.get(b)
        if g is None:
            tb, fb, _ = self.dense[b]
            nb = len(self.spec[b])
            g = np.zeros((nb + 4, K + 2), bool)
            for dt in range(-2, 3):
                for df in (-1, 0, 1):
                    g[np.clip(tb + dt + 2, 0, nb + 3), np.clip(fb + df + 1, 0, K + 1)] = True
            if len(self._grids) > 400:
                self._grids.clear()
            self._grids[b] = g
        return g

    def strip(self):
        """Every clip's peak grid end to end along one time axis, GAP frames apart, bins by frames, made once; and where each
        clip's frame 0 is on it."""
        if getattr(self, "_strip", None) is None:
            lens = [len(s) + 4 for s in self.spec]
            self.at = np.concatenate([[0], np.cumsum(np.array(lens) + GAP)])[:-1] + 2
            g = np.zeros((K + 2, int(self.at[-1] + lens[-1] + GAP)), np.uint8)
            for b in range(len(self.clips)):
                g[:, self.at[b] - 2:self.at[b] - 2 + lens[b]] = self.grid(b).T
            self._strip = g
            self._grids.clear()
        return self._strip

    def gate(self, a):
        """Clip a's peaks (within MAIN_DB of its loudest) against every clip at its own pitch and speed, at every time: for
        each clip, the best share of them beside one of its peaks, and chance: the 90th percentile of the share at the
        other times a overlaps it (presence's measure, without the pitches either side)."""
        g = self.strip()
        ta, fa, da = self.dense[a]
        m = da <= MAIN_DB
        ta, fa = ta[m], fa[m]
        na, T = len(self.spec[a]), g.shape[1]
        best, mean = np.zeros(len(self.clips)), np.zeros(len(self.clips))
        if not len(ta):
            return best, mean
        # hits[s]: a's frame 0 at strip frame s - na
        pad = np.concatenate([np.zeros((K + 2, na), np.uint8), g, np.zeros((K + 2, na + 4), np.uint8)], axis=1)
        hits = np.zeros(T + na, np.int32)
        for tt, ff in zip(ta, fa):
            hits += pad[ff + 1, tt:tt + T + na]
        n = float(len(ta))
        for b in range(len(self.clips)):
            lo, hi = self.at[b], self.at[b] + len(self.spec[b]) + na
            seg = hits[max(lo, 0):hi]
            if len(seg) > 8:
                k = int(np.argmax(seg))
                rest = np.concatenate([seg[:max(k - 12, 0)], seg[k + 13:]])
                best[b] = seg[k] / n
                mean[b] = np.percentile(rest, 90) / n if len(rest) else 0.0
        return best, mean

    def lookup(self, i, others=None):
        """Clip i's hashes found in every other clip: {other: [(votes, speed, time offset in frames, pitch offset in bins)]},
        the TOP places (speed, time, pitch) with the most votes for each, most first. A place is a cell of (other clip, time
        offset in 32 ms steps, pitch offset), packed into one integer; its votes are its own and its two neighbours' in time
        (a little off in speed), and each speed's two best cells for each other clip are kept."""
        h = self.h[i]
        found = []
        for r in SPEEDS:
            dt1 = np.rint(h[:, 2] * r).astype(np.int64)
            dt2 = np.rint(h[:, 3] * r).astype(np.int64)
            ok = (dt2 <= DT_MAX) & (dt1 >= 1)
            hk = h[ok]
            q = key(hk[:, 0], hk[:, 1], dt1[ok], dt2[ok])
            lo = np.searchsorted(self.key, q, "left")
            cnt = np.searchsorted(self.key, q, "right") - lo
            total = int(cnt.sum())
            if total == 0:
                continue
            src = np.repeat(np.arange(len(q)), cnt)
            pos = np.arange(total) - np.repeat(np.cumsum(cnt) - cnt, cnt) + np.repeat(lo, cnt)
            other = self.cid[pos]
            keep = other != i
            if others is not None:
                keep &= np.isin(other, others)
            if not keep.any():
                continue
            src, pos, other = src[keep], pos[keep], other[keep]
            dt0 = np.rint((self.t[pos] - hk[src, 4] * r) / 4).astype(np.int64) + 2048   # 32 ms steps
            df0 = self.f[pos] - hk[src, 5] + 256
            cell = (other.astype(np.int64) << 21) | (dt0 << 9) | df0
            # one vote per hash per cell: a hash repeated in a held tone must not count many times
            cell = np.unique((cell << 24) | src) >> 24
            u, c = np.unique(cell, return_counts=True)
            sm_ = c.copy()
            for step in (-(1 << 9), 1 << 9):
                j = np.searchsorted(u, u + step)
                hit = (j < len(u)) & (u[np.minimum(j, len(u) - 1)] == u + step)
                sm_[hit] += c[j[hit]]
            o = u >> 21
            order = np.lexsort((-sm_, o))
            first = np.r_[True, o[order][1:] != o[order][:-1]]
            rank = np.arange(len(order)) - np.maximum.accumulate(np.where(first, np.arange(len(order)), 0))
            pick = order[rank < 2]
            found.append(np.stack([o[pick], sm_[pick], np.full(len(pick), int(round(np.log2(r) * 32))),
                                   ((u[pick] >> 9) & 4095) - 2048, (u[pick] & 511) - 256], axis=1))
        best = {}
        if found:
            f = np.concatenate(found)
            f = f[np.lexsort((-f[:, 1], f[:, 0]))]
            first = np.r_[True, f[1:, 0] != f[:-1, 0]]
            rank = np.arange(len(f)) - np.maximum.accumulate(np.where(first, np.arange(len(f)), 0))
            for row in f[rank < TOP]:
                best.setdefault(int(row[0]), []).append((int(row[1]), float(2.0 ** (row[2] / 32.0)), int(row[3]) * 4, int(row[4])))
        return best


def reasons(pr, base, derived, speed, dt0, df0):
    """What was done to the base to make the derived sound, as read from their spectrograms lined up: a list of short
    phrases, and the numbers behind them."""
    sb, sd = pr.spec[base], pr.spec[derived]
    out, facts = [], {}
    pct = (2 ** (df0 / PER_OCT) - 1) * 100
    if abs(df0) >= 2:
        out.append("pitched %s by about %d%%" % ("up" if df0 > 0 else "down", round(abs(pct))))
    facts["pitch_pct"] = round(pct, 1)
    pitch_speed = 2 ** (df0 / PER_OCT)
    # the speed the hashes found is the base's time steps scaled to the derived's; a resampled sound's speed is its pitch's
    facts["speed"] = round(speed, 3)
    if abs(np.log2(speed / pitch_speed)) > np.log2(1.06):
        # a pitch changed with the length kept (pitch shifting, not resampling) says so rather than "sped up 0%"
        if round(abs(speed - 1) * 100) == 0:
            out.append("the same length")
        else:
            out.append("%s %d%%" % ("slowed" if speed > 1 else "sped up", round(abs(speed - 1) * 100)))
    # the base's span in the derived's frames
    start, end = dt0, dt0 + int(round(len(sb) * speed))
    nd = len(sd)
    e_b = 10 ** (sb / 10).sum(axis=1)
    e_d = 10 ** (sd / 10).sum(axis=1)
    if end < nd - 25:
        after = e_d[max(end, 0):].sum() / (e_d.sum() + 1e-12)
        if after > 0.05:
            out.append("a longer tail (%.1f s more)" % ((nd - end) * HOP / vl.RATE))
        facts["tail_after"] = round(float(after), 3)
    elif end > nd + 25:
        out.append("a shorter tail (%.1f s less)" % ((end - nd) * HOP / vl.RATE))
    if start > 25:
        out.append("something before it (%.1f s)" % (start * HOP / vl.RATE))
    # layered: peaks of the derived within the base's span that no base peak lines up with
    tb, fb = peaks(sb)
    td, fd = peaks(sd)
    inside = (td >= max(start, 0)) & (td < min(end, nd))
    if inside.sum() > 10:
        bset = set(zip(np.rint(tb * speed + start).astype(int) // 3, (fb + df0) // 2))
        own = np.array([(int(t) // 3, int(f) // 2) in bset or (int(t) // 3 + 1, int(f) // 2) in bset or
                        (int(t) // 3 - 1, int(f) // 2) in bset for t, f in zip(td[inside], fd[inside])])
        extra = 1 - own.mean()
        facts["extra_peaks"] = round(float(extra), 2)
        if extra > 0.4:
            out.append("layered with another sound")
    # filtered: the band's ends compared over the shared span, the base moved by the pitch offset
    a, b = max(start, 0), min(end, nd)
    if b - a > 10:
        idx = np.clip(np.rint((np.arange(a, b) - start) / speed).astype(int), 0, len(sb) - 1)
        pb = 10 ** (sb[idx] / 10)
        pd = 10 ** (sd[a:b] / 10)
        sh = np.zeros_like(pb)
        if df0 >= 0:
            sh[:, df0:] = pb[:, :K - df0]
        else:
            sh[:, :K + df0] = pb[:, -df0:]
        third = K // 3
        def ratio(lo, hi):
            return 10 * np.log10((pd[:, lo:hi].sum() + 1e-12) / (pd.sum() + 1e-12)) - 10 * np.log10((sh[:, lo:hi].sum() + 1e-12) / (sh.sum() + 1e-12))
        hi_db, lo_db = ratio(2 * third, K), ratio(0, third)
        facts["high_db"], facts["low_db"] = round(float(hi_db), 1), round(float(lo_db), 1)
        if hi_db < -10:
            out.append("filtered (the highs cut)")
        elif lo_db < -10:
            out.append("filtered (the lows cut)")
        elif hi_db > 10:
            out.append("brighter (more highs)")
        elif lo_db > 10:
            out.append("fuller (more lows)")
    return out, facts


def plain_time(pr, a, b):
    """Where a's peaks best line up with b's at the same pitch and speed: the time offset, in frames, with the most of them
    beside a peak of b, every 2 frames from a starting before b to a starting at b's end."""
    ta, fa, da = pr.dense[a]
    m = da <= MAIN_DB
    ta, fa = ta[m], fa[m]
    nb = len(pr.spec[b])
    if not len(ta):
        return 0
    g = pr.grid(b)
    shifts = np.arange(-int(ta.max()), nb, 2)
    t2 = ta[None, :] + shifts[:, None]
    ok = (t2 >= -2) & (t2 < nb + 2)
    hit = np.zeros(t2.shape, bool)
    hit[ok] = g[t2[ok] + 2, np.broadcast_to(fa + 1, t2.shape)[ok]]
    return int(shifts[int(np.argmax(hit.sum(axis=1)))])


def judge_pair(pr, i, j, li, lj):
    """From both lookups: None, or (base, derived, holds, held back, speed, time offset, pitch offset), where holds is how
    much of the base is in the derived sound and held back how much of the derived is in the base (presence above chance),
    at whichever of the places the hashes voted for most has the most."""
    cands = [c[1:] for c in li.get(j, [])]
    cands += [(1 / s2, int(round(-d2 / s2)), -f2) for _, s2, d2, f2 in lj.get(i, [])]   # j in i, turned round to i in j
    # and unchanged in pitch and speed (a layer, reverb or a filter only), the best time found by sliding i along j
    cands.append((1.0, plain_time(pr, i, j), 0))
    # the raw share at each place first, then chance for the three best only
    raw = []
    for speed, dt0, df0 in cands:
        pi, _, dt1, df1 = presence(pr, i, j, speed, dt0, df0, chance=False)
        pj, _, _, _ = presence(pr, j, i, 1 / speed, int(round(-dt1 / speed)), -df1, chance=False)
        raw.append((max(pi, pj), speed, dt1, df1))
    found = []
    for _, speed, dt0, df0 in sorted(raw, key=lambda r: -r[0])[:3]:
        pi, zi, dt0, df0 = presence(pr, i, j, speed, dt0, df0)
        pj, zj, _, _ = presence(pr, j, i, 1 / speed, int(round(-dt0 / speed)), -df0)
        # how far above chance: 1 when every peak is there, 0 when no more are than at a wrong place
        ci = max(0.0, (pi - zi) / max(1 - zi, 1e-6))
        cj = max(0.0, (pj - zj) / max(1 - zj, 1e-6))
        if ci >= cj:
            found.append((i, j, ci, cj, speed, dt0, df0))
        else:
            found.append((j, i, cj, ci, 1 / speed, int(round(-dt0 / speed)), -df0))
    if not found:
        return None
    # of the places within 0.02 of the best, the plainest (least pitch and speed change)
    top = max(r[2] for r in found)
    best = min((r for r in found if r[2] >= top - 0.02), key=lambda r: abs(r[6]) + abs(np.log2(r[4])) * PER_OCT)
    return best


def wanted(pr, i, j):
    p, q = pr.clips[i], pr.clips[j]
    return p.game != q.game and not (p.extra and q.extra)


def candidates(pr, keep, looks=None):
    """The pairs worth the full check: those the gate passes either way (a layer, reverb or a filter, at the same pitch and
    speed), and each clip's VOTED others with the most hash votes for their size (a change of pitch or speed)."""
    out = set()
    for i in range(len(pr.clips)):
        best, mean = pr.gate(i)
        ex = (best - mean) / np.maximum(1 - mean, 1e-6)
        for j in np.nonzero(ex >= GATE)[0]:
            j = int(j)
            if j != i and keep(i, j):
                out.add((min(i, j), max(i, j)))
        votes = pr.lookup(i)
        ranked = sorted(votes, key=lambda o: -votes[o][0][0] / len(pr.h[o]) ** 0.5)
        mine = {int(j) for j in np.nonzero(ex >= GATE)[0]}
        for j in [o for o in ranked if keep(i, o)][:VOTED]:
            out.add((min(i, j), max(i, j)))
            mine.add(j)
        if looks is not None:
            # the votes kept for the full check: only those for the clips this one put forward
            looks[i] = {o: votes[o] for o in mine if o in votes}
        if i % 200 == 0:
            print("  gate %d/%d, %d pairs" % (i, len(pr.clips), len(out)), flush=True)
    return out


def name(c):
    return "%s:%s" % (c.game, c.ref)


def test(pr, threshold, back_line=0.15):
    d = json.load(open(DECISIONS, encoding="utf-8"))
    at = {name(c): n for n, c in enumerate(pr.clips)}
    rows = []
    cand = candidates(pr, lambda i, j: True)
    for k, verdict in sorted(d.items(), key=lambda kv: kv[1]):
        a, b = k.split("|")
        if a not in at or b not in at:
            rows.append((verdict, k, "not indexed"))
            continue
        i, j = at[a], at[b]
        li, lj = pr.lookup(i, [j]), pr.lookup(j, [i])
        r = judge_pair(pr, i, j, li, lj)
        if not r:
            rows.append((verdict, k, "no votes"))
            continue
        base, der, holds, back, speed, dt0, df0 = r
        why, facts = reasons(pr, base, der, speed, dt0, df0)
        checked = (min(i, j), max(i, j)) in cand
        rows.append((verdict, k, "%s %.2f/%.2f base %s; %s" % (("BASED" if checked else "UNCHECKED") if holds >= threshold and back >= back_line else "-", holds, back,
                                                                name(pr.clips[base]), ", ".join(why) or "-") +
                     ("  %s" % facts if os.environ.get("FACTS") else "")))
    for r in rows:
        print("%-12s %s\n             %s" % r)
    bj3 = [r for r in rows if r[0] == "not similar" and "Bejeweled3:" in r[1]]
    other = [r for r in rows if r[0] == "not similar" and "Bejeweled3:" not in r[1]]
    sim = [r for r in rows if r[0] == "similar"]
    found = lambda rs: sum(r[2].startswith("BASED") for r in rs)
    print("turned-down Bejeweled 3 pairs found as based on: %d of %d" % (found(bj3), len(bj3)))
    print("the other turned-down pairs found as based on: %d of %d" % (found(other), len(other)))
    print("pairs the user called similar found as based on: %d of %d" % (found(sim), len(sim)))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--assets")
    ap.add_argument("--extra", action="append", default=[])
    ap.add_argument("--test", action="store_true", help="score the pairs the user has judged")
    ap.add_argument("--min", type=float, default=0.50, help="the share of the base's peaks the derived must hold")
    ap.add_argument("--back", type=float, default=0.15, help="the share of the derived's peaks the base must hold")
    ap.add_argument("--refilter", action="store_true", help="filter the last run's based.json at these lines")
    a = ap.parse_args()
    if a.refilter:
        out = json.load(open(os.path.join(sm.OUT, "based-all.json"), encoding="utf-8"))
        write(prune([h for h in out if h["holds"] >= a.min and h["back"] >= a.back]))
        return
    if a.assets:
        vl.ASSETS = os.path.abspath(a.assets)
    clips = sm.load_clips(a.extra)
    if a.test:
        judged = {s for k in json.load(open(DECISIONS, encoding="utf-8")) for s in k.split("|")}
        clips = [c for c in clips if name(c) in judged]
    pr = Prints(clips)
    print("indexed %d clips, %d hashes" % (len(pr.clips), len(pr.key)), flush=True)
    if a.test:
        test(pr, a.min, a.back)
        return
    out = []
    looks = {}
    pairs = candidates(pr, lambda i, j: wanted(pr, i, j), looks)
    print("pairs to check: %d" % len(pairs), flush=True)
    for n, (i, j) in enumerate(sorted(pairs)):
        r = judge_pair(pr, i, j, looks.get(i, {}), looks.get(j, {}))
        if n % 2000 == 0:
            print("  %d/%d, %d found" % (n, len(pairs), len(out)), flush=True)
        if not r or r[2] < a.min:
            continue
        base, der, holds, back, speed, dt0, df0 = r
        why, facts = reasons(pr, base, der, speed, dt0, df0)
        out.append(dict(kind="based on", holds=round(holds, 3), back=round(back, 3), why=why, facts=facts,
                        base=sm.side(pr.clips[base]), derived=sm.side(pr.clips[der])))
    # the same recordings soundmatch found are left to it (read now: its run may end after this one starts)
    mf = os.path.join(sm.OUT, "matches.json")
    if os.path.exists(mf):
        same = {frozenset([name_d(h["a"]), name_d(h["b"])]) for h in json.load(open(mf, encoding="utf-8"))}
        out = [h for h in out if frozenset([name_d(h["base"]), name_d(h["derived"])]) not in same]
    json.dump(out, open(os.path.join(sm.OUT, "based-all.json"), "w", encoding="utf-8"), indent=1, ensure_ascii=False)
    write(prune([h for h in out if h["back"] >= a.back]))


def prune(out):
    """The generic hits' weaker pairs and the same recordings out (HUB, SAME)."""
    count = {}
    for h in out:
        for d in (h["base"], h["derived"]):
            count[name_d(d)] = count.get(name_d(d), 0) + 1
    return [h for h in out
            if (h["holds"] >= HUB_STRONG or (count[name_d(h["base"])] <= HUB and count[name_d(h["derived"])] <= HUB))
            and not (h["holds"] >= SAME and h["back"] >= SAME and not h["why"])]


def write(out):
    # strongest first by both ways at once: a pair that holds much one way and gives little back is weaker than one that
    # shares a good part of both (a click held whole in a longer notification sound comes after a reworked combo sound)
    out.sort(key=lambda h: -(h["holds"] + h["back"]))
    json.dump(out, open(os.path.join(sm.OUT, "based.json"), "w", encoding="utf-8"), indent=1, ensure_ascii=False)
    with open(os.path.join(sm.OUT, "based.txt"), "w", encoding="utf-8") as f:
        for h in out:
            f.write("%.2f/%.2f  %s %s [%s]  is based on  %s %s [%s]: %s\n" % (
                h["holds"], h["back"], h["derived"]["game"], h["derived"]["file"], h["derived"]["label"], h["base"]["game"],
                h["base"]["file"], h["base"]["label"], ", ".join(h["why"]) or "-"))
    print("based on: %d pairs; build/soundmatch/based.txt" % len(out))


def name_d(d):
    return "%s:%s" % (d["game"], d["file"])


if __name__ == "__main__":
    main()
