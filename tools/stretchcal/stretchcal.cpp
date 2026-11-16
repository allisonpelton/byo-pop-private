// stretchcal: what the renderer's present of a software-drawn frame looks like, pixel for pixel, against a computation on the
// processor (docs/MEMORY.md, "A smooth stretch on the processor"). It draws test pictures through the same SDL calls as
// SurfaceDisplay's renderer mode: the frame into a texture of its own size, that into the screen texture at the render scale
// with the nearest filter (each pixel repeated, mPixelPerfect), and the screen texture into the window's presentation rect
// with the linear filter (DDInterface::Present). It reads the window's back buffer before the present and compares it with
// each model of the linear filter in turn, then with framework/LinearStretch (the one the display uses).
//
//   build/native/rundesk.exe build/native/stretchcal.exe [-src=WxH,...] [-win=WxH,...] [-out=<dir>]
//   build/native/stretchcal.exe -selftest     (Draw against DrawReference on the processor, no window)
//   build/native/stretchcal.exe -bench        (Draw's time for a whole rect, no window)
//
// The defaults: frames of 640x480 and 800x600; windows of 3840x2160, 1920x1080, 1000x750 and 500x375 (a factor below 1). One
// line a case: the pattern, the sizes, the render scale, and for each model the pixels that differ and the largest
// difference in a channel; "LinearStretch" last. With -out the renderer's pictures are written as BMP.

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "DisplayGeometry.h"
#include "LinearStretch.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <set>
#include <vector>

using namespace Sexy;

namespace
{

struct Size
{
	int mWidth;
	int mHeight;
};

std::vector<Size> ParseSizes(const char* theText)
{
	std::vector<Size> aSizes;
	const char* p = theText;
	while (*p != 0)
	{
		Size aSize = { 0, 0 };
		if (sscanf(p, "%dx%d", &aSize.mWidth, &aSize.mHeight) == 2)
			aSizes.push_back(aSize);
		const char* aComma = strchr(p, ',');
		if (aComma == NULL)
			break;
		p = aComma + 1;
	}
	return aSizes;
}

// the test pictures, 0x00RRGGBB
std::vector<uint32_t> MakePattern(int thePattern, int theWidth, int theHeight)
{
	std::vector<uint32_t> aPixels((size_t) theWidth * theHeight);
	uint32_t aSeed = 12345;
	for (int y = 0; y < theHeight; y++)
	{
		for (int x = 0; x < theWidth; x++)
		{
			uint32_t c = 0;
			switch (thePattern)
			{
			case 0:  // gradients: red across, green down, blue in steps of one along the diagonal
				c = ((x * 255 / (theWidth - 1)) << 16) | ((y * 255 / (theHeight - 1)) << 8) | ((x + y) & 255);
				break;
			case 1:  // single-pixel lines on black: white every 7 columns, yellow every 5 rows, a red one-pixel grid in a corner
				if (x % 7 == 3)
					c = 0xFFFFFF;
				else if (y % 5 == 2)
					c = 0xFFFF00;
				else if ((x < 64) && (y < 64) && (((x & 1) == 0) || ((y & 1) == 0)))
					c = 0xFF0000;
				break;
			case 2:  // checkerboards: of one pixel on the left half, of two on the right, blue and orange below
				if (x < theWidth / 2)
					c = (((x + y) & 1) != 0) ? 0xFFFFFF : 0x000000;
				else
					c = ((((x >> 1) + (y >> 1)) & 1) != 0) ? 0xFFFFFF : 0x000000;
				if (y >= theHeight * 3 / 4)
					c = (((x + y) & 1) != 0) ? 0x2050F0 : 0xF08020;
				break;
			case 4:  // columns: each column one colour from a hash of x (any weight between columns shows in every row)
				c = ((uint32_t) x * 2654435761u >> 8) & 0xFFFFFF;
				break;
			case 5:  // rows: each row one colour from a hash of y
				c = ((uint32_t) y * 2654435761u >> 8) & 0xFFFFFF;
				break;
			default:  // noise
				aSeed = aSeed * 1103515245 + 12345;
				c = (aSeed >> 8) & 0xFFFFFF;
				break;
			}
			aPixels[(size_t) y * theWidth + x] = c;
		}
	}
	return aPixels;
}

const char* kPatternNames[] = { "gradients", "lines", "checkers", "noise", "columns", "rows" };
const int kPatternCount = 6;

// A model of the linear filter: where an output pixel's centre falls among the texels, how finely the weight between two
// texels is kept, and how the blend is rounded to 8 bits
enum { ROUND_HALF_UP, ROUND_DOWN, ROUND_HALF_DOWN };
struct Model
{
	const char* mName;
	bool mFloatCoords;   // the texel position computed in 32-bit floats from the interpolated texture coordinate
	int mWeightBits;     // 0: the weight unquantised; else the fraction kept to that many bits
	bool mRoundWeight;   // the fraction rounded to those bits (else truncated)
	int mRoundOutput;    // how the blend comes to a level
	bool mRoundBetween;  // the columns' blend rounded to a level the same way before the rows' (else kept exact)
	int mCoordBits;      // 0: the texture coordinate exact; else kept to that many fractional bits before it is scaled to texels
	bool mRoundCoord;    // the coordinate rounded to those bits (else truncated)
	bool mCardFilter;    // the card's integer filter (LinearStretch's arithmetic), mRoundOutput and mRoundBetween unused
};

const Model kModels[] = {
	{ "dbl-exact-hu", false, 0, false, ROUND_HALF_UP, false },
	{ "dbl-8floor-hu", false, 8, false, ROUND_HALF_UP, false },
	{ "dbl-8round-hu", false, 8, true, ROUND_HALF_UP, false },
	{ "dbl-8round-hd", false, 8, true, ROUND_HALF_DOWN, false },
	{ "flt-8round-hd", true, 8, true, ROUND_HALF_DOWN, false },
	{ "dbl-8round-hd-sep", false, 8, true, ROUND_HALF_DOWN, true },
	{ "flt-8round-hd-sep", true, 8, true, ROUND_HALF_DOWN, true },
	{ "dbl-8round-down", false, 8, true, ROUND_DOWN, false },
	{ "dbl-9round-hd", false, 9, true, ROUND_HALF_DOWN, false },
	{ "u20t-8round-hd", false, 8, true, ROUND_HALF_DOWN, false, 20, false },
	{ "u21t-8round-hd", false, 8, true, ROUND_HALF_DOWN, false, 21, false },
	{ "u22t-8round-hd", false, 8, true, ROUND_HALF_DOWN, false, 22, false },
	{ "u21r-8round-hd", false, 8, true, ROUND_HALF_DOWN, false, 21, true },
	{ "u21t-8floor-hd", false, 8, false, ROUND_HALF_DOWN, false, 21, false },
	{ "u21t-8round-hu", false, 8, true, ROUND_HALF_UP, false, 21, false },
	{ "card", false, 8, true, ROUND_HALF_DOWN, false, 21, false, true },
};
const int kModelCount = sizeof(kModels) / sizeof(kModels[0]);

double RoundLevel(int theMode, double v)
{
	if (theMode == ROUND_DOWN)
		return std::floor(v);
	if (theMode == ROUND_HALF_DOWN)
		return std::ceil(v - 0.5);
	return std::floor(v + 0.5);
}

// one axis: for each output pixel, the two source pixels (the frame's, after the render scale's repeat) and the weight of
// the second
struct Tap
{
	int mA;
	int mB;
	double mWeight;
	double mRaw;  // the exact position's fraction in steps of the weight (before it is kept to mWeightBits)
};

std::vector<Tap> MakeTaps(const Model& theModel, int theSourceSize, int theRenderScale, int theOutputSize)
{
	const int aTexels = theSourceSize * theRenderScale;
	std::vector<Tap> aTaps(theOutputSize);
	for (int i = 0; i < theOutputSize; i++)
	{
		double aPos;
		if (theModel.mFloatCoords)
		{
			const float u = ((float) i + 0.5f) / (float) theOutputSize;
			aPos = (double) (u * (float) aTexels - 0.5f);
		}
		else if (theModel.mCoordBits > 0)
		{
			const double aSteps = std::ldexp(1.0, theModel.mCoordBits);
			const double u = ((double) i + 0.5) / theOutputSize * aSteps;
			aPos = (theModel.mRoundCoord ? std::floor(u + 0.5) : std::floor(u)) / aSteps * aTexels - 0.5;
		}
		else
			aPos = ((double) i + 0.5) * aTexels / theOutputSize - 0.5;
		double aFloor = std::floor(aPos);
		double aFrac = aPos - aFloor;
		const double aRaw = aFrac * (double) (1 << std::max(1, theModel.mWeightBits));
		if (theModel.mWeightBits > 0)
		{
			const double aSteps = (double) (1 << theModel.mWeightBits);
			double aQ = theModel.mRoundWeight ? std::floor(aFrac * aSteps + 0.5) : std::floor(aFrac * aSteps);
			if (aQ >= aSteps)
			{
				aQ = 0;
				aFloor += 1;
			}
			aFrac = aQ / aSteps;
		}
		int aT0 = (int) aFloor;
		int aT1 = aT0 + 1;
		aT0 = std::max(0, std::min(aTexels - 1, aT0));
		aT1 = std::max(0, std::min(aTexels - 1, aT1));
		aTaps[i].mA = aT0 / theRenderScale;
		aTaps[i].mB = aT1 / theRenderScale;
		aTaps[i].mWeight = aFrac;
		aTaps[i].mRaw = aRaw;
	}
	return aTaps;
}

void ModelStretch(const Model& theModel, const std::vector<uint32_t>& theSrc, int theSrcWidth, int theSrcHeight, int theRenderScale,
	int theOutWidth, int theOutHeight, std::vector<uint32_t>& theOut)
{
	std::vector<Tap> aCols = MakeTaps(theModel, theSrcWidth, theRenderScale, theOutWidth);
	std::vector<Tap> aRows = MakeTaps(theModel, theSrcHeight, theRenderScale, theOutHeight);
	theOut.resize((size_t) theOutWidth * theOutHeight);
	for (int y = 0; y < theOutHeight; y++)
	{
		const Tap& r = aRows[y];
		const uint32_t* aRowA = &theSrc[(size_t) r.mA * theSrcWidth];
		const uint32_t* aRowB = &theSrc[(size_t) r.mB * theSrcWidth];
		for (int x = 0; x < theOutWidth; x++)
		{
			const Tap& c = aCols[x];
			uint32_t aOut = 0;
			if (theModel.mCardFilter)
			{
				// the four weights: the product of the two rounded to 8 bits, the other three from it so that they sum to 256;
				// the sum to 12 bits by 257/4096, then to 8 bits rounding half down
				const int wx = (int) std::lround(c.mWeight * 256);
				const int wy = (int) std::lround(r.mWeight * 256);
				const int w11 = (wx * wy + 128) >> 8;
				const int w10 = wx - w11;
				const int w01 = wy - w11;
				const int w00 = 256 - wx - wy + w11;
				for (int aShift = 0; aShift <= 16; aShift += 8)
				{
					const int n = (int) ((aRowA[c.mA] >> aShift) & 255) * w00 + (int) ((aRowA[c.mB] >> aShift) & 255) * w10 +
						(int) ((aRowB[c.mA] >> aShift) & 255) * w01 + (int) ((aRowB[c.mB] >> aShift) & 255) * w11;
					const int q = (n * 257 + 128) >> 12;
					aOut |= (uint32_t) ((q * 255 + 2047) >> 12) << aShift;
				}
				theOut[(size_t) y * theOutWidth + x] = aOut;
				continue;
			}
			for (int aShift = 0; aShift <= 16; aShift += 8)
			{
				const double c00 = (aRowA[c.mA] >> aShift) & 255;
				const double c10 = (aRowA[c.mB] >> aShift) & 255;
				const double c01 = (aRowB[c.mA] >> aShift) & 255;
				const double c11 = (aRowB[c.mB] >> aShift) & 255;
				double aTop = c00 + (c10 - c00) * c.mWeight;
				double aBottom = c01 + (c11 - c01) * c.mWeight;
				if (theModel.mRoundBetween)
				{
					aTop = RoundLevel(theModel.mRoundOutput, aTop);
					aBottom = RoundLevel(theModel.mRoundOutput, aBottom);
				}
				const double v = aTop + (aBottom - aTop) * r.mWeight;
				int aLevel = (int) RoundLevel(theModel.mRoundOutput, v);
				aLevel = std::max(0, std::min(255, aLevel));
				aOut |= (uint32_t) aLevel << aShift;
			}
			theOut[(size_t) y * theOutWidth + x] = aOut;
		}
	}
}

struct Diff
{
	long long mPixels;
	int mLargest;
	int mX;
	int mY;
};

Diff Compare(const std::vector<uint32_t>& theWanted, const uint32_t* theShown, int theWidth, int theHeight, int theShownPitch)
{
	Diff d = { 0, 0, -1, -1 };
	for (int y = 0; y < theHeight; y++)
	{
		for (int x = 0; x < theWidth; x++)
		{
			const uint32_t a = theWanted[(size_t) y * theWidth + x];
			const uint32_t b = theShown[(size_t) y * theShownPitch + x];
			int aLargest = 0;
			for (int aShift = 0; aShift <= 16; aShift += 8)
				aLargest = std::max(aLargest, std::abs((int) ((a >> aShift) & 255) - (int) ((b >> aShift) & 255)));
			if (aLargest > 0)
			{
				if (d.mPixels == 0)
				{
					d.mX = x;
					d.mY = y;
				}
				d.mPixels++;
				d.mLargest = std::max(d.mLargest, aLargest);
			}
		}
	}
	return d;
}

void SaveBmp(const std::string& thePath, const uint32_t* thePixels, int theWidth, int theHeight, int thePitch)
{
	SDL_Surface* aSurface = SDL_CreateRGBSurfaceWithFormat(0, theWidth, theHeight, 32, SDL_PIXELFORMAT_ARGB8888);
	if (aSurface == NULL)
		return;
	for (int y = 0; y < theHeight; y++)
		memcpy((uint8_t*) aSurface->pixels + (size_t) y * aSurface->pitch, thePixels + (size_t) y * thePitch, (size_t) theWidth * 4);
	SDL_SaveBMP(aSurface, thePath.c_str());
	SDL_FreeSurface(aSurface);
}

} // namespace

int main(int argc, char* argv[])
{
	std::vector<Size> aSources = ParseSizes("640x480,800x600");
	std::vector<Size> aWindows = ParseSizes("3840x2160,1920x1080,1000x750,500x375");
	std::string anOut;
	int aDumpRow = -1;
	std::string aPairsPath;  // -pairs=<file>: with -diag, every blend clear of rounding boundaries, channel by channel, with the card's level
	std::set<uint64_t> aSinglePairs, aDoublePairs;
	std::string aWeightsPath;  // -weights=<file>: with -diag, appended: each column's and row's weights that agree with the card
	int aDiagKind = -1;   // -kind=<1 columns, 2 rows, 3 both>: show only the misses of that kind clear of rounding boundaries
	int aDiagModel = -1;  // -diag=<model index>  // -dump=<row>: the checkers' row of the output rect, column by column (taps, exact weight, the card's level)
	for (int i = 1; i < argc; i++)
	{
		if (strncmp(argv[i], "-pairs=", 7) == 0)
			aPairsPath = argv[i] + 7;
		if (strncmp(argv[i], "-weights=", 9) == 0)
			aWeightsPath = argv[i] + 9;
		if (strncmp(argv[i], "-kind=", 6) == 0)
			aDiagKind = atoi(argv[i] + 6);
		if (strncmp(argv[i], "-diag=", 6) == 0)
			aDiagModel = atoi(argv[i] + 6);
		if (strncmp(argv[i], "-dump=", 6) == 0)
			aDumpRow = atoi(argv[i] + 6);
		if (strncmp(argv[i], "-src=", 5) == 0)
			aSources = ParseSizes(argv[i] + 5);
		else if (strncmp(argv[i], "-win=", 5) == 0)
			aWindows = ParseSizes(argv[i] + 5);
		else if (strncmp(argv[i], "-out=", 5) == 0)
			anOut = argv[i] + 5;
	}

	// -selftest: LinearStretch::Draw (copies through a table, SSE2 blends) against DrawReference (the arithmetic pixel by
	// pixel), to the byte: whole rects drawn in bands as GdiStretch::PresentLinear draws them, and random parts of them, for
	// frames and windows of many sizes and every pattern; and the SSE2 levels against the table for every sum. No window.
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-selftest") != 0)
			continue;
		const std::vector<Size> aTestSources = ParseSizes("640x480,800x600,320x240,1024x768,512x384");
		const std::vector<Size> aTestWindows = ParseSizes("3840x2160,2560x1440,1920x1080,1920x1200,1680x1050,1600x900,1440x900,"
			"1366x768,1280x1024,1280x800,1280x720,1111x777,1000x750,1000x555,777x555,1357x1018,601x451,666x500,500x375,3440x1440,"
			"2880x1800,1024x600,800x480,333x250,1601x1201,2000x1500,1500x1000,2560x1600,1036x777,960x540");
		const int aLevelMisses = LinearStretch::CheckLevels();
		int aCases = 0, aDiffering = 0;
		uint32_t aSeed = 2024;
		const Uint64 aStart = SDL_GetPerformanceCounter();
		for (const Size& aWin : aTestWindows)
		{
			for (const Size& aSrc : aTestSources)
			{
				DisplayGeometry aGeometry;
				aGeometry.mWidth = aSrc.mWidth;
				aGeometry.mHeight = aSrc.mHeight;
				const int aScale = aGeometry.UpdateDisplaySize(aWin.mWidth, aWin.mHeight, 16384, 16384, 0);
				const int w = aGeometry.mPresentationRect.mWidth, h = aGeometry.mPresentationRect.mHeight;
				LinearStretch aStretch;
				aStretch.Setup(aSrc.mWidth, aSrc.mHeight, aScale, w, h);
				const int aRows = std::max(1, 1000 * 1024 / (4 * w));
				std::vector<uint32_t> aWant((size_t) w * h), aGot((size_t) w * h);
				for (int aPattern = 0; aPattern < kPatternCount; aPattern++)
				{
					std::vector<uint32_t> aPixels = MakePattern(aPattern, aSrc.mWidth, aSrc.mHeight);
					// a top byte in the frame must not reach the window
					for (size_t p = 0; p < aPixels.size(); p += 7)
						aPixels[p] |= 0xFF000000;
					aStretch.DrawReference(&aPixels[0], aSrc.mWidth * 4, 0, 0, w, h, &aWant[0], w);
					std::fill(aGot.begin(), aGot.end(), 0xDEADBEEF);
					for (int y = 0; y < h; y += aRows)
						aStretch.Draw(&aPixels[0], aSrc.mWidth * 4, 0, y, w, std::min(h, y + aRows), &aGot[(size_t) y * w], w);
					aCases++;
					if (aWant != aGot)
					{
						aDiffering++;
						printf("selftest: %s %dx%d -> %dx%d (rect %dx%d, render scale %d), whole: differs\n", kPatternNames[aPattern],
							aSrc.mWidth, aSrc.mHeight, aWin.mWidth, aWin.mHeight, w, h, aScale);
					}
					// parts: a changed rect's columns and rows, drawn into a buffer of their own width
					for (int aPart = 0; aPart < 6; aPart++)
					{
						aSeed = aSeed * 1103515245 + 12345;
						const int x0 = (int) ((aSeed >> 8) % (uint32_t) w);
						aSeed = aSeed * 1103515245 + 12345;
						const int x1 = x0 + 1 + (int) ((aSeed >> 8) % (uint32_t) (w - x0));
						aSeed = aSeed * 1103515245 + 12345;
						const int y0 = (int) ((aSeed >> 8) % (uint32_t) h);
						aSeed = aSeed * 1103515245 + 12345;
						const int y1 = y0 + 1 + (int) ((aSeed >> 8) % (uint32_t) std::min(h - y0, 300));
						const int pw = x1 - x0;
						std::vector<uint32_t> aPart1((size_t) pw * (y1 - y0), 0xDEADBEEF), aPart2((size_t) pw * (y1 - y0), 0x12345678);
						aStretch.Draw(&aPixels[0], aSrc.mWidth * 4, x0, y0, x1, y1, &aPart1[0], pw);
						aStretch.DrawReference(&aPixels[0], aSrc.mWidth * 4, x0, y0, x1, y1, &aPart2[0], pw);
						aCases++;
						if (aPart1 != aPart2)
						{
							aDiffering++;
							printf("selftest: %s %dx%d -> %dx%d, part %d,%d to %d,%d: differs\n", kPatternNames[aPattern], aSrc.mWidth,
								aSrc.mHeight, aWin.mWidth, aWin.mHeight, x0, y0, x1, y1);
						}
					}
				}
			}
		}
		printf("stretchcal selftest: %d cases, %d differing; levels: %d of %d sums differing (%.1f s)\n", aCases, aDiffering, aLevelMisses,
			255 * 256 + 1, (double) (SDL_GetPerformanceCounter() - aStart) / (double) SDL_GetPerformanceFrequency());
		return ((aDiffering == 0) && (aLevelMisses == 0)) ? 0 : 1;
	}

	// -bench: what drawing the whole presentation rect costs the processor, band by band into a bitmap of under 1 MB as
	// GdiStretch::PresentLinear draws it (the copy into the window not included), for each frame and window size; no window
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-bench") != 0)
			continue;
		for (const Size& aWin : aWindows)
		{
			for (const Size& aSrc : aSources)
			{
				DisplayGeometry aGeometry;
				aGeometry.mWidth = aSrc.mWidth;
				aGeometry.mHeight = aSrc.mHeight;
				const int aScale = aGeometry.UpdateDisplaySize(aWin.mWidth, aWin.mHeight, 16384, 16384, 0);
				const Rect aDest = aGeometry.mPresentationRect;
				LinearStretch aStretch;
				aStretch.Setup(aSrc.mWidth, aSrc.mHeight, aScale, aDest.mWidth, aDest.mHeight);
				const int aRows = std::max(1, 1000 * 1024 / (4 * aDest.mWidth));
				std::vector<uint32_t> aBand((size_t) aDest.mWidth * aRows);
				for (int aPattern = 0; aPattern < kPatternCount; aPattern++)
				{
					std::vector<uint32_t> aPixels = MakePattern(aPattern, aSrc.mWidth, aSrc.mHeight);
					const int kFrames = 20;
					const Uint64 aStart = SDL_GetPerformanceCounter();
					for (int f = 0; f < kFrames; f++)
						for (int y = 0; y < aDest.mHeight; y += aRows)
							aStretch.Draw(&aPixels[0], aSrc.mWidth * 4, 0, y, aDest.mWidth, std::min(aDest.mHeight, y + aRows), &aBand[0],
								aDest.mWidth);
					const double aMs = (double) (SDL_GetPerformanceCounter() - aStart) * 1000.0 / (double) SDL_GetPerformanceFrequency() /
						kFrames;
					printf("bench %s %dx%d -> %dx%d (rect %dx%d, render scale %d): %.2f ms a whole frame\n", kPatternNames[aPattern],
						aSrc.mWidth, aSrc.mHeight, aWin.mWidth, aWin.mHeight, aDest.mWidth, aDest.mHeight, aScale, aMs);
				}
			}
		}
		return 0;
	}

	// the renderer as DDInterface::Init asks for it
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d11");
	SDL_SetHint(SDL_HINT_RENDER_BATCHING, "1");
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		printf("stretchcal: SDL_Init failed: %s\n", SDL_GetError());
		return 1;
	}

	int aFailures = 0;
	for (const Size& aWin : aWindows)
	{
		SDL_Window* aWindow = SDL_CreateWindow("stretchcal", 0, 0, aWin.mWidth, aWin.mHeight, SDL_WINDOW_BORDERLESS);
		if (aWindow == NULL)
		{
			printf("stretchcal: no window of %dx%d: %s\n", aWin.mWidth, aWin.mHeight, SDL_GetError());
			aFailures++;
			continue;
		}
		SDL_Renderer* aRenderer = SDL_CreateRenderer(aWindow, -1, SDL_RENDERER_ACCELERATED);
		if (aRenderer == NULL)
		{
			printf("stretchcal: no renderer: %s\n", SDL_GetError());
			SDL_DestroyWindow(aWindow);
			aFailures++;
			continue;
		}
		SDL_RendererInfo anInfo;
		SDL_GetRendererInfo(aRenderer, &anInfo);
		int aDrawWidth = 0, aDrawHeight = 0;
		SDL_GetRendererOutputSize(aRenderer, &aDrawWidth, &aDrawHeight);

		for (const Size& aSrc : aSources)
		{
			DisplayGeometry aGeometry;
			aGeometry.mWidth = aSrc.mWidth;
			aGeometry.mHeight = aSrc.mHeight;
			const int aScale = aGeometry.UpdateDisplaySize(aDrawWidth, aDrawHeight, 16384, 16384, 0);
			const Rect aDest = aGeometry.mPresentationRect;

			SDL_Texture* aFrame = SDL_CreateTexture(aRenderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STATIC, aSrc.mWidth, aSrc.mHeight);
			SDL_Texture* aScreen = SDL_CreateTexture(aRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, aSrc.mWidth * aScale,
				aSrc.mHeight * aScale);
			SDL_SetTextureBlendMode(aFrame, SDL_BLENDMODE_NONE);
			SDL_SetTextureScaleMode(aFrame, SDL_ScaleModeNearest);
			SDL_SetTextureBlendMode(aScreen, SDL_BLENDMODE_NONE);
			SDL_SetTextureScaleMode(aScreen, SDL_ScaleModeLinear);

			LinearStretch aStretch;
			aStretch.Setup(aSrc.mWidth, aSrc.mHeight, aScale, aDest.mWidth, aDest.mHeight);

			for (int aPattern = 0; aPattern < kPatternCount; aPattern++)
			{
				std::vector<uint32_t> aPixels = MakePattern(aPattern, aSrc.mWidth, aSrc.mHeight);
				SDL_UpdateTexture(aFrame, NULL, &aPixels[0], aSrc.mWidth * 4);
				SDL_SetRenderTarget(aRenderer, aScreen);
				SDL_RenderCopy(aRenderer, aFrame, NULL, NULL);
				SDL_SetRenderTarget(aRenderer, NULL);
				SDL_SetRenderDrawColor(aRenderer, 0, 0, 0, 255);
				SDL_RenderClear(aRenderer);
				SDL_Rect aRect = { aDest.mX, aDest.mY, aDest.mWidth, aDest.mHeight };
				SDL_RenderCopy(aRenderer, aScreen, NULL, &aRect);
				std::vector<uint32_t> aShown((size_t) aDrawWidth * aDrawHeight);
				if (SDL_RenderReadPixels(aRenderer, NULL, SDL_PIXELFORMAT_ARGB8888, &aShown[0], aDrawWidth * 4) != 0)
				{
					printf("stretchcal: no read-back: %s\n", SDL_GetError());
					aFailures++;
					continue;
				}
				SDL_RenderPresent(aRenderer);
				for (size_t i = 0; i < aShown.size(); i++)
					aShown[i] &= 0xFFFFFF;
				// the bars are black
				long long aBarPixels = 0;
				for (int y = 0; y < aDrawHeight; y++)
					for (int x = 0; x < aDrawWidth; x++)
						if (((x < aDest.mX) || (x >= aDest.mX + aDest.mWidth) || (y < aDest.mY) || (y >= aDest.mY + aDest.mHeight)) &&
							(aShown[(size_t) y * aDrawWidth + x] != 0))
							aBarPixels++;
				const uint32_t* aShownDest = &aShown[(size_t) aDest.mY * aDrawWidth + aDest.mX];
				if ((aDumpRow >= 0) && (aPattern == 2))
				{
					const int aTexels = aSrc.mWidth * aScale;
					const double aRowPos = ((double) aDumpRow + 0.5) * aSrc.mHeight * aScale / aDest.mHeight - 0.5;
					printf("dump row %d: texel row %.6f\n", aDumpRow, aRowPos);
					for (int x = 0; x < std::min(aDest.mWidth, 400); x++)
					{
						const double aPos = ((double) x + 0.5) * aTexels / aDest.mWidth - 0.5;
						const int t0 = (int) std::floor(aPos);
						const int a = std::max(0, t0) / aScale;
						const int b = std::min(aTexels - 1, t0 + 1) / aScale;
						const uint32_t g = aShownDest[(size_t) aDumpRow * aDrawWidth + x];
						const uint32_t ca = aPixels[(size_t) (std::max(0, (int) std::floor(aRowPos + 0.5)) / aScale) * aSrc.mWidth + a];
						const uint32_t cb = aPixels[(size_t) (std::max(0, (int) std::floor(aRowPos + 0.5)) / aScale) * aSrc.mWidth + b];
						if (a != b)
							printf("  x %d pos %.6f frac %.6f (x256 %.3f) a %d b %d: %06x %06x -> card %06x\n", x, aPos, aPos - t0,
								(aPos - t0) * 256, a, b, ca, cb, g);
					}
				}

				printf("%s %dx%d -> %dx%d (rect %dx%d at %d,%d, render scale %d, %s):", kPatternNames[aPattern], aSrc.mWidth, aSrc.mHeight,
					aDrawWidth, aDrawHeight, aDest.mWidth, aDest.mHeight, aDest.mX, aDest.mY, aScale, anInfo.name);
				if (aBarPixels > 0)
					printf(" BARS NOT BLACK (%lld)", aBarPixels);
				for (int m = 0; m < kModelCount; m++)
				{
					std::vector<uint32_t> aModel;
					ModelStretch(kModels[m], aPixels, aSrc.mWidth, aSrc.mHeight, aScale, aDest.mWidth, aDest.mHeight, aModel);
					Diff d = Compare(aModel, aShownDest, aDest.mWidth, aDest.mHeight, aDrawWidth);
					printf(" %s %lld/%d", kModels[m].mName, d.mPixels, d.mLargest);
					// -diag=<model>: that model's misses by the column's and the row's weight (blended or not), and the first few
					if ((m == aDiagModel) && (d.mPixels > 0))
					{
						std::vector<Tap> aCols = MakeTaps(kModels[m], aSrc.mWidth, aScale, aDest.mWidth);
						std::vector<Tap> aRows = MakeTaps(kModels[m], aSrc.mHeight, aScale, aDest.mHeight);
						// -weights=<file>: for each column (row) between two of the frame's pixels, the weights (0 to 256) that agree
						// with every pixel of it where the other axis does not blend, by the card's arithmetic
						FILE* aWeightsFile = aWeightsPath.empty() ? NULL : fopen(aWeightsPath.c_str(), "a");
						for (int aAxis = 0; (aWeightsFile != NULL) && (aAxis < 2); aAxis++)
						{
							const std::vector<Tap>& aTaps = (aAxis == 0) ? aCols : aRows;
							const std::vector<Tap>& aOther = (aAxis == 0) ? aRows : aCols;
							const int aTexels = ((aAxis == 0) ? aSrc.mWidth : aSrc.mHeight) * aScale;
							for (int i = 0; i < (int) aTaps.size(); i++)
							{
								if (aTaps[i].mA == aTaps[i].mB)
									continue;
								std::vector<bool> aCan(257, true);
								int aSeen = 0;
								for (int j = 0; (j < (int) aOther.size()) && (aSeen < 200); j++)
								{
									const int xx = (aAxis == 0) ? i : j;
									const int yy = (aAxis == 0) ? j : i;
									// where the other axis blends, only if it blends equal pixels
									if (aOther[j].mA != aOther[j].mB)
									{
										const uint32_t* aRA = &aPixels[(size_t) aRows[yy].mA * aSrc.mWidth];
										const uint32_t* aRB = &aPixels[(size_t) aRows[yy].mB * aSrc.mWidth];
										const bool aSame = (aAxis == 0) ?
											((aRA[aCols[xx].mA] == aRB[aCols[xx].mA]) && (aRA[aCols[xx].mB] == aRB[aCols[xx].mB])) :
											((aRA[aCols[xx].mA] == aRA[aCols[xx].mB]) && (aRB[aCols[xx].mA] == aRB[aCols[xx].mB]));
										if (!aSame)
											continue;
									}
									const uint32_t pa = aPixels[(size_t) aRows[yy].mA * aSrc.mWidth + aCols[xx].mA];
									const uint32_t pb = (aAxis == 0) ? aPixels[(size_t) aRows[yy].mA * aSrc.mWidth + aCols[xx].mB] :
										aPixels[(size_t) aRows[yy].mB * aSrc.mWidth + aCols[xx].mA];
									if (((pa ^ pb) & 0xFFFFFF) == 0)
										continue;
									aSeen++;
									const uint32_t g = aShownDest[(size_t) yy * aDrawWidth + xx];
									for (int q = 0; q <= 256; q++)
									{
										for (int aShift = 0; aCan[q] && (aShift <= 16); aShift += 8)
										{
											const int n = (int) ((pa >> aShift) & 255) * (256 - q) + (int) ((pb >> aShift) & 255) * q;
											const int aLevel = ((((n * 257 + 128) >> 12) * 255) + 2047) >> 12;
											if (aLevel != (int) ((g >> aShift) & 255))
												aCan[q] = false;
										}
									}
								}
								const double aPos = ((double) i + 0.5) * aTexels / (double) aTaps.size() - 0.5;
								fprintf(aWeightsFile, "%c:%d:%d %d %d %d %d %d %.6f %d %d", (aAxis == 0) ? 'C' : 'R',
									(aAxis == 0) ? aDest.mHeight : aDest.mWidth, (aAxis == 0) ? aDest.mY : aDest.mX, (int) aTaps.size(), aTexels,
									(aAxis == 0) ? aDest.mX : aDest.mY, (aAxis == 0) ? aDrawWidth : aDrawHeight, i, (aPos - std::floor(aPos)) * 256,
									(int) std::lround(aTaps[i].mWeight * 256), aSeen);
								for (int q = 0; q <= 256; q++)
									if (aCan[q])
										fprintf(aWeightsFile, " %d", q);
								fprintf(aWeightsFile, "\n");
							}
						}
						if (aWeightsFile != NULL)
							fclose(aWeightsFile);
						long long aKinds[4] = { 0, 0, 0, 0 };  // neither blends, columns only, rows only, both
						long long aKindAll[4] = { 0, 0, 0, 0 };
						long long aClearMisses[4] = { 0, 0, 0, 0 };  // misses where no blending tap is within 0.3 steps of a rounding boundary
						long long aClearAll[4] = { 0, 0, 0, 0 };
						int aShownMisses = 0;
						for (int y = 0; y < aDest.mHeight; y++)
						{
							for (int x = 0; x < aDest.mWidth; x++)
							{
								const bool aColBlend = (aCols[x].mA != aCols[x].mB) && (aCols[x].mWeight > 0);
								const bool aRowBlend = (aRows[y].mA != aRows[y].mB) && (aRows[y].mWeight > 0);
								const int aKind = (aColBlend ? 1 : 0) + (aRowBlend ? 2 : 0);
								aKindAll[aKind]++;
								const double aColNear = std::fabs(aCols[x].mRaw - std::floor(aCols[x].mRaw) - 0.5);
								const double aRowNear = std::fabs(aRows[y].mRaw - std::floor(aRows[y].mRaw) - 0.5);
								const bool aClear = ((!aColBlend) || (aColNear > 0.3)) && ((!aRowBlend) || (aRowNear > 0.3));
								if (aClear)
									aClearAll[aKind]++;
								const uint32_t a = aModel[(size_t) y * aDest.mWidth + x];
								const uint32_t b = aShownDest[(size_t) y * aDrawWidth + x];
								if (aClear && (aKind != 0) && !aPairsPath.empty())
								{
									const uint32_t* q0 = &aPixels[(size_t) aRows[y].mA * aSrc.mWidth];
									const uint32_t* q1 = &aPixels[(size_t) aRows[y].mB * aSrc.mWidth];
									const uint64_t wx = (uint64_t) std::lround(aCols[x].mWeight * 256);
									const uint64_t wy = (uint64_t) std::lround(aRows[y].mWeight * 256);
									for (int aShift = 0; aShift <= 16; aShift += 8)
									{
										const uint64_t p00 = (q0[aCols[x].mA] >> aShift) & 255, p10 = (q0[aCols[x].mB] >> aShift) & 255;
										const uint64_t p01 = (q1[aCols[x].mA] >> aShift) & 255, p11 = (q1[aCols[x].mB] >> aShift) & 255;
										const uint64_t c = (b >> aShift) & 255;
										if (aKind == 1)
											aSinglePairs.insert((p00 << 24) | (p10 << 16) | (wx << 8) | c);
										else if (aKind == 2)
											aSinglePairs.insert((p00 << 24) | (p01 << 16) | (wy << 8) | c);
										else
											aDoublePairs.insert((p00 << 56) | (p10 << 48) | (p01 << 40) | (p11 << 32) | (wx << 24) | (wy << 16) | (c << 8));
									}
								}
								if (a == b)
									continue;
								aKinds[aKind]++;
								if (aClear)
									aClearMisses[aKind]++;
								if ((aShownMisses < 12) && ((aDiagKind < 0) || ((aKind == aDiagKind) && aClear)))
								{
									aShownMisses++;
									const uint32_t* r0 = &aPixels[(size_t) aRows[y].mA * aSrc.mWidth];
									const uint32_t* r1 = &aPixels[(size_t) aRows[y].mB * aSrc.mWidth];
									printf("\n    miss %d,%d col %d-%d w %.4f row %d-%d w %.4f: %06x %06x / %06x %06x model %06x card %06x", x, y,
										aCols[x].mA, aCols[x].mB, aCols[x].mWeight * 256, aRows[y].mA, aRows[y].mB, aRows[y].mWeight * 256,
										r0[aCols[x].mA], r0[aCols[x].mB], r1[aCols[x].mA], r1[aCols[x].mB], a, b);
								}
							}
						}
						printf("\n    misses: none blend %lld/%lld, columns only %lld/%lld, rows only %lld/%lld, both %lld/%lld\n   ",
							aKinds[0], aKindAll[0], aKinds[1], aKindAll[1], aKinds[2], aKindAll[2], aKinds[3], aKindAll[3]);
						printf("    clear of boundaries: columns only %lld/%lld, rows only %lld/%lld, both %lld/%lld\n   ", aClearMisses[1], aClearAll[1],
							aClearMisses[2], aClearAll[2], aClearMisses[3], aClearAll[3]);
					}
				}
				// the display's own
				std::vector<uint32_t> anOwn((size_t) aDest.mWidth * aDest.mHeight);
				aStretch.Draw(&aPixels[0], aSrc.mWidth * 4, 0, 0, aDest.mWidth, aDest.mHeight, &anOwn[0], aDest.mWidth);
				for (size_t i = 0; i < anOwn.size(); i++)
					anOwn[i] &= 0xFFFFFF;
				Diff d = Compare(anOwn, aShownDest, aDest.mWidth, aDest.mHeight, aDrawWidth);
				printf(" | LinearStretch %lld/%d", d.mPixels, d.mLargest);
				if (d.mPixels > 0)
				{
					printf(" first at %d,%d", d.mX, d.mY);
					aFailures++;
				}
				printf("\n");
				fflush(stdout);
				if (!anOut.empty())
				{
					char aName[256];
					snprintf(aName, sizeof(aName), "%s/%s-%dx%d-%dx%d.bmp", anOut.c_str(), kPatternNames[aPattern], aSrc.mWidth, aSrc.mHeight,
						aDrawWidth, aDrawHeight);
					SaveBmp(aName, &aShown[0], aDrawWidth, aDrawHeight, aDrawWidth);
				}
			}
			SDL_DestroyTexture(aScreen);
			SDL_DestroyTexture(aFrame);
		}
		SDL_DestroyRenderer(aRenderer);
		SDL_DestroyWindow(aWindow);
	}
	printf("stretchcal: %d cases differing from LinearStretch\n", aFailures);
	SDL_Quit();
	if (!aPairsPath.empty())
	{
		FILE* f = fopen(aPairsPath.c_str(), "w");
		if (f != NULL)
		{
			for (uint64_t k : aSinglePairs)
				fprintf(f, "1 %d %d %d %d\n", (int) (k >> 24) & 255, (int) (k >> 16) & 255, (int) (k >> 8) & 255, (int) k & 255);
			for (uint64_t k : aDoublePairs)
				fprintf(f, "2 %d %d %d %d %d %d %d\n", (int) (k >> 56) & 255, (int) (k >> 48) & 255, (int) (k >> 40) & 255,
					(int) (k >> 32) & 255, (int) (k >> 24) & 255, (int) (k >> 16) & 255, (int) (k >> 8) & 255);
			fclose(f);
		}
	}
	return aFailures;
}
