#!/usr/bin/env python3
"""Cuts a SoundFont 2 bank down to what some MIDI files play, for the games whose music is MIDI.

    python tools/sf2trim.py <bank.sf2> <out.sf2> --midi <file.mid> [<file.mid> ...]
        [--presets <bank>:<program>,...] [--all-zones] [--comment <text>]

The MIDI files are followed as TinySoundFont (third_party/tinysoundfont) plays them when a game sends it every
message: each channel starts on program 0 (channel 10 on the drum kits, bank 128), bank selects and program changes
pick presets with tsf_channel_set_presetnumber's fallbacks, and every note on is counted against the preset in use.
The new bank keeps those presets (and any --presets), their instruments and their samples, and of each preset only the
zones that a played key and velocity reach (--all-zones keeps every zone of a kept preset). Velocities are matched one
below as well, since TinySoundFont truncates velocity * 127.

The kept presets are printed and written into the new bank's INFO comment (ICMT) after the source bank's own, with
the source bank's name and this script's, so the bank itself records what it holds. The game files the MIDI comes from
are never in the repository; only the cut-down bank is (third_party/generaluser-gs/BYO-POP.md).
"""

import argparse
import os
import struct
import sys

GEN_INSTRUMENT = 41
GEN_KEYRANGE = 43
GEN_VELRANGE = 44
GEN_SAMPLEID = 53
SAMPLE_PADDING = 46  # zero sample points the format asks for after each sample


def read_chunks(data, start, end):
    pos = start
    while pos + 8 <= end:
        cid = data[pos:pos + 4]
        size = struct.unpack_from('<I', data, pos + 4)[0]
        yield cid, pos + 8, size
        pos += 8 + size + (size & 1)


# An INFO text: zero-terminated and padded with a zero to an even size, at most 65536 bytes
def info_string(payload):
    text = payload.split(b'\0')[0][:65534]
    return text + (b'\0\0' if len(text) % 2 == 0 else b'\0')


def make_chunk(cid, payload):
    return cid + struct.pack('<I', len(payload)) + payload + (b'\0' if len(payload) & 1 else b'')


class Bank:
    def __init__(self, path):
        data = open(path, 'rb').read()
        if data[0:4] != b'RIFF' or data[8:12] != b'sfbk':
            sys.exit('%s is not a SoundFont 2 bank' % path)
        self.info = []  # (id, payload) in their order
        self.smpl = b''
        self.sm24 = None
        pdta = {}
        for cid, pos, size in read_chunks(data, 12, len(data)):
            if cid != b'LIST':
                continue
            kind = data[pos:pos + 4]
            for sub, spos, ssize in read_chunks(data, pos + 4, pos + size):
                payload = data[spos:spos + ssize]
                if kind == b'INFO':
                    self.info.append((sub, payload))
                elif kind == b'sdta' and sub == b'smpl':
                    self.smpl = payload
                elif kind == b'sdta' and sub == b'sm24':
                    self.sm24 = payload
                elif kind == b'pdta':
                    pdta[sub] = payload

        def records(cid, fmt):
            size = struct.calcsize(fmt)
            raw = pdta[cid]
            return [struct.unpack_from(fmt, raw, i) for i in range(0, len(raw) - size + 1, size)]

        def raws(cid, size):
            raw = pdta[cid]
            return [raw[i:i + size] for i in range(0, len(raw) - size + 1, size)]

        self.phdr = records(b'phdr', '<20sHHHIII')
        self.pbag = records(b'pbag', '<HH')
        self.pmod = raws(b'pmod', 10)
        self.pgen = records(b'pgen', '<HH')
        self.inst = records(b'inst', '<20sH')
        self.ibag = records(b'ibag', '<HH')
        self.imod = raws(b'imod', 10)
        self.igen = records(b'igen', '<HH')
        self.shdr = records(b'shdr', '<20sIIIIIBbHH')

    def name(self):
        for cid, payload in self.info:
            if cid == b'INAM':
                return payload.split(b'\0')[0].decode('latin-1')
        return ''

    # The zones of a preset or an instrument: (generators, modulators) of each of its bags
    @staticmethod
    def zones(first, last, bags, gens, mods):
        return [(gens[bags[b][0]:bags[b + 1][0]], mods[bags[b][1]:bags[b + 1][1]]) for b in range(first, last)]

    def preset_zones(self, index):
        return self.zones(self.phdr[index][3], self.phdr[index + 1][3], self.pbag, self.pgen, self.pmod)

    def instrument_zones(self, index):
        return self.zones(self.inst[index][1], self.inst[index + 1][1], self.ibag, self.igen, self.imod)


def zone_ranges(gens):
    keys = (0, 127)
    vels = (0, 127)
    for oper, amount in gens:
        if oper == GEN_KEYRANGE:
            keys = (amount & 0xff, amount >> 8)
        elif oper == GEN_VELRANGE:
            vels = (amount & 0xff, amount >> 8)
    return keys, vels


def reaches(keys, vels, notes):
    return any(keys[0] <= k <= keys[1] and vels[0] <= v <= vels[1] for k, v in notes)


def find_preset(presets, bank, program, drums):
    # tsf_channel_set_presetnumber's order of lookups
    candidates = []
    if drums:
        candidates += [(128 | (bank & 0x7fff), program), (128, program), (128, 0), (bank & 0x7fff, program)]
    else:
        candidates.append((bank & 0x7fff, program))
    candidates.append((0, program))
    for key in candidates:
        if key in presets:
            return key
    return None


def read_varlen(data, pos):
    value = 0
    while True:
        byte = data[pos]
        pos += 1
        value = (value << 7) | (byte & 0x7f)
        if byte < 0x80:
            return value, pos


def midi_events(path):
    """The channel messages of a MIDI file in playing order: (tick, track, status, data1, data2)."""
    data = open(path, 'rb').read()
    if data[0:4] != b'MThd':
        sys.exit('%s is not a MIDI file' % path)
    events = []
    track = 0
    for cid, pos, size in read_chunks_midi(data):
        if cid != b'MTrk':
            continue
        end = pos + size
        tick = 0
        running = 0  # meta events and system exclusive messages do not change the running status
        while pos < end:
            delta, pos = read_varlen(data, pos)
            tick += delta
            status = running
            if data[pos] >= 0x80:
                status = data[pos]
                pos += 1
                if status < 0xf0:
                    running = status
            if status == 0xff:
                pos += 1
                length, pos = read_varlen(data, pos)
                pos += length
            elif status in (0xf0, 0xf7):
                length, pos = read_varlen(data, pos)
                pos += length
            elif status & 0xf0 in (0xc0, 0xd0):
                events.append((tick, track, len(events), status, data[pos], 0))
                pos += 1
            else:
                events.append((tick, track, len(events), status, data[pos], data[pos + 1]))
                pos += 2
        track += 1
    events.sort()
    return [(e[0], e[1], e[3], e[4], e[5]) for e in events]


def read_chunks_midi(data):
    # MIDI chunks are big-endian and not padded
    pos = 0
    while pos + 8 <= len(data):
        cid = data[pos:pos + 4]
        size = struct.unpack_from('>I', data, pos + 4)[0]
        yield cid, pos + 8, size
        pos += 8 + size


def played_notes(paths, presets):
    """{(bank, program): set of (key, velocity)} for the notes the files play."""
    notes = {}
    for path in paths:
        banks = [0] * 16
        current = [find_preset(presets, 0, 0, c == 9) for c in range(16)]
        for tick, track, status, data1, data2 in midi_events(path):
            channel = status & 0xf
            kind = status & 0xf0
            if kind == 0xb0 and data1 == 0:
                banks[channel] = 0x8000 | data2
            elif kind == 0xb0 and data1 == 32:
                banks[channel] = (((banks[channel] & 0x7f) << 7) if banks[channel] & 0x8000 else 0) | data2
            elif kind == 0xb0 and data1 == 121:
                banks[channel] = 0
            elif kind == 0xc0:
                found = find_preset(presets, banks[channel], data1, channel == 9)
                if found is not None:
                    current[channel] = found
            elif kind == 0x90 and data2 > 0 and current[channel] is not None:
                played = notes.setdefault(current[channel], set())
                played.add((data1, data2))
                played.add((data1, data2 - 1))
    return notes


def trim(bank, notes, extra, all_zones):
    presets = {(p[2], p[1]): i for i, p in enumerate(bank.phdr[:-1])}
    keep = {key: None if all_zones else played for key, played in notes.items()}
    for key in extra:
        if key not in presets:
            sys.exit('the bank has no preset %d:%d' % key)
        keep[key] = None

    out = {'phdr': [], 'pbag': [], 'pmod': [], 'pgen': [], 'inst': [], 'ibag': [], 'imod': [], 'igen': [],
           'shdr': []}
    instruments = {}  # old index -> new index
    samples = {}
    instrument_notes = {}  # old instrument -> notes that reach it (None: all)
    kept_zones = 0

    # Presets in the source bank's order
    for index in sorted(presets[k] for k in keep):
        name, program, bank_number, _, library, genre, morphology = bank.phdr[index]
        played = keep[(bank_number, program)]
        bag_start = len(out['pbag'])
        for z, (gens, mods) in enumerate(bank.preset_zones(index)):
            instrument = gens[-1][1] if gens and gens[-1][0] == GEN_INSTRUMENT else None
            keys, vels = zone_ranges(gens)
            if instrument is not None:
                if played is not None:
                    reaching = {(k, v) for k, v in played if keys[0] <= k <= keys[1] and vels[0] <= v <= vels[1]}
                    if not reaching:
                        continue
                    previous = instrument_notes.get(instrument, set())
                    if previous is not None:
                        instrument_notes[instrument] = previous | reaching
                else:
                    instrument_notes[instrument] = None
                if instrument not in instruments:
                    instruments[instrument] = None  # numbered below, in the source's order
            out['pbag'].append((gens, mods))
            kept_zones += 1
        out['phdr'].append((name, program, bank_number, bag_start, library, genre, morphology))

    for n, old in enumerate(sorted(instruments)):
        instruments[old] = n
    for old in sorted(instruments):
        name, _ = bank.inst[old]
        played = instrument_notes[old]
        bag_start = len(out['ibag'])
        for gens, mods in bank.instrument_zones(old):
            sample = gens[-1][1] if gens and gens[-1][0] == GEN_SAMPLEID else None
            if sample is not None:
                keys, vels = zone_ranges(gens)
                if played is not None and not reaches(keys, vels, played):
                    continue
                samples[sample] = None
            out['ibag'].append((gens, mods))
        out['inst'].append((name, bag_start))

    sample_order = sorted(samples)
    for n, old in enumerate(sample_order):
        samples[old] = n
    return out, instruments, samples, sample_order, kept_zones


def write_bank(bank, path, out, instruments, samples, sample_order, comment):
    smpl = bytearray()
    sm24 = bytearray() if bank.sm24 is not None else None
    shdr = b''
    for old in sample_order:
        name, start, end, loop_start, loop_end, rate, pitch, correction, link, kind = bank.shdr[old]
        new_start = len(smpl) // 2
        smpl += bank.smpl[start * 2:end * 2] + b'\0' * (SAMPLE_PADDING * 2)
        if sm24 is not None:
            sm24 += bank.sm24[start:end] + b'\0' * SAMPLE_PADDING
        shift = new_start - start
        new_end = new_start + end - start

        def moved(point):
            return min(max(point + shift, new_start), new_end)

        if kind & 0x6 and link not in samples:
            kind = (kind & 0x8000) | 1  # a stereo half whose partner is gone plays as mono
            link = 0
        else:
            link = samples.get(link, 0)
        shdr += struct.pack('<20sIIIIIBbHH', name, new_start, new_end, moved(loop_start), moved(loop_end), rate,
                            pitch, correction, link, kind)
    shdr += struct.pack('<20sIIIIIBbHH', b'EOS', 0, 0, 0, 0, 0, 0, 0, 0, 0)

    def zones_out(headers, bags, header_format, remap_oper, remap):
        header_data = b''
        bag_data = b''
        mod_data = b''
        gen_data = b''
        gen_count = 0
        mod_count = 0
        for gens, mods in bags:
            bag_data += struct.pack('<HH', gen_count, mod_count)
            for m in mods:
                mod_data += m
            mod_count += len(mods)
            for oper, amount in gens:
                if oper == remap_oper:
                    amount = remap[amount]
                gen_data += struct.pack('<HH', oper, amount)
            gen_count += len(gens)
        bag_data += struct.pack('<HH', gen_count, mod_count)
        mod_data += b'\0' * 10
        gen_data += struct.pack('<HH', 0, 0)
        for h in headers:
            header_data += struct.pack(header_format, *h)
        return header_data, bag_data, mod_data, gen_data

    phdr, pbag, pmod, pgen = zones_out(out['phdr'], out['pbag'], '<20sHHHIII', GEN_INSTRUMENT, instruments)
    phdr += struct.pack('<20sHHHIII', b'EOP', 0, 0, len(out['pbag']), 0, 0, 0)
    inst, ibag, imod, igen = zones_out(out['inst'], out['ibag'], '<20sH', GEN_SAMPLEID, samples)
    inst += struct.pack('<20sH', b'EOI', len(out['ibag']))

    info = b''
    has_comment = False
    for cid, payload in bank.info:
        if cid == b'ICMT':
            own = payload.split(b'\0')[0]
            payload = own + b'\n\n' + comment.encode('latin-1', 'replace')
            has_comment = True
        info += make_chunk(cid, payload if cid in (b'ifil', b'iver') else info_string(payload))
    if not has_comment:
        info += make_chunk(b'ICMT', info_string(comment.encode('latin-1', 'replace')))

    sdta = make_chunk(b'smpl', bytes(smpl))
    if sm24 is not None:
        sdta += make_chunk(b'sm24', bytes(sm24))
    pdta = (make_chunk(b'phdr', phdr) + make_chunk(b'pbag', pbag) + make_chunk(b'pmod', pmod) +
            make_chunk(b'pgen', pgen) + make_chunk(b'inst', inst) + make_chunk(b'ibag', ibag) +
            make_chunk(b'imod', imod) + make_chunk(b'igen', igen) + make_chunk(b'shdr', shdr))
    body = (b'sfbk' + make_chunk(b'LIST', b'INFO' + info) + make_chunk(b'LIST', b'sdta' + sdta) +
            make_chunk(b'LIST', b'pdta' + pdta))
    with open(path, 'wb') as f:
        f.write(b'RIFF' + struct.pack('<I', len(body)) + body)


def main():
    parser = argparse.ArgumentParser(description='Cuts a SoundFont 2 bank down to what some MIDI files play.')
    parser.add_argument('bank')
    parser.add_argument('out')
    parser.add_argument('--midi', nargs='*', default=[], help='the MIDI files whose notes are kept')
    parser.add_argument('--presets', default='', help='more presets to keep whole, as bank:program,...')
    parser.add_argument('--all-zones', action='store_true', help='keep every zone of a kept preset')
    parser.add_argument('--comment', default='', help='what the bank is for, the first line of its record')
    args = parser.parse_args()

    bank = Bank(args.bank)
    presets = {(p[2], p[1]) for p in bank.phdr[:-1]}
    notes = played_notes(args.midi, presets)
    extra = []
    for item in filter(None, args.presets.split(',')):
        b, p = item.split(':')
        extra.append((int(b), int(p)))
    if not notes and not extra:
        sys.exit('nothing to keep: give --midi files or --presets')
    out, instruments, samples, sample_order, kept_zones = trim(bank, notes, extra, args.all_zones)

    lines = []
    for name, program, bank_number, *_ in out['phdr']:
        how = 'whole' if (args.all_zones or (bank_number, program) in extra) else \
            '%d notes' % len({k for k, v in notes.get((bank_number, program), set())})
        lines.append('%03d:%03d %s (%s)' % (bank_number, program, name.split(b'\0')[0].decode('latin-1'), how))
    record = []
    if args.comment:
        record.append(args.comment)
    record.append('Cut down from %s (%s) by byo-pop\'s tools/sf2trim.py to %d presets, %d instruments and %d samples:'
                  % (bank.name(), os.path.basename(args.bank), len(out['phdr']), len(instruments), len(samples)))
    record += lines
    write_bank(bank, args.out, out, instruments, samples, sample_order, '\n'.join(record))

    print('\n'.join(record))
    print('%s: %d bytes (from %d)' % (args.out, os.path.getsize(args.out), os.path.getsize(args.bank)))


if __name__ == '__main__':
    main()
