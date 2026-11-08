"""Re-creates the port's sound-effect mix from the game's files and its sound log, and compares it with the port's own
capture, so that a fault in decoding, rates, gains, pans or pitches shows as a difference (NOTES.md, "Sound").

    python games/bejeweledtwist/tools/sfxmix.py <capture.wav> <BYOPOP_SOUNDLOG file> [--ffmpeg <ffmpeg.exe>]

The port runs with -audiocapture=<capture.wav> (1/100 s of the mix per update) and BYOPOP_SOUNDLOG=<file>, with its
music at 0. Each source is decoded by ffmpeg (not by the port's decoder) at its own rate and played as the port's mixer
plays it: from the start of the update after the one that logged its "play", stepped by rate * pitch / 48000 with
linear interpolation, at DirectSound's gain for its logged hundredths of a dB and pan, until its "stop". The sum is
clamped at full scale as the mixer does. Prints the capture's and the re-creation's peaks, how many samples each
clamps, and the difference between them (its peak and its level against the mix's), and per effect the loudest
unclamped peak a single play reaches.
"""
import argparse
import os
import re
import subprocess
import sys
import wave

import numpy as np

RATE = 48000
FRAMES_PER_UPDATE = RATE // 100


def decode(path, ffmpeg):
    """the file at its own rate as float frames (n, 2), and the rate"""
    probe = subprocess.run([ffmpeg, '-hide_banner', '-i', path], capture_output=True, text=True).stderr
    m = re.search(r'(\d+) Hz, (mono|stereo)', probe)
    rate, ch = int(m.group(1)), (1 if m.group(2) == 'mono' else 2)
    raw = subprocess.run([ffmpeg, '-v', 'error', '-i', path, '-f', 's16le', '-acodec', 'pcm_s16le', '-'],
                         capture_output=True).stdout
    x = np.frombuffer(raw, dtype=np.int16).astype(np.float64).reshape(-1, ch) / 32768.0
    if ch == 1:
        x = np.repeat(x, 2, axis=1)
    return x, rate


def gain(cdb, pan):
    g = 0.0 if cdb <= -10000 else 1.0 if cdb >= 0 else 10 ** (cdb / 2000.0)
    left = right = g
    if pan > 0:
        left *= 10 ** (-pan / 2000.0)
    elif pan < 0:
        right *= 10 ** (pan / 2000.0)
    return left, right


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('capture')
    ap.add_argument('log')
    ap.add_argument('--ffmpeg', default='ffmpeg')
    ap.add_argument('--gamedir', default='assets/bejeweledtwist')
    a = ap.parse_args()

    w = wave.open(a.capture)
    cap = np.frombuffer(w.readframes(w.getnframes()), dtype=np.int16).astype(np.float64).reshape(-1, 2) / 32767.0
    total = len(cap)

    names, voices, live, cache = {}, [], {}, {}
    for line in open(a.log):
        p = line.split()
        if len(p) < 3:
            continue
        upd, what = int(p[0]), p[1]
        if what == 'load':
            names[int(p[2])] = p[3]
        elif what == 'play':
            sid, inst = int(p[2]), p[3]
            kv = dict(x.split('=') for x in p[4:])
            v = {'id': sid, 'start': upd, 'end': None, 'pitch': float(kv['pitch']),
                 'events': [(upd, int(kv['vol']), int(kv['pan']))]}
            voices.append(v)
            live[inst] = v
        elif what in ('volume', 'pan') and p[3] in live:
            v = live[p[3]]
            _, cdb, pan = v['events'][-1]
            if what == 'volume':
                cdb = int(p[4])
            else:
                pan = int(p[4])
            v['events'].append((upd, cdb, pan))
        elif what == 'pitch' and p[3] in live:
            live[p[3]]['pitch'] = float(p[4])
        elif what == 'stop' and p[3] in live:
            live.pop(p[3])['end'] = upd

    # update n's sounds start in the capture's update n + 1 (AudioCapture renders after each update): line up by the
    # first play, searching a few updates either way
    mix = np.zeros((total, 2))
    peaks = {}
    for v in voices:
        name = names[v['id']]
        if name not in cache:
            path = os.path.join(a.gamedir, name.replace('\\', '/') + '.ogg')
            cache[name] = decode(path, a.ffmpeg)
        src, rate = cache[name]
        # the log prints the pitch to 4 places; Twist's pitches are semitone steps, played at a whole Hz cut down
        # (SDLSoundInstance::AdjustPitch), which 4 places would drift from over a long sound
        semis = round(12 * np.log2(v['pitch']))
        pitch = np.trunc(rate * 2 ** (semis / 12.0)) / rate if abs(2 ** (semis / 12.0) - v['pitch']) < 1e-3 else v['pitch']
        step = rate * pitch / RATE
        start = v['start'] * FRAMES_PER_UPDATE
        end = total if v['end'] is None else min(total, v['end'] * FRAMES_PER_UPDATE)
        n = min(end - start, int((len(src) - 1) / step))
        if n <= 0:
            continue
        pos = np.arange(n) * step
        i0 = pos.astype(int)
        t = (pos - i0)[:, None]
        seg = src[i0] + (src[np.minimum(i0 + 1, len(src) - 1)] - src[i0]) * t
        g = np.zeros((n, 2))
        for k, (u, cdb, pan) in enumerate(v['events']):
            frm = max(0, u * FRAMES_PER_UPDATE - start)
            g[frm:] = gain(cdb, pan)
        out = seg * g
        mix[start:start + n] += out
        peaks[name] = max(peaks.get(name, 0.0), float(np.abs(out).max()))
    rec = np.clip(mix, -1, 1)

    # the best shift of the re-creation against the capture, within 3 updates
    best = None
    for s in range(-3 * FRAMES_PER_UPDATE, 3 * FRAMES_PER_UPDATE + 1, 16):
        a0, b0 = max(0, s), max(0, -s)
        n = min(total - a0, total - b0)
        d = np.abs(cap[a0:a0 + n] - rec[b0:b0 + n]).mean()
        if best is None or d < best[0]:
            best = (d, s)
    s = best[1]
    a0, b0 = max(0, s), max(0, -s)
    n = min(total - a0, total - b0)
    diff = cap[a0:a0 + n] - rec[b0:b0 + n]
    lvl = lambda x: 20 * np.log10(np.sqrt((x ** 2).mean()) + 1e-12)
    print('capture: peak %.3f, %d samples at full scale' % (np.abs(cap).max(), int((np.abs(cap) >= 0.999).sum())))
    print('re-creation: peak before the clamp %.3f, %d samples clamped' % (np.abs(mix).max(), int((np.abs(mix) > 1).sum())))
    print('shift %d frames; difference: peak %.4f, %.1f dB against the capture\'s %.1f dB'
          % (s, np.abs(diff).max(), lvl(diff), lvl(cap)))
    print('loudest single play of each effect (gain applied):')
    for name, pk in sorted(peaks.items(), key=lambda kv: -kv[1])[:15]:
        print('  %.3f  %s' % (pk, name))


if __name__ == '__main__':
    main()
