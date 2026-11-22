// PathTrace: a path of points read from a picture (0x454a30..0x454ab0 and on, 0x22750 bytes in the original). The picture
// is a line of pixels drawn in colours that mean: black nothing, blue (1), green (2), white (4: the start) and the others
// (3); the loader follows the line from the start pixel from neighbour to neighbour, makes each point's angle from its
// neighbours' (turned by a window of 25 points) and keeps the points relative to the start. The base's life icon
// (MetaGame) and the flies and moths (BugBrainFly) fly along them: a key reads the tables through MOD_PATH, the position along
// the path being the PATH channel.
#pragma once

#include "Common.h"

namespace Sexy
{

const int PATH_MAX_POINTS = 5040;               // 0xec40 bytes of raw pixels, 12 each

class GameApp;

class PathTrace
{
public:
	// The offsets in the comments are the original's, from the object
	struct RawPixel
	{
		int mKind;                              // +0x0: what the pixel's colour means (0 once the point is taken in the path)
		int mX;                                 // +0x4
		int mY;                                 // +0x8
	};

	RawPixel mRaw[PATH_MAX_POINTS];             // +0x0: the line's pixels, in the order the picture holds them, until they are put in order
	int mCount;                                 // +0xec40: points on the path
	float mX[PATH_MAX_POINTS];                  // +0xec44: each point's place across, from the start
	float mY[PATH_MAX_POINTS];                  // +0x13b04: down
	float mAngle[PATH_MAX_POINTS];              // +0x189c4: the angle it faces (in radians, up is a quarter turn), smoothed
	int mPointKind[PATH_MAX_POINTS];            // +0x1d884: the kind of each point's pixel (the name is a guess): the first is the start's
	GameApp* mApp;                              // +0x22744: the app the picture is loaded through

	PathTrace();                                // 0x454a30
	void Load(const char* theName);             // 0x454ab0 (ESI the path, EDX the name)

	// 0x454a40 (EAX the row, ECX the column, EDX the width, the pixels and the height pushed): what a pixel's colour means,
	// 0 for a place outside the picture
	static int ClassifyPixel(int theColumn, int theRow, int theWidth, const ulong* thePixels, int theHeight);
};

} // namespace Sexy
