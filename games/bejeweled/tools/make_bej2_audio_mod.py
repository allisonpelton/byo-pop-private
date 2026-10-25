#!/usr/bin/env python3
"""Build the "Bejeweled 2 Audio" mod from the user's own Bejeweled 2 Deluxe install.

    python games/bejeweled/tools/make_bej2_audio_mod.py [--bej2 "<Bejeweled 2 Deluxe folder>"]
                                                        [--ffmpeg <ffmpeg.exe>]
                                                        [--out <mods folder, default games/bejeweled/mods>]

The result contains Bejeweled 2's copyrighted audio, so it is generated locally and git-ignored.

Music. Bejeweled 2 keeps all its music in one MO3 module (music/BeyondNetwork.mo3, "Beyond the Network"
by Peter Hajba) and plays it from different order offsets (properties/music.xml). The port's tracker
player cannot read MO3, so the needed sections are rendered with ffmpeg's libopenmpt demuxer (which
also emulates the module's DMO Echo) at 48 kHz:
  * title   (Bejeweled 1 "1980-autonomus.s3m") <- MAIN_MENU, subsong 2: 30 s intro + 100 s loop
  * normal  (Bejeweled 1 "1984-network.s3m")   <- MAIN, subsong 1: the 31-minute in-game suite
  * timed   (Bejeweled 1 "1981-datajack.s3m")  <- ACTION: the same suite entered at order 0x7A,
                                                  exactly as Bejeweled 2 does for Action mode
Loop points are exact sample positions: libopenmpt plays floor(48000 * 2.5 / tempo) samples per tick,
so a row-by-row walk of the module gives the sample where each section starts. They were computed once
from the module's order list and patterns (this matches libopenmpt's own subsong durations to the
sample). libopenmpt renders a fixed 0.1 s tail (4800 samples) after the end of a song; every subsong
shows exactly that, so each render is trimmed to the computed length to make the loop seamless.

Sounds. Bejeweled 1's sound names are kept; each is replaced by the matching Bejeweled 2 recording
(including the voice lines "Go", "Game over" and "Level complete"). Files that are identical in both
games are left alone.
"""
import argparse
import json
import os
import shutil
import struct
import subprocess
import sys

RATE = 48000
MUSIC = {
    # name: (subsong, expected total samples at 48 kHz, loop start sample)
    "bej2-menu.ogg": (2, 6237609, 1439232),
    "bej2-suite.ogg": (1, 89125409, 0),
}
ACTION_START = 52996720  # sample where the suite reaches order 0x7A (Bejeweled 2's MUSICOFFSET_ACTION)

# Bejeweled 1 sound name -> Bejeweled 2 file (in sounds/)
SOUNDS = {
    "gotset2": "gotset2",
    "gotsetbig2": "gotsetbig2",
    "combo22": "combo22",
    "combo32": "combo32",
    "combo42": "combo42",
    "combo52": "combo52",
    "gemongem2": "gemongem2",
    "explode2": "explode2",
    "menuclick": "menuclick",
    "multishot": "multishot",
    "go1": "Go",
    "levelup1": "Level_Complete",
    "gameover1": "Game_Over",
    "warning1": "warning1",
    "whirlpool1": "whirlpool1",
    "excellent1": "excellent1",
}

DEFAULT_INSTALLS = [
    r"A:\SteamLibrary\steamapps\common\Bejeweled 2 Deluxe",
    r"C:\Program Files (x86)\Steam\steamapps\common\Bejeweled 2 Deluxe",
    r"C:\Program Files\Steam\steamapps\common\Bejeweled 2 Deluxe",
]
DEFAULT_FFMPEG = [r"C:\Program Files\ShareX\ffmpeg.exe"]


def find_case(folder, name):
    """Case-insensitive lookup of a file name inside a folder."""
    for n in os.listdir(folder):
        if n.lower() == name.lower():
            return os.path.join(folder, n)
    return None


def find_ffmpeg(explicit):
    candidates = [explicit] if explicit else [shutil.which("ffmpeg")] + DEFAULT_FFMPEG
    for c in candidates:
        if not c or not os.path.isfile(c):
            continue
        demuxers = subprocess.run([c, "-hide_banner", "-demuxers"], capture_output=True, text=True).stdout
        encoders = subprocess.run([c, "-hide_banner", "-encoders"], capture_output=True, text=True).stdout
        if "libopenmpt" in demuxers and "libvorbis" in encoders:
            return c
    return None


def ogg_total_samples(path):
    """Granule position of the last Ogg page = total PCM frames of a Vorbis stream."""
    with open(path, "rb") as f:
        f.seek(0, 2)
        size = f.tell()
        f.seek(max(0, size - 65536))
        tail = f.read()
    i = tail.rfind(b"OggS")
    return struct.unpack_from("<q", tail, i + 6)[0] if i >= 0 else None


def render(ffmpeg, mo3, subsong, frames, out, quality):
    """Renders one subsong pass and trims it to exactly `frames` samples (drops libopenmpt's end tail)."""
    cmd = [ffmpeg, "-hide_banner", "-loglevel", "error", "-y", "-subsong", str(subsong), "-sample_rate", str(RATE),
           "-i", mo3, "-af", f"atrim=end_sample={frames}", "-c:a", "libvorbis", "-q:a", str(quality), out]
    subprocess.run(cmd, check=True)


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--bej2")
    ap.add_argument("--ffmpeg")
    ap.add_argument("--out", default=os.path.join(here, "..", "mods"))
    ap.add_argument("--quality", type=int, default=4, help="Vorbis quality for the music renders (0-10)")
    # Bejeweled 2's renders are louder (and peak above full scale); 0.63 matches the average loudness of
    # Bejeweled 1's three modules (RMS of the louder half of 400 ms blocks, both rendered by libopenmpt).
    ap.add_argument("--music-volume", type=float, default=0.63, help="gain applied to the rendered music tracks")
    args = ap.parse_args()

    bej2 = next((p for p in ([args.bej2] if args.bej2 else DEFAULT_INSTALLS) if p and os.path.isdir(p)), None)
    if not bej2 or not os.path.isdir(os.path.join(bej2, "sounds")):
        sys.exit("Bejeweled 2 Deluxe not found; pass --bej2 \"<install folder>\"")
    mo3 = find_case(os.path.join(bej2, "music"), "BeyondNetwork.mo3")
    if not mo3:
        sys.exit("music/BeyondNetwork.mo3 not found in the Bejeweled 2 install")
    ffmpeg = find_ffmpeg(args.ffmpeg)
    if not ffmpeg:
        sys.exit("Need an ffmpeg build with libopenmpt and libvorbis (pass --ffmpeg).")

    mod = os.path.join(os.path.abspath(args.out), "bejeweled2-audio")
    os.makedirs(os.path.join(mod, "music"), exist_ok=True)
    os.makedirs(os.path.join(mod, "sounds"), exist_ok=True)
    print(f"Bejeweled 2: {bej2}\nffmpeg: {ffmpeg}")

    for name, (subsong, expected, _) in MUSIC.items():
        out = os.path.join(mod, "music", name)
        print(f"rendering subsong {subsong} -> music/{name} ...", flush=True)
        render(ffmpeg, mo3, subsong, expected, out, args.quality)
        got = ogg_total_samples(out)
        status = "exact" if got == expected else f"MISMATCH (expected {expected}); loop points may be off"
        print(f"  {got} samples ({got / RATE:.3f} s), {os.path.getsize(out) / 1e6:.1f} MB: {status}")

    copied = 0
    for bej1_name, bej2_name in SOUNDS.items():
        src = find_case(os.path.join(bej2, "sounds"), bej2_name + ".ogg")
        if not src:
            print(f"  warning: sounds/{bej2_name}.ogg missing; keeping Bejeweled 1's {bej1_name}")
            continue
        shutil.copyfile(src, os.path.join(mod, "sounds", bej1_name + ".ogg"))
        copied += 1
    print(f"copied {copied} sounds")

    menu_loop = MUSIC["bej2-menu.ogg"][2] / RATE
    vol = args.music_volume
    manifest = {
        "id": "bejeweled2-audio",
        "name": "Bejeweled 2 Audio",
        "version": "1.0",
        "author": "generated from the local Bejeweled 2 Deluxe install",
        "description": "Bejeweled 2's music (Beyond the Network) and sound effects. Title screen: main menu theme; "
                       "Normal: the in-game suite; Timed: the suite from its Action-mode entry point.",
        "files": {
            "music/1980-autonomus.s3m": {"file": "music/bej2-menu.ogg", "loopStart": round(menu_loop, 6), "volume": vol},
            "music/1984-network.s3m": {"file": "music/bej2-suite.ogg", "loopStart": 0, "volume": vol},
            "music/1981-datajack.s3m": {"file": "music/bej2-suite.ogg", "start": round(ACTION_START / RATE, 6),
                                        "loopStart": 0, "volume": vol},
        },
    }
    with open(os.path.join(mod, "mod.json"), "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)
        f.write("\n")
    print(f"wrote {mod}")


if __name__ == "__main__":
    main()
