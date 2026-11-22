// Sexy::MainMenu (vtable 0x86c35c (bejeweled3), 0x7a0 bytes, made by 0x6a54b0): the first screen. It is the
// loading screen too: until the loading thread ends it shows "Loading...", and the player's name under the picture.
// This is the 2D mode's menu: the flat background, the PLAY, RECORDS and OPTIONS buttons and the texts.
// port: the logos, the particles, the tips and the 3D branch are not translated yet. The buttons are CrystalBalls
// (CrystalBall.h).
#pragma once

#include "CrystalBall.h"
#include "CurvedVal.h"
#include "Math3D.h"
#include "Widget.h"

#include <string>
#include <vector>

namespace Sexy {

class DDImage;

// One partner logo shown before the menu (16 bytes in the exe's list at +0xcc)
struct MenuLogo {
	DDImage* mImage;   // the file named by "Logo<n>File", loaded by the app's slot 0x3c (null when the file is missing)
	int mHoldLeft;   // updates still to hold the logo at full strength (counts down from mHold)
	int mHold;       // "Logo<n>HoldTime"
	int mAlpha;      // 0 to 255
};

class MainMenu : public Widget {
public:
	CrystalBall* mPlayButton = nullptr;      // +0x6e4 (param_1[0x1b9]): string id 0x11f
	CrystalBall* mOptionsButton = nullptr;   // +0x6e8 (param_1[0x1ba]): string id 0x11d
	CrystalBall* mRecordsButton = nullptr;   // +0x6ec (param_1[0x1bb]): string id 0x11e

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
	std::vector<MenuLogo> mLogos;               // +0xcc: the logos still to show, one at a time
	float mBlackFade = 0.0f;                    // +0xd8: the black cover's strength: 1 while a logo shows, then down by 0.05

	MainMenu();                                 // 0x6a54b0 (bejeweled3)
	~MainMenu() override;

	void Draw(Graphics* g) override;            // 0x6a0ca0 (bejeweled3)
	void Update() override;                     // 0x6a3530 (bejeweled3)
};

}  // namespace Sexy
