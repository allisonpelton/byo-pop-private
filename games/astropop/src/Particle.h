#ifndef __ASTROPOP_PARTICLE_H__
#define __ASTROPOP_PARTICLE_H__

// Particle: the game's flying sprite (class of 0x8c bytes: the base part 0x402930 with vtable 0x55d3c0 moves it, the
// derived part 0x4034a0 with vtable 0x55d4a4 animates, fades and draws it; the game never makes a base object). A
// particle starts moving after mDelay updates: its vertical speed grows by gravity, its position follows, its
// frames step on, its colour fades towards a target and it is done when the strip has played or its life has run out.

#include "Graphics.h"
#include "Image.h"
#include "MTRand.h"

namespace Sexy
{

// The game's own generator (the object that DAT_005a4e28 points to): not Rand()'s. The application makes it as its Init is done
// (AstroPopApp::Init), seeding it with the next number of Rand()'s generator, as the original's start-up does (0x429280).
extern MTRand	gGameRand;

inline unsigned int AstroRand()         // Sexy::MTRand::NextNoAssert(DAT_005a4e28)
{
	return (unsigned int) gGameRand.NextNoAssert();
}

class Particle
{
public:
	float		mVX;            // +0x04 horizontal speed per update
	float		mVY;            // +0x08 vertical speed, grows by the gravity once the delay is over
	float		mSpeed;         // +0x0c
	float		mGravity;       // +0x10
	float		mX;             // +0x14
	float		mY;             // +0x18
	float		mEndX;          // +0x1c the range of the shot, from the angle (the particle never meets it exactly)
	float		mEndY;          // +0x20
	const float* mSin;          // +0x24 gSinTable
	const float* mCos;          // +0x28 gCosTable
	int			mField2C;       // +0x2c (set by a slot of the vtable, never read)
	int			mAngle;         // +0x30 degrees
	int			mAge;           // +0x34 updates
	int			mDelay;         // +0x38 updates before it moves
	bool		mDone;          // +0x3c
	bool		mPaused;        // +0x3d
	Image*		mImage;         // +0x40
	int			mNumCels;       // +0x44
	int			mCelDelay;      // +0x48 updates per cel
	int			mCel;           // +0x4c
	int			mCelWidth;      // +0x50
	int			mStep[4];       // +0x54 red, green, blue and alpha change per update
	int			mLife;          // +0x64 updates left, -1: for ever
	bool		mFade;          // +0x68 draw with mColor and fade it
	bool		mAdditive;      // +0x69
	bool		mHidden;        // +0x6a
	int			mColor[4];      // +0x6c red, green, blue, alpha
	int			mTarget[4];     // +0x7c

	Particle(float theSpeed, float theVX, float theVY, int theAngle, float theGravity, float theHeight,
		const float* theSin, const float* theCos);     // 0x4034a0 (and 0x402930)

	void SetImage(Image* theImage, int theCelDelay);    // 0x402810
	void SetColors(const Color& theColor, const Color& theTarget);  // 0x402840
	void SetFadeBlue();                                 // 0x402880
	void SetX(float theX);                              // 0x402a30
	void SetY(float theY);                              // 0x402a50

	void Update();                                      // 0x403580 (the base part 0x4029d0)
	void Draw(Graphics* g);                             // 0x402890
};

}

#endif
