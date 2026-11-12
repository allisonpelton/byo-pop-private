#ifndef __EFFECTOVERLAY_H__
#define __EFFECTOVERLAY_H__

// EffectOverlay.h: the widget over the board that draws what goes over everything else (Sexy::EffectOverlay, 0x5b524c-0x5b9500, its
// last member ends at +0x1ac): the big texts ("LEVEL 3", "EXCELLENT") that stretch in and fade out, the portal of the warp that
// opens between the main menu and the game, and the end screen of a galaxy, a picture that is built out of gems, lines and
// polygons (the files in edata) and then morphs through four shapes before the credits roll. Board::mEffectOverlay (+0x98) holds
// it; it also draws the arrows of the hints and of the puzzle pods. The offsets in the comments are the original's, from the
// start of the object.
//
// The end screen (InitFace, DoUpdate, Draw) goes through mFaceState: 1 the black bars close in, 2 the gems fall into place,
// 3 the lines are drawn between them, 4 the hyper gem is made (a flash), 5 a pause, 6 the shape morphs by the table of
// steps in DoUpdate and fades, 7 (never set in this class) a second fade, 8 a gem flies a curve to the middle of the screen,
// 9 the end. mEndTimer counts the updates since the start and the board's own code uses it to time the texts that follow.

#include "Widget.h"
#include "Color.h"
#include "Point.h"
#include <string>
#include <vector>

namespace Sexy
{

class WinDMApp;
class MemoryImage;
class Graphics;

// a gem that falls into the shape of the end screen (0x14 bytes); the places come from edata\gem_points.txt, the colour and
// the start of the fall from the random numbers
struct FaceGem
{
	int				mType;              // +0x00 the frame of IMAGE_SMALL_GEMS
	float			mX;                 // +0x04
	float			mY;                 // +0x08
	float			mVY;                // +0x0c the speed of the fall (starts at a random value from 0 to -1.25)
	float			mTargetY;           // +0x10 where it lands
};

// a polygon of the end screen (0x20 bytes): the points are indices into the shape (mShape)
struct FacePoly
{
	Color			mColor;             // +0x00
	std::vector<int> mIndices;          // +0x10
};

// a step of the morph of the end screen (the original builds the table of them on the stack in DoUpdate): the shape moves
// from where it was towards a mix of mSets[mFrom] and mSets[mTo], for mDuration updates
struct FaceStep
{
	int				mDuration;          // +0x00 in updates; -1 ends the table
	int				mFrom;              // +0x04 the number of a set
	int				mTo;                // +0x08 the number of the other set
	int				mPercent;           // +0x0c how far the mix goes from mFrom to mTo
};

class EffectOverlay : public Widget
{
public:
	WinDMApp*		mApp;               // +0x88
	int				mTextOn;            // +0x8c the big text is showing (DoStrechyText sets it, DoUpdate clears it)
	MemoryImage*	mTextImage;         // +0x90 the text, drawn once into a picture (leaked by the original)
	int				mTextWidth;         // +0x94 the width of the text in the picture (at most 800)
	float			mTextAlpha;         // +0x98 how opaque the text is: rises to 1, then falls
	float			mTextSizeFactor;    // +0x9c the size of the text relative to its spring (DoStrechyText sets 0.73; Board changes it afterwards)
	float			mTextScale;         // +0xa0 the spring that sizes the text: 1 at the start, settles round 1.2 and 1.75, then falls to 0
	float			mTextScaleVel;      // +0xa4 its speed
	float			mTextStretch;       // +0xa8 how much the text is stretched sideways (1, damped to 0)
	float			mTextStretchVel;    // +0xac its speed
	float			mTextAge;           // +0xb0 0 to 1 over 25 updates: the damping of the stretch
	int				mTextHold;          // +0xb8 updates before the spring starts (30)
	int				mTextFadeIn;        // +0xbc updates that the text fades in (the delay and 90)
	int				mTextSettleA;       // +0xc0 updates until the spring stops aiming at 1.75 (the delay and 60)
	int				mTextSettleB;       // +0xc4 updates until the spring stops aiming at 1.2 (60)
	bool			mClearBacklogAfterPortal; // set by WinDMApp::DoNewGame once the portal is made; Draw clears the update backlog
	bool			mPortalOpen;        // the portal is open: the overlay covers the whole screen (MainMenu::Draw skips its backdrop)
	float			mPortalProgress;    // +0xcc 0 to 1: the size of the portal; the board is shown when it is over
	int				mPortalDelay;       // +0xd0 updates left before the portal opens (20)
	int				mEndTimer;          // +0xd4 the updates since InitFace
	int				mMorphTimer;        // +0xd8 the updates since the morph began (state 6)
	std::vector<Point> mSets[4];        // +0xdc the four shapes (edata\points.txt, points2 to points4): 170 points each
	std::vector<Point> mCurShape;       // +0x11c the shape at the start of the step of the morph
	std::vector<Point> mShape;          // +0x12c the shape that is drawn
	std::vector<Point> mLines;          // +0x13c the lines (edata\lines.txt): two indices into mShape
	std::vector<FacePoly> mPolys;       // +0x14c the polygons (edata\polys.txt)
	std::vector<FaceGem> mGems;         // +0x15c the gems that fall into place
	int				mLastHitSound;      // +0x16c the update when a landing last played its sound
	float			mLineProgress;      // +0x170 0 to 1: how far the lines are drawn
	int				mFaceDelay;         // +0x174 updates left in the state
	float			mFlash;             // +0x178 the white flash over the screen
	int				mFaceState;         // +0x17c 0 is off, then 1 to 9 as above
	float			mWildX;             // +0x180 the middle of the gem that flies in state 8 (and sits in the middle in 6 and 7)
	float			mWildY;             // +0x184
	float			mWildT;             // +0x188 how far along its curve it has flown
	float			mFaceAlpha;         // +0x18c the alpha of the lines and the polygons: falls to 0 in state 6
	float			mFade;              // +0x190 how far the credits and the bars have faded (1 to 0; Board lowers it)
	int				mFadeHold;          // +0x194 updates before the face starts to fade (550)
	int				mBarStep;           // +0x198 the size of the black bars at the top and the bottom (to 56)
	std::vector<std::string> mCredits;  // +0x19c the lines of the credits (edata\credits.txt)

public:
	EffectOverlay(WinDMApp* theApp);    // 0x5b8b7e
	virtual ~EffectOverlay();           // 0x5b8cee

	void			DoStrechyText(const std::string& theText, int theDelay);    // 0x5b524c a big text that stretches in
	void			DoPortal();                                                 // 0x5b53b0
	void			InitFace();                                                 // 0x5b9071 the end screen: reads the files, starts state 1
	virtual void	Update() override;                                          // 0x5b871f
	virtual void	Draw(Graphics* g) override;                                 // 0x5b59b2

	void			DoUpdate();                                                 // 0x5b7cda one update (Update makes two in the hyper mode)

	static FPoint	InterpolatePointF(const Point& theA, const Point& theB, float theT);    // 0x5b592c
	static Point	InterpolatePoint(const FPoint& theA, const FPoint& theB, float theT);   // 0x5b5960
};

}

#endif
