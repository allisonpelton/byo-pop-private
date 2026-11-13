#ifndef __HAMMERHEADS_AVATAR_H__
#define __HAMMERHEADS_AVATAR_H__

// The player's hammer and crosshair (0x42d660-0x42eb10, 0x42e4f0): follows the mouse, takes the clicks the screen queues for
// it, finds what a click hits (a creature on a cubby, a sticker, any other object that answers MouseOver) and swings; the
// hit lands, as a Strike on the target, in the third frame of the swing.

#include <utility>
#include <vector>

#include "Object.h"

namespace Sexy
{

class Graphics;
class Image;

class Avatar : public Object
{
public:
	enum
	{
		SWING_NONE = 0,
		SWING_HIT = 1,				// frames 0 to 6; the Strike falls in frame 3
		SWING_LONG = 2				// ends at frame 12 (nothing in the original sets it)
	};

	double			mMouseX;			// +0xa0: where the pointer is
	double			mMouseY;			// +0xa8
	double			mHammerX;			// +0xb0: where the hammer is drawn and the last hit landed
	double			mHammerY;			// +0xb8
	int				mSwing;				// +0xc0
	bool			mOverTarget;		// +0xc4: the pointer is on something the hammer can hit
	bool			mStruck;			// +0xc5: this swing's Strike has been made
	double			mBoing;				// +0xc8: the hammer bounces off a shield; counts down from 1
	double			mBombShake;			// +0xd0: after a bomb, the hammer shakes for this long
	ObjectRef		mTarget;			// +0xd8: what the swing is to hit
	double			mFrame;				// +0xf8: the animation of the swing
	bool			mCrosshairVisible;	// +0x100
	std::vector<Point> mMask;			// +0x104: the points of a circle of radius 26 that a hit is tested with
	int				mMaskWidth;			// +0x114
	int				mMaskHeight;		// +0x118
	double			mHammerFade;		// +0x120: 1 at a swing, down to 0 in 0.03 steps; the hammer is drawn as it fades
	std::vector<std::pair<double, double> > mClicks;	// +0x128: the click waiting for the hammer (at most one)
	int				mClickCount;		// +0x138: clicks taken since the avatar was made

public:
	Avatar();								// 0x42def0
	OBJECT_TYPE(Avatar)

	virtual void	FixedUpdate() override;	// 0x42eac0
	virtual void	Update(float theFrac) override;	// 0x42d730
	virtual void	Draw(Graphics* g) override;		// 0x42da00

	void			SetMouse(int theX, int theY);	// 0x42d820
	void			ClearHammerFade();				// 0x42d840: the hammer is not drawn until the next swing
	void			Deflect();						// 0x42d850: the hammer bounces
	void			BombHit();						// 0x42d880
	void			AddClick(int theX, int theY);	// 0x42e0d0: kept only when none is waiting and no gift is to be opened
	void			ClearClicks();					// 0x42e110
	SimpleObject*	GetTarget() const;				// 0x42d680

private:
	bool			Probe(int theX, int theY, bool theHover);	// 0x42e120: finds the object under the hammer; marks it hovered, or makes it the swing's target
	bool			UpdateHover();					// 0x42e490
	void			HandleClick();					// 0x42e5f0
};

Avatar* GetAvatar();					// 0x42d7f0: null until the screen has made one
Avatar* MakeAvatar();					// 0x42e4f0

// 0x42d6b0: the hammer's image for its level (0 to 3) and whether it is on fire
Image* GetHammerImage(int theLevel, bool theFire);

}

#endif
