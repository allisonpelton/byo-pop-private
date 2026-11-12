"""Reads Venice's exe image: the sections' addresses and the bytes at a virtual address.

    import pe
    image = pe.Image()
    image.u32(0x586190)      # the dword at a virtual address
    image.bytes(0x401000, 16)
    image.section(0x586190)  # '.rdata'
"""
import os
import struct

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
EXE = os.path.join(ROOT, "re", "venice", "bin", "VeniceGame.exe")


class Image:
    def __init__(self, path=EXE):
        self.data = open(path, "rb").read()
        header = struct.unpack_from("<I", self.data, 0x3C)[0]
        count = struct.unpack_from("<H", self.data, header + 6)[0]
        optional = struct.unpack_from("<H", self.data, header + 20)[0]
        self.base = struct.unpack_from("<I", self.data, header + 24 + 28)[0]
        self.sections = []
        offset = header + 24 + optional
        for _ in range(count):
            name = self.data[offset:offset + 8].rstrip(b"\0").decode("latin-1")
            virtual_size, virtual, raw_size, raw = struct.unpack_from("<IIII", self.data, offset + 8)
            self.sections.append((name, self.base + virtual, virtual_size, raw, raw_size))
            offset += 40

    def section(self, va):
        for name, start, size, raw, raw_size in self.sections:
            if start <= va < start + size:
                return name
        return None

    def offset(self, va):
        for name, start, size, raw, raw_size in self.sections:
            if start <= va < start + min(size, raw_size):
                return raw + va - start
        return None

    def bytes(self, va, count):
        offset = self.offset(va)
        return self.data[offset:offset + count]

    def u32(self, va):
        return struct.unpack_from("<I", self.data, self.offset(va))[0]

    def region(self, name):
        for section, start, size, raw, raw_size in self.sections:
            if section == name:
                return start, start + size
        raise KeyError(name)

    def virtual_address(self, offset):
        for name, start, size, raw, raw_size in self.sections:
            if raw <= offset < raw + raw_size:
                return start + offset - raw
        return None
