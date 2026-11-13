#ifndef __HEAVYWEAPON_PARTICLES_H__
#define __HEAVYWEAPON_PARTICLES_H__

// Particles.cpp: the sparks and smoke puffs thrown off by hits and explosions, each a sprite's frames played once.
// Original: 0x14 bytes (vtable 0x52ed60, constructor 0x42b8c0, destructor 0x42b8f0, deleting 0x42bc10), kept inside
// the board at +0x2e8.

#include "ObjectList.h"

namespace Sexy
{

class Graphics;
class Sprite;

// One particle (0x50 bytes)
struct Particle
{
	Sprite*			mSprite;		// +0x00
	double			mX;				// +0x08
	double			mY;				// +0x10
	double			mVelX;			// +0x18
	double			mVelY;			// +0x20
	double			mGravity;		// +0x28 added to mVelY while it has not reached mVelYLimit
	double			mVelYLimit;		// +0x30 the fall speed it stops at (a rise speed when mGravity is not above 0)
	bool			mFades;			// +0x38 its alpha falls as its frames run out
	double			mFrame;			// +0x40 it goes when this passes the sprite's last column
	double			mFrameSpeed;	// +0x48
};

class Particles
{
public:
	ObjectList<Particle>	mParticles;	// +0x04

public:
	Particles();				// 0x42b8c0
	virtual ~Particles();		// 0x42b8f0 (deleting 0x42bc10, slot 0)

	// theCount particles at (x, y), each moving at (theVelX, theVelY) plus a random speed below theSpread in a random
	// direction; theFrame >= 0 holds every one on that frame, else they play the sprite's columns in theFrameUpdates
	void	Emit(Sprite* theSprite, double x, double y, double theVelX, double theVelY, int theCount, double theSpread,
				 double theGravity, double theVelYLimit, int theFrameUpdates, bool fades, int theFrame); // 0x42b900
	void	Update();			// 0x42ba60
	void	Draw(Graphics* g);	// 0x42bb50
	void	Clear();			// 0x42bc40
};

}

#endif
