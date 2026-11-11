#ifndef __ASTROPOP_ASTROBUTTON_H__
#define __ASTROPOP_ASTROBUTTON_H__

// AstroButton: the game's image button (original class derived from ButtonWidget, constructor 0x454c10, sizeof 0x1f0,
// vftable 0x565310 with 53 slots; destructor 0x454e00 / body 0x454e30). It plays three frame animations of its
// images, one for each state: idle (mButtonImage), pointer over (mOverImage, or the button image) and pressed
// (mDownImage; without one the over image is drawn one pixel down and right), and may glow (a pulsing additive
// second draw), tint its images, play a sound when the pointer enters and mask its hit area with a bitmap.
// Overrides: 16 Draw 0x43bda0, 17 Update 0x434a20, 23 MouseEnter 0x43bc00, 24 MouseLeave 0x4348f0,
// 27 MouseDown 0x434940, 30 MouseUp 0x4349c0, 33 IsPointVisible 0x4348a0.
// Offsets in the comments are the original's.

#include "ButtonWidget.h"
#include "Color.h"
#include "FrameAnim.h"
#include <vector>

namespace Sexy
{

class SoundInstance;

class AstroButton : public ButtonWidget
{
public:
	int*					mHitMask;           // +0xe0 one int per pixel of the button (-1: the pixel belongs to the button), NULL for none
	int						mHitMaskWidth;      // +0xe4
	bool					mDrawEnabled;       // +0xe8 constructed true; false draws nothing
	bool					mStopOtherSounds;   // +0xe9 constructed true: entering stops the sounds listed in mOtherSounds
	bool					mGlow;              // +0xea constructed false
	std::vector<SoundInstance*> mOtherSounds;   // +0xf0 (begin +0xf0, end +0xf4)
	FrameAnim				mAnimOver;          // +0xfc pointer over: stopped until the pointer enters
	FrameAnim				mAnimDown;          // +0x13c pressed: stopped until a press
	FrameAnim				mAnimIdle;          // +0x17c idle: runs from the constructor on
	Color					mTintNormal;        // +0x1bc used with mUseTintNormal
	Color					mTintOver;          // +0x1cc used with mUseTintOver
	SoundInstance*			mSound;             // +0x1dc an instance the owner keeps, played on MouseEnter
	int						mSoundId;           // +0x1e0 a sample played when mSound is NULL or still playing (-1: none)
	int						mSoundPitch;        // +0x1e4 pitch steps, 500000000 for none
	bool					mUseTintNormal;     // +0x1e8
	bool					mUseTintOver;       // +0x1e9

public:
	AstroButton(int theId, ButtonListener* theListener);        // 0x454c10
	virtual ~AstroButton();                                     // 0x454e00 slot 0

	virtual void	Draw(Graphics* g) override;                 // 0x43bda0 slot 16
	virtual void	Update() override;                          // 0x434a20 slot 17
	virtual void	MouseEnter() override;                      // 0x43bc00 slot 23
	virtual void	MouseLeave() override;                      // 0x4348f0 slot 24
	virtual void	MouseDown(int x, int y, int theClickCount) override;  // 0x434940 slot 27
	virtual void	MouseUp(int x, int y) override;             // 0x4349c0 slot 30
	virtual bool	IsPointVisible(int x, int y) override;      // 0x4348a0 slot 33

	using ButtonWidget::MouseDown;
	using ButtonWidget::MouseUp;
};

}

#endif
