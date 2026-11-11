"""Compares what the original Bejeweled 2 and the port play (sounds, and the music by song, order and volume), from the logs of
one scripted run each: Bejeweled's soundcmp.py (the same framework, DirectSound for the sounds and a BASS proxy for the music),
with one normalisation of the music's list for how the two sides write a song's change at the end of a fade-out.

When a faded-out song reaches volume 0 and a pending song is due (the menu's song, the board's), the original's
BassMusicInterface stops the song, sets its volume back to 100 and plays the new order, all in one update. The proxy's log
reads that as a "music volume 100" followed by "music play" (its stop is part of the play's setup calls), and the port logs
a "music stop" followed by "music play" (the framework's fade-out stops it, and a play sets the volume itself). Both are the
same sound, so the wrapper takes the volume-100 event out of the original's list and the stop out of the port's, where the play
of the same song follows in the same update.

    python games/bejeweled2/tools/origcmp/soundcmp.py <origrun run dir> <port run dir> [--times]
"""
import importlib.util
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SPEC = importlib.util.spec_from_file_location(
    "bejeweled_soundcmp", os.path.join(HERE, "..", "..", "..", "bejeweled", "tools", "origcmp", "soundcmp.py"))
B = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(B)
BASE_AUDIBLE = B.audible


def audible(music):
    music = BASE_AUDIBLE(music)
    out = []
    for k, e in enumerate(music):
        nxt = music[k + 1] if k + 1 < len(music) else None
        plays_next = nxt is not None and nxt[1] == "music play" and nxt[2] == e[2]
        # the volume reset comes a few ms before the play (a slow call in between); a port's stop comes in the play's update
        if plays_next and ((e[1] == "music stop" and nxt[0] == e[0]) or
                           (e[1] == "music volume" and e[3] == 100 and 0 <= nxt[0] - e[0] <= 5)):
            continue
        out.append(e)
    return out


B.audible = audible

if __name__ == "__main__":
    B.main()
