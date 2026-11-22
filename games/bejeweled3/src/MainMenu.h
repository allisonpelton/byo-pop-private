// Sexy::MainMenu (vtable 0x86c35c (bejeweled3), 0x7a0 bytes, made by 0x6a54b0): the first screen. It is the
// loading screen too: until the loading thread ends it shows "Loading...", and the player's name under the picture.
// This is the 2D mode's menu: the flat background, the PLAY, RECORDS and OPTIONS buttons and the texts.
// port: the original's curves (the logos' fade, the buttons' arrival), the particles, the tips, the 3D branch and the
// buttons' crystal balls (CrystalBall, 0x78ab50, with their effects) are not translated yet: the buttons are plain.
#pragma once

#include "CurvedVal.h"
#include "Math3D.h"
#include "Widget.h"

#include <string>

namespace Sexy {

// One of the three menu buttons. port: stands in for the original's CrystalBall (vtable 0x86e2c4?, 0x678 bytes,
// 0x78ab50), whose drawing is not translated: it shows its text, lit while the mouse is over it.
class MainMenuButton : public Widget {
public:
	std::string mText;                  // the button's text, from the properties' string ids 0x11d to 0x11f
	float mHighlight = 0.0f;            // +0x450: 0 to 1, up 0.05 an update while the mouse is over the button, else down
	int mUpdateCount = 0;               // +0x47c: updates since it was made (the crystal ball's frame counts them)
	CurvedVal mBlendCurve;              // +0x190: PLAY's blend from its place to the menu's middle (0 until something sets it)
	CurvedVal mScaleCurve;              // +0x200: a constant 0.17 from the constructor on; the menu's curves set it
	double mSubpixelX = 0.0;            // +0x270: how far the place the menu worked out is from the whole pixel it put the ball on
	double mSubpixelY = 0.0;            // +0x278
	float mDepth = 0.0f;                // +0x280: the depth of the ball's point in the camera (0 to 1)
	float mFade = 1.0f;                 // +0x368: a curve's value that fades the ball (the menu sets it)
	bool mActive = false;               // port: stands in for what the menu's calls to slot 0x7c switch on: the plate is drawn once set

	explicit MainMenuButton(const std::string& theText);

	void Draw(Graphics* g) override;
	void Update() override;                // 0x78c730 (bejeweled3), slot 22
};

class MainMenu : public Widget {
public:
	MainMenuButton* mPlayButton = nullptr;      // +0x6e4 (param_1[0x1b9]): string id 0x11f
	MainMenuButton* mOptionsButton = nullptr;   // +0x6e8 (param_1[0x1ba]): string id 0x11d
	MainMenuButton* mRecordsButton = nullptr;   // +0x6ec (param_1[0x1bb]): string id 0x11e

	// The menu's curves, named by their offsets: the constructor sets them (and the intro in Update starts them)
	PerspectiveCamera mCameraA;                 // +0xdc: turned by mTurnACurve * -0.78 (nothing projects through it in 2D)
	PerspectiveCamera mCameraB;                 // +0x134: turned by mTurnBCurve; the balls are projected through it
	CurvedVal mTurnACurve;                        // +0x190: the turn of camera A (0: constant)
	CurvedVal mTurnBCurve;                        // +0x200: the turn of camera B
	CurvedVal mIntroCurve1;
	CurvedVal mBackgroundFadeCurve;                        // +0x2e8: the background's fade in
	CurvedVal mIntroCurve2;
	CurvedVal mIntroCurve3;
	CurvedVal mIntroCurve4;
	CurvedVal mIntroCurve5;
	CurvedVal mIntroCurve6;
	CurvedVal mSideFadeCurve;                        // +0x588: the fade of the side balls
	CurvedVal mNameCurve;
	CurvedVal mIntroCurve7;
	CurvedVal mDistanceCurve;                        // +0x718: how far the cameras stand back (1 at the start)
	bool mShown = true;                         // +0x70f: nothing covers the menu
	bool mIntroStarted = false;                 // +0x70c: the loading ended and the intro began
	bool mBackgroundAdded = false;              // port: stands for the background widget (+0x6d8) having a parent: its fade started
	bool mMusicSwitched = false;                // +0x70e
	float mLoadProgress = 0.0f;                 // +0x708: the loading progress, smoothed

	MainMenu();                                 // 0x6a54b0 (bejeweled3)
	~MainMenu() override;

	void Draw(Graphics* g) override;            // 0x6a0ca0 (bejeweled3)
	void Update() override;                     // 0x6a3530 (bejeweled3)
};

}  // namespace Sexy
