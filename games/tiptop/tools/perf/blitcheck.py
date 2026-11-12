"""blitcheck.py: proves the software blitter's fast paths (lowos::BlitPixels) make the same pixels as its generic loop.

    python games/tiptop/tools/perf/blitcheck.py [cases]

Takes the function (with its helpers) from the committed tree (git HEAD, the generic loop alone) and from the working tree
(the fast paths in front of it), builds one test program that holds both, and runs random calls on copies of one buffer
(every flag combination, 1 to 4 bytes a pixel, same and different pixel formats, color keys that hit, levels 0 to 255,
pitches with padding, and sources that overlap the destination), comparing every byte of the buffer afterwards.
Needs a compiler (CXX, default the llvm-mingw clang++ the build uses).
"""
import os
import subprocess
import sys

ROOT = subprocess.run(['git', 'rev-parse', '--show-toplevel'], capture_output=True, text=True).stdout.strip()
SRC = 'games/tiptop/engine/Lowos.cpp'
CXX = os.environ.get('CXX', 'E:/retools/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-clang++.exe')


def extract(text):
    """From ScaleChannel to the end of BlitPixels: the helpers and the function."""
    i = text.index('// 0x43ba50: a channel value scaled')
    helpers_start = text.index('static uint32_t ReadPixel')
    j = text.index('// 0x430700 (ConvertBitmap)')
    return text[helpers_start:j]


def main():
    cases = sys.argv[1] if len(sys.argv) > 1 else '3000000'
    old = subprocess.run(['git', 'show', 'HEAD:' + SRC], capture_output=True, text=True, cwd=ROOT).stdout
    new = open(os.path.join(ROOT, SRC), encoding='utf-8').read()
    old, new = extract(old).replace('\r', ''), extract(new).replace('\r', '')
    prelude = '''#include "Lowos.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <vector>
using namespace lowos;
'''
    body = []
    for name, src in (('oldv', old), ('newv', new)):
        # lowos::ConvertColor (Lowos.h) would be found as well, by argument-dependent lookup, so each copy's is renamed
        body.append('namespace %s {\n%s\n}\n' % (name, src.replace('ConvertColor', 'ConvertColor_' + name)))
    main_src = r'''
static uint64_t gState = 88172645463325252ull;
static uint32_t Rnd() { gState ^= gState << 13; gState ^= gState >> 7; gState ^= gState << 17; return (uint32_t)(gState >> 11); }
int main(int argc, char** argv)
{
	long aCases = atol(argv[1]), aBad = 0;
	const PixelFormat aFormats[3] = {{8, 16, 8, 8, 8, 0}, {5, 11, 6, 5, 5, 0}, {8, 0, 8, 8, 8, 16}};
	const size_t kSize = 40000;
	std::vector<uint8_t> aBase(kSize), aOld(kSize), aNew(kSize);
	long aOverlap = 0, aFast = 0;
	for (long c = 0; c < aCases; c++)
	{
		bool aSmallRange = Rnd() % 3 == 0;   // few distinct byte values, so keys and equal pixels come up
		for (size_t i = 0; i < kSize; i++)
			aBase[i] = aSmallRange ? (uint8_t)(Rnd() % 3 * 100) : (uint8_t)Rnd();
		uint8_t aFlags = Rnd() % 4 == 0 ? (uint8_t)Rnd() : (uint8_t)(Rnd() % 2 ? (Rnd() & 0x1f) : (Rnd() & 0x09));
		if (Rnd() % 3 == 0) aFlags &= ~0x12;     // mostly plain, keyed and translucent
		uint8_t aSrcBpp = Rnd() % 4 == 0 ? (uint8_t)(1 + Rnd() % 4) : 4, aDestBpp = Rnd() % 4 == 0 ? (uint8_t)(1 + Rnd() % 4) : 4;
		const PixelFormat* aSrcFormat = &aFormats[Rnd() % 4 == 0 ? Rnd() % 3 : 0];
		const PixelFormat* aDestFormat = Rnd() % 4 == 0 ? &aFormats[Rnd() % 3] : aSrcFormat;
		int aWidth = Rnd() % 6 == 0 ? 0 : 1 + Rnd() % 40, aHeight = Rnd() % 8 == 0 ? 0 : 1 + Rnd() % 12;
		uint32_t aSrcPitch = (uint32_t)aWidth * (aSrcBpp + 1) + Rnd() % 20, aDestPitch = (uint32_t)aWidth * aDestBpp + Rnd() % 20;
		if (Rnd() % 5 == 0) aSrcPitch = aWidth * 4 + (Rnd() % 4 == 0 ? 0 : Rnd() % 12) ;
		bool anOverlap = Rnd() % 5 == 0;
		size_t aDestOff = 100 + Rnd() % 8000, aSrcOff;
		if (anOverlap)
		{
			long aDelta = (long)(Rnd() % 1200) - 600;
			if (Rnd() % 3 == 0) aDelta = (long)(Rnd() % 9) - 4;
			aSrcOff = (size_t)((long)aDestOff + aDelta);
		}
		else
			aSrcOff = (Rnd() % 2) ? 12000 + Rnd() % 8000 : (aDestOff > 9000 ? 100 : 9500 + Rnd() % 6000);
		if (aSrcOff + (size_t)aHeight * aSrcPitch + 8 > kSize || aDestOff + (size_t)aHeight * aDestPitch + 8 > kSize) continue;
		uint32_t aKey = Rnd() % 4 == 0 ? Rnd() : (uint32_t)aBase[aSrcOff] | aBase[aSrcOff + 1] << 8 | aBase[aSrcOff + 2] << 16 | (uint32_t)aBase[aSrcOff + 3] << 24;
		if (Rnd() % 4 == 0) aKey &= 0xffffff;
		uint8_t aLevel = Rnd() % 4 == 0 ? (uint8_t)(Rnd() % 2 ? 0xff : 0) : (uint8_t)Rnd();
		aOld = aBase; aNew = aBase;
		oldv::BlitPixels(aSrcFormat, aDestFormat, aOld.data() + aDestOff, aOld.data() + aSrcOff, aHeight, aWidth, aDestPitch, aSrcPitch, aFlags, aSrcBpp, aDestBpp, aKey, aLevel);
		newv::BlitPixels(aSrcFormat, aDestFormat, aNew.data() + aDestOff, aNew.data() + aSrcOff, aHeight, aWidth, aDestPitch, aSrcPitch, aFlags, aSrcBpp, aDestBpp, aKey, aLevel);
		aOverlap += anOverlap;
		if (aSrcBpp == 4 && aDestBpp == 4 && !(aFlags & 0x12) && aSrcFormat == aDestFormat) aFast++;
		if (aOld != aNew)
		{
			if (aBad++ < 5) printf("DIFFERENT: case %ld flags %02x bpp %d/%d %dx%d pitches %u/%u offsets %zu/%zu level %d\n", c, aFlags, aSrcBpp, aDestBpp, aWidth, aHeight, aSrcPitch, aDestPitch, aSrcOff, aDestOff, aLevel);
		}
	}
	printf("%ld random calls (%ld with a source overlapping the destination, %ld of the fast paths' kind), %ld different\n", aCases, aOverlap, aFast, aBad);
	return aBad != 0;
}
'''
    out = os.path.join(ROOT, 'build', 'perf', 'blitcheck.cpp')
    open(out, 'w', encoding='utf-8').write(prelude + ''.join(body) + main_src)
    exe = os.path.join(ROOT, 'build', 'perf', 'blitcheck.exe')
    r = subprocess.run([CXX, '-O1', '-std=c++17', '-static', '-I' + os.path.join(ROOT, 'games/tiptop/engine'), out, '-o', exe], capture_output=True, text=True)
    if r.returncode:
        print(r.stderr[-3000:])
        sys.exit(2)
    sys.exit(subprocess.run([exe, cases]).returncode)


main()
