"""A small PE reader for Mahjong Escape's files: the headers, sections, imports and the resource tree.

Only what the staging script and the analysis need; the PE/COFF format is Microsoft's published one.
"""
import struct


class PE:
	def __init__(self, path):
		with open(path, 'rb') as f:
			self.data = f.read()
		d = self.data
		pe = struct.unpack_from('<I', d, 0x3c)[0]
		assert d[pe:pe + 4] == b'PE\0\0'
		self.machine, nsec, self.timestamp, _, _, optsize, self.characteristics = struct.unpack_from('<HHIIIHH', d, pe + 4)
		opt = pe + 24
		self.magic = struct.unpack_from('<H', d, opt)[0]
		self.linker = (d[opt + 2], d[opt + 3])
		self.entry = struct.unpack_from('<I', d, opt + 16)[0]
		self.image_base = struct.unpack_from('<I', d, opt + 28)[0]
		ndirs = struct.unpack_from('<I', d, opt + 92)[0]
		self.dirs = [struct.unpack_from('<II', d, opt + 96 + 8 * i) for i in range(ndirs)]
		self.sections = []
		s = opt + optsize
		for i in range(nsec):
			name, vsize, va, rsize, raw = struct.unpack_from('<8sIIII', d, s + 40 * i)
			self.sections.append((name.rstrip(b'\0').decode('latin1'), va, vsize, raw, rsize))

	def off(self, rva):
		for name, va, vsize, raw, rsize in self.sections:
			if va <= rva < va + max(vsize, rsize):
				return raw + rva - va
		raise ValueError(hex(rva))

	def cstr(self, rva):
		o = self.off(rva)
		return self.data[o:self.data.index(b'\0', o)].decode('latin1')

	def imports(self):
		rva = self.dirs[1][0]
		out = []
		while rva:
			ilt, _, _, name, iat = struct.unpack_from('<IIIII', self.data, self.off(rva))
			if not name:
				break
			names = []
			t = ilt or iat
			while True:
				v = struct.unpack_from('<I', self.data, self.off(t))[0]
				if not v:
					break
				names.append('#%d' % (v & 0xffff) if v & 0x80000000 else self.cstr(v + 2))
				t += 4
			out.append((self.cstr(name), iat, names))
			rva += 20
		return out

	def resources(self):
		"""Yields (type, name, lang, rva, size); ids are ints, names are strings."""
		base = self.dirs[2][0]
		if not base:
			return
		bo = self.off(base)
		d = self.data

		def entries(o):
			_, _, _, _, nnamed, nid = struct.unpack_from('<IIHHHH', d, bo + o)
			for i in range(nnamed + nid):
				n, t = struct.unpack_from('<II', d, bo + o + 16 + 8 * i)
				if n & 0x80000000:
					so = bo + (n & 0x7fffffff)
					ln = struct.unpack_from('<H', d, so)[0]
					key = d[so + 2:so + 2 + 2 * ln].decode('utf-16le')
				else:
					key = n
				yield key, t

		for typ, t1 in entries(0):
			for name, t2 in entries(t1 & 0x7fffffff):
				for lang, t3 in entries(t2 & 0x7fffffff):
					rva, size = struct.unpack_from('<II', d, bo + t3)[:2]
					yield typ, name, lang, rva, size

	def resource_data(self, rva, size):
		o = self.off(rva)
		return self.data[o:o + size]
