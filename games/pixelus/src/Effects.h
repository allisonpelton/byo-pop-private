#ifndef __PIXELUS_EFFECTS_H__
#define __PIXELUS_EFFECTS_H__

// Effects.cpp: the small frame animations the Nuclide layer puts on top of the screens' pictures (Pixelus.exe
// 0x437440-0x437b40): an image strip of N equal frames shown at a place, the next frame fading in over the old one.
// Three kinds: a loop (hair and sea of the main menu), a ping-pong (back and forth) and a sparkle (waits a random
// time, then plays its frames once). The original updates them with 0.01 per update (the interface's seconds).

#include "Common.h"
#include <vector>

namespace Sexy
{

class Graphics;
class Image;

class FrameEffect
{
public:
	Image*		mImage;			// +0x08 the strip
	int			mFrames;		// +0x0c
	int			mX;				// +0x10
	int			mY;				// +0x14
	int			mCur;			// +0x18 the frame shown
	int			mNext;			// +0x1c the frame that fades in
	double		mFade;			// +0x20 how far the next frame has come in (0..1)
	double		mInterval;		// +0x28 seconds per frame
	double		mFadeTime;		// +0x30 seconds the cross-fade takes, at the end of the interval
	double		mTime;			// +0x38 seconds into the frame

public:
	FrameEffect(Image* theImage, int theFrames, int theX, int theY, double theInterval, double theFadeTime);
	virtual ~FrameEffect() {}

	virtual void	Update(double theDelta);		// 0x4374a0
	virtual void	Draw(Graphics* g);				// 0x437770

protected:
	void			UpdateFade();
	void			AdvanceFrame();
};

// 0x437530: the frames run to the last and back
class PingPongEffect : public FrameEffect
{
public:
	int			mStep;			// +0x40 +1 or -1

public:
	PingPongEffect(Image* theImage, int theFrames, int theX, int theY, double theInterval, double theFadeTime);
	virtual void	Update(double theDelta) override;	// 0x437590
};

// 0x437630: sits on its first frame for a random time, then plays all the frames once
class SparkleEffect : public FrameEffect
{
public:
	double		mMinWait;		// +0x40
	double		mMaxWait;		// +0x48
	double		mWait;			// +0x50 seconds left until the next play
	bool		mPlaying;		// +0x58

public:
	SparkleEffect(Image* theImage, int theFrames, int theX, int theY, double theMinWait, double theMaxWait,
		double theInterval, double theFadeTime);	// 0x437630
	virtual void	Update(double theDelta) override;	// 0x4376d0

private:
	double			RandomWait();
};

// 0x437b40 add, 0x437880 update all, 0x4378c0 draw all
class EffectList
{
public:
	std::vector<FrameEffect*>	mEffects;

public:
	~EffectList();
	void		Add(FrameEffect* theEffect) { mEffects.push_back(theEffect); }
	void		Update(double theDelta);
	void		Draw(Graphics* g);
	void		Clear();
};

}

#endif
