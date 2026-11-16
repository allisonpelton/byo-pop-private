#ifndef __PIXELUS_BUBBLES_H__
#define __PIXELUS_BUBBLES_H__

// Bubbles.cpp: the board's helper widget (Pixelus.exe 0x40e940 constructor, vftable 0x50d138): the speech bubbles the
// tutorial and the failed clicks put beside a cell. It covers the whole screen and takes no mouse. Each bubble is one of
// four pictures (the tail at a corner: 0 north west, 1 north east, 2 south east, 3 south west) with a short text beside
// it, which fades in 50 updates after it is added (a bubble that a click made waits no time) and out when it is taken away.

#include "Widget.h"
#include <string>
#include <vector>

namespace Sexy
{

class PixelusApp;
class Graphics;

class HintBubbles : public Widget
{
public:
	// the original's 0x40-byte record (+0x58 app, +0x5c the list, +0x6c the number the next one gets)
	struct Bubble
	{
		std::string	mText;			// +0x00
		int			mAlpha;			// +0x1c 0 to 255 (200 beside the right part of the screen)
		bool		mRemoving;		// +0x20
		int			mCorner;		// +0x24
		int			mX;				// +0x28 the cell's centre
		int			mY;				// +0x2c
		int			mId;			// +0x30
		bool		mDirty;			// +0x34 added, not yet counted by an update
		bool		mFlagged;		// +0x35 the mouse moving away takes it back (0x40e5c0)
		int			mDelay;			// +0x38 updates before it shows
		int			mSoundDelay;	// +0x3c updates before its sound
	};

	PixelusApp*			mApp;
	std::vector<Bubble>	mBubbles;
	int					mNextId;

public:
	explicit HintBubbles(PixelusApp* theApp);					// 0x40e940

	int					Add(int theX, int theY, int theCorner, const std::string& theText, bool theFlagged);	// 0x40ece0
	void				Remove(int theId);						// 0x40e550
	void				RemoveAll();							// 0x40e590
	bool				RemoveFlagged();						// 0x40e5c0
	static int			CountLines(Graphics* g, int theWidth, const std::string& theText);	// 0x431c70

	virtual void		Update() override;						// 0x40ede0
	virtual void		Draw(Graphics* g) override;				// 0x40e5f0
};

}

#endif
