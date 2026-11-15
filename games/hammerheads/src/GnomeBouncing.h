#ifndef __HAMMERHEADS_GNOMEBOUNCING_H__
#define __HAMMERHEADS_GNOMEBOUNCING_H__

// The jester (0x435b70-0x436f10): a gnome that hops across the screen from one side to the other, bouncing on a floor at
// y = 570. Like the aviator it is an Object of its own, not one of a cubby's creatures. Every hit throws it up again, doubles
// its sideways speed and spins it, and while it is in the air after a hit it lets coins fall, one every eight steps, up to
// twenty. A jester that crosses the screen without a hit does nothing.

#include <string>

#include "Object.h"

namespace Sexy
{

class Graphics;

// What a saved game keeps of a jester (0x436e40 copies it out, 0x436510 puts it back)
struct BouncingState
{
	std::string		mName;
	bool			mActive;
	double			mFrame;
	double			mSpeedX;
	double			mSpeedY;
	int				mFloorY;
	int				mUnusedInt;
	Rect			mBounds;
	double			mX;
	double			mY;
	double			mRotation;
	double			mRotationSpeed;
	bool			mHit;
	int				mCount;
	bool			mCalled;
	int				mHoldCount;
};

class GnomeBouncing : public SimpleObject
{
public:
	// +0x80 to +0x8f: the rectangle of one cel at the jester's position (SimpleObject's bounds); +0x90 and +0x98: its position
	double			mFrame;						// +0xa0: 0.3 a step; never read (the picture comes from the height)
	double			mSpeedX;					// +0xa8: 350 to the right or left, doubled by each hit
	double			mSpeedY;					// +0xb0: down, 18 more every step; -700 after a hit
	int				mFloorY;					// +0xb8: 570, the y that the jester's feet bounce on
	int				mUnusedInt;					// +0xbc: set to 0 and never used
	double			mRotation;					// +0xc0: the angle of its picture
	double			mRotationSpeed;				// +0xc8: 0.1 to 0.15 either way after a hit, 2 percent less every step
	bool			mHit;						// +0xd0: it has been hit
	int				mCount;						// +0xd4: the coins dropped
	bool			mCalled;					// +0xd8: it has said its "yoohoo" on the way in
	int				mHoldCount;					// +0xdc: a hit takes 1 off; it is only taken off the screen once this is below 1

public:
	GnomeBouncing();							// 0x4365d0
	virtual ~GnomeBouncing();					// 0x436270

	OBJECT_TYPE(GnomeBouncing)

	virtual void	FixedUpdate() override;						// 0x436be0
	virtual void	Update(float theFrac) override;				// 0x4367e0
	virtual void	Draw(Graphics* g) override;					// 0x436310
	virtual bool	MouseOver(int theX, int theY) override;		// 0x443290: the point is in the bounds
	virtual bool	Hit(int theX, int theY, const std::vector<Point>& theMask, int theWidth, int theHeight) override;	// 0x4369f0
	virtual void	Strike(double theX, double theY) override;	// 0x436cd0

	void			SaveState(BouncingState& theState) const;	// 0x436e40
	void			LoadState(const BouncingState& theState);	// 0x436510

private:
	int				GetCel() const;								// 0x436280
	void			DropCoin();									// 0x436ab0
	void			UpdateBounds();
};

// 0x436770: makes a jester and gives it the name
GnomeBouncing* MakeGnomeBouncing(const std::string& theName);

// 0x4364a0: the jester at the middle of a level's introduction (theCel 0 to 9, which one of its cels is shown)
void DrawBouncingIntro(Graphics* g, double theX, double theY, int theCel);

}

#endif
