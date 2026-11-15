#!/usr/bin/env python3
"""Compares what the original Heavy Weapon and the port play, by order and state, from the folder of one `idplay.py sound`
run: orig_sound.log (origrun's DirectSound stand-in: the sounds), orig_bass.log (the logging BASS beside the copy's exe:
the music) and port_sound.log (the port's BYOPOP_SOUNDLOG).

Heavy Weapon's BASS is the 2.0 Big Money has, called through the same functions, so each log is read as Big Money's
soundcmp.py reads it (games/bigmoney/tools/origcmp/soundcmp.py, which reads the sounds as Bookworm Adventures' does):

- the sounds are paired by the order they are loaded, and each pair's length, rate and channels checked;
- every play of a sound is compared in order, with its volume, pan, pitch and looping (stops are not: the original's
  duplicates are freed when next needed, see Bejeweled's soundcmp.py);
- the music's plays (with the order they start at), its volumes when they change, its stops, pauses and resumes, the songs
  by load order on both sides.

    python games/heavyweapon/tools/idplay/soundcmp.py <run folder> [--times]

--times adds each event's time: the original's in milliseconds of the stand-in's clock, the port's as its update.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', '..', '..', 'bigmoney', 'tools', 'origcmp'))
import soundcmp as BIG_MONEY  # noqa: E402
SOUNDS = BIG_MONEY.S


def file_name(path):
    return os.path.basename(path.replace('\\', '/'))


def main():
    args = [arg for arg in sys.argv[1:] if not arg.startswith('--')]
    times = '--times' in sys.argv
    if len(args) != 1:
        sys.exit(__doc__)
    folder = args[0]
    original_sounds, original_loads = SOUNDS.orig_sounds(os.path.join(folder, 'orig_sound.log'))
    original_music, original_songs = BIG_MONEY.orig_music(os.path.join(folder, 'orig_bass.log'))
    port_log = os.path.join(folder, 'port_sound.log')
    port_sounds, port_loads = SOUNDS.port_sounds(port_log)
    port_music, port_songs = BIG_MONEY.port_music(port_log)

    original_loads = [load for load in original_loads if load is not None]
    differing = 0
    for index, (original, port) in enumerate(zip(original_loads, port_loads)):
        size, rate, bits, channels = original
        frames = size // (bits // 8 * channels) if bits and channels else -1
        if (frames, rate, channels) != (port[2], port[3], port[4]):
            differing += 1
            print('sound %d: original %d frames %d Hz %d ch, port %s %d frames %d Hz %d ch' %
                  (index, frames, rate, channels, port[1], port[2], port[3], port[4]))
    print('sounds loaded: original %d, port %d, %d pairs differ' % (len(original_loads), len(port_loads), differing))
    songs_differing = [index for index, (original, port) in enumerate(zip(original_songs, port_songs))
                       if file_name(original).lower() != file_name(port).lower()]
    for index in songs_differing:
        print('song %d: original %s, port %s' % (index, original_songs[index], port_songs[index]))
    print('songs loaded: original %d, port %d, %d names differ' % (len(original_songs), len(port_songs), len(songs_differing)))

    # the original's sounds are named by load order, the port's by id; both through the port's file names
    by_load = dict((index, file_name(port[1])) for index, port in enumerate(port_loads))
    by_id = dict((port[0], file_name(port[1])) for port in port_loads)
    same_plays = SOUNDS.compare('sound plays', [event for event in original_sounds if event[1] == 'play'],
                                [event for event in port_sounds if event[1] == 'play'],
                                lambda sound: by_load.get(sound, 'o%s' % sound), lambda sound: by_id.get(sound, 'p%s' % sound),
                                times)
    same_music = SOUNDS.compare('music', original_music, port_music, lambda song: 'song%s' % song,
                                lambda song: 'song%s' % song, times)
    same = (same_plays and same_music and not differing and not songs_differing and len(original_loads) == len(port_loads)
            and len(original_songs) == len(port_songs))
    print('soundcmp: %s' % ('same' if same else 'DIFFERENT'))
    return 0 if same else 1


if __name__ == '__main__':
    sys.exit(main())
