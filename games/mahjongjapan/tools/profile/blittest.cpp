// The faster software blits against the original loops, byte for byte, on random rows and on the kinds of pixel an image
// holds (opaque, empty, all alpha values), for Blend (not in place and in place), CopyRgb and SetRgb. The reference is the
// engine's Blit.cpp as it was before the performance pass (2026-11-09), built under another namespace:
//   git show 37b2cf77^:games/mahjongjapan/engine/Blit.cpp > BlitOld.cpp
//   clang++ -O3 -Djb=jbold -I engines/jollybear -c BlitOld.cpp -o BlitOld.o
//   clang++ -O3 -I engines/jollybear -c engines/jollybear/Blit.cpp -o BlitNew.o
//   clang++ -O3 -c games/mahjongjapan/tools/profile/blittest.cpp -o blittest.o
//   clang++ -static blittest.o BlitOld.o BlitNew.o -o blittest.exe && ./blittest.exe        (12000 cases, 0 differ)
// (the i686 llvm-mingw clang++ of the native build; no window, no sound)
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cstdint>

#include "../../../../engines/jollybear/Blit.h"
namespace jbold { namespace blit {
int Blend(const uint8_t* theSrc, int theSrcPitch, const uint8_t* theUnder, int theUnderPitch, uint8_t* theDst, int theDstPitch, int theWidth, int theHeight, int theMode);
int CopyRgb(const uint8_t* theSrc, int theSrcPitch, uint8_t* theDst, int theDstPitch, int theWidth, int theHeight);
int SetRgb(const uint8_t* theValue, uint8_t* theDst, int theDstPitch, int theWidth, int theHeight);
}}

static uint32_t gState = 12345;
static uint32_t Rand()
{
	gState = gState * 1664525u + 1013904223u;
	return gState >> 8;
}

static uint32_t Pixel(int theKind)
{
	switch (theKind)
	{
	case 0: return Rand() ^ (Rand() << 16);
	case 1: return 0xff000000u | (Rand() & 0xffffff);
	case 2: return 0;
	case 3: return (Rand() & 0xff) << 24;
	case 4: return (Rand() & 0xffffff);
	default: return ((Rand() & 1) ? 0xff000000u : 0) | ((Rand() & 1) ? 0xffffff : 0);
	}
}

int main()
{
	int aBad = 0, aCases = 0;
	for (int aCase = 0; aCase < 3000; aCase++)
	{
		int aW = 1 + Rand() % 70, aH = 1 + Rand() % 5;
		int aPitch = aW * 4 + 4 * (Rand() % 5);
		int aKind = aCase % 6;
		std::vector<uint8_t> aSrc(aPitch * aH), aUnder(aPitch * aH), aOld(aPitch * aH), aNew(aPitch * aH);
		for (int i = 0; i < aPitch * aH / 4; i++)
		{
			uint32_t s = Pixel(Rand() % 3 == 0 ? Rand() % 6 : aKind), u = Pixel(Rand() % 4 == 0 ? Rand() % 6 : 0);
			memcpy(&aSrc[i * 4], &s, 4);
			memcpy(&aUnder[i * 4], &u, 4);
		}
		// Blend into a copy of the under pixels and in place
		for (int aInPlace = 0; aInPlace < 2; aInPlace++)
		{
			aOld = aUnder;
			aNew = aUnder;
			if (aInPlace)
			{
				jbold::blit::Blend(aSrc.data(), aPitch, aOld.data(), aPitch, aOld.data(), aPitch, aW, aH, 6);
				jb::blit::Blend(aSrc.data(), aPitch, aNew.data(), aPitch, aNew.data(), aPitch, aW, aH, 6);
			}
			else
			{
				std::vector<uint8_t> aOut1(aPitch * aH, 0x5a), aOut2(aPitch * aH, 0x5a);
				jbold::blit::Blend(aSrc.data(), aPitch, aUnder.data(), aPitch, aOut1.data(), aPitch, aW, aH, 6);
				jb::blit::Blend(aSrc.data(), aPitch, aUnder.data(), aPitch, aOut2.data(), aPitch, aW, aH, 6);
				aOld = aOut1;
				aNew = aOut2;
			}
			aCases++;
			if (aOld != aNew)
				aBad++;
		}
		// CopyRgb and SetRgb
		aOld = aUnder;
		aNew = aUnder;
		jbold::blit::CopyRgb(aSrc.data(), aPitch, aOld.data(), aPitch, aW, aH);
		jb::blit::CopyRgb(aSrc.data(), aPitch, aNew.data(), aPitch, aW, aH);
		aCases++;
		if (aOld != aNew)
			aBad++;
		uint8_t aValue[4] = {(uint8_t)Rand(), (uint8_t)Rand(), (uint8_t)Rand(), (uint8_t)Rand()};
		aOld = aUnder;
		aNew = aUnder;
		jbold::blit::SetRgb(aValue, aOld.data(), aPitch, aW, aH);
		jb::blit::SetRgb(aValue, aNew.data(), aPitch, aW, aH);
		aCases++;
		if (aOld != aNew)
			aBad++;
	}
	printf("%d cases, %d differ\n", aCases, aBad);
	return aBad != 0;
}
