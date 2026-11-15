#ifndef __PIXELUS_GULLFLOCK_H__
#define __PIXELUS_GULLFLOCK_H__

// GullFlock.cpp: the gulls that circle over the island map (Pixelus.exe 0x430ab0 constructor, 0x430cf0 Update, 0x430270
// Draw, 0x4306f0 one gull's step, 0x430620 / 0x42fd00 / 0x42fe20 / 0x42fff0 its neighbours and the three flocking rules,
// 0x430140 the scare of a crowded gull, 0x42fc80 / 0x42fca0 / 0x42fcc0 the pointer). Each gull is a point in a box that
// follows its neighbours; the pointer pushes the gulls away, or pulls them while a button is held, and one gull at a
// time is sent to a place picked at random. The numbers come from the C library's generator (CrtRand), which the
// original seeds from the clock, so no two runs fly alike.

#include <vector>

namespace Sexy
{

class Graphics;
class Image;

struct Vec3
{
	double x, y, z;
	Vec3() : x(0), y(0), z(0) {}
	Vec3(double theX, double theY, double theZ) : x(theX), y(theY), z(theZ) {}
};

// 0x60 bytes in the original (0x42fbb0 constructor)
struct Gull
{
	Vec3	mPos;				// +0x00
	Vec3	mAccel;				// +0x18
	Vec3	mVel;				// +0x38
	int		mFrame;				// +0x50 the wing's picture, 0 to 3 (an animation of the "Gull" strip, 0x437e50)
	double	mWingTimer;			// +0x58 counts down with the distance flown
	Gull() : mFrame(0), mWingTimer(0) {}
};

class GullFlock
{
public:
	enum { FRAMES = 4 };

	std::vector<Gull*>	mGulls;				// +0x04
	Gull*				mScaredGull;		// +0x10 a crowded gull and the time left (mScareTime), or NULL
	double				mScareTime;
	int					mTargetGull;		// +0x14 the gull sent to mTarget; the count it started with
	double				mTargetTime;		// +0x18
	Vec3				mTarget;			// +0x20
	bool				mPointerDown;		// +0x38
	int					mPointerX, mPointerY;	// +0x3c, +0x40
	Image*				mImage;				// IMAGE_BIRD, four frames side by side

public:
	GullFlock(Image* theImage, int theCount);	// 0x430ab0
	~GullFlock();								// 0x4302d0

	void		Update();									// 0x430cf0, once an update (a step of 0.01)
	void		Draw(Graphics* g);							// 0x430270
	void		MouseMove(int x, int y);					// 0x42fcc0
	void		MouseDown(int x, int y);					// 0x42fc80
	void		MouseUp(int x, int y);						// 0x42fca0

private:
	int			StepGull(Gull* theGull, double theStep);	// 0x4306f0: returns the neighbours it saw
	void		FindNeighbours(Gull* theGull, std::vector<Gull*>& theList);	// 0x430620
	bool		StepScare();								// 0x430140: true once the scare is over
};

}

#endif
