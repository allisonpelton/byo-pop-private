#include "RedPiranha.h"
#include "WinWSApp.h"
#include "Image.h"
#include "Res.h"

using namespace Sexy;

// Not translated (vtordisp thunks): 0x410150, 0x410160, 0x410170, 0x4101c0 (jump to Piranha's functions with the
// pointer adjusted). Nothing is overridden: every slot is Piranha's.

// 0x40ff50: Piranha (0x40fca0, GameObject 0x4611b0 and Enemy 0x40a3f0 first), then the RedPiranha part.
// The pictures are the ones the export names IMAGE_GUMBO_* (ids 0xba, 0xbb and 0xbc)
RedPiranha::RedPiranha(WinWSApp* theApp, int theType) :
	Piranha(theApp, theType)
{
	mImages[0] = mApp->mImages[IMAGE_GUMBO_SWIM];
	mImages[2] = mApp->mImages[IMAGE_GUMBO_DIE];
	mImages[3] = mApp->mPiranhaImages[2][3];
	mImages[1] = mApp->mImages[IMAGE_GUMBO_SHOCK];
	mTextColor = Color(255, 255, 255);
	mTextOffsetX = 4;
	mTextOffsetY = 5;
	mImage = mImages[0];
}

RedPiranha::~RedPiranha()
{
}
