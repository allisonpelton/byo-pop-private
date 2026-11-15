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

The logging BASS runs on the "no sound" device, and there the original's love theme (`BASS_StreamCreateFile`, the first
song it loads, its file in the copy) comes back 0, so the original plays nothing for it. Such a song keeps its place in
the original's load order, the port's song at that place is named for it, and that song's events in the port are
counted apart rather than compared.

    python games/heavyweapon/tools/idplay/soundcmp.py <run folder> [--times]

--times adds each event's time: the original's in milliseconds of the stand-in's clock, the port's as its update.
"""
import importlib.util
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))


def load_module(name, path):
    """Big Money's soundcmp.py imports Bookworm Adventures' as `soundcmp`: loaded under that name itself, it would find
    itself there, so it is loaded under its own"""
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


BIG_MONEY = load_module('bigmoney_soundcmp', os.path.join(HERE, '..', '..', '..', 'bigmoney', 'tools', 'origcmp', 'soundcmp.py'))
SOUNDS = BIG_MONEY.S


def file_name(path):
    return os.path.basename(path.replace('\\', '/'))


def failed_streams(path):
    """the places in the original's load order (streams and MOD musics together) of the streams BASS did not make"""
    places, loads, pending = [], 0, None
    for line in open(path, errors='replace'):
        fields = line.split()
        if len(fields) < 4 or not fields[0].isdigit():
            continue
        if fields[2] in ('BASS_StreamCreateFile', 'BASS_MusicLoad'):
            pending = (fields[2], loads)
            loads += 1
        elif fields[2] == '=' and pending is not None and fields[3] == pending[0]:
            if pending[0] == 'BASS_StreamCreateFile' and int(fields[4], 16) == 0:
                places.append(pending[1])
            pending = None
    return places


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

    # the streams the original could not make keep their places in its load order; the port's events for them are set aside
    unmade = failed_streams(os.path.join(folder, 'orig_bass.log'))
    for place in unmade:
        original_songs.insert(place, '%s (not made)' % (port_songs[place] if place < len(port_songs) else '?'))
        original_music = [event[:2] + (event[2] + 1 if event[2] >= place else event[2],) + event[3:] for event in original_music]
    for place in unmade:
        set_aside = [event for event in port_music if event[2] == place]
        port_music = [event for event in port_music if event[2] != place]
        print("song %d (%s): the original's stream was not made on the no-sound device; the port's %d events for it set aside: %s"
              % (place, port_songs[place] if place < len(port_songs) else '?', len(set_aside),
                 ', '.join(sorted(set(event[1] for event in set_aside)))))

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
                       if index not in unmade and file_name(original).lower() != file_name(port).lower()]
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
