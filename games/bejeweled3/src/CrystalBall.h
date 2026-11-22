// Sexy::CrystalBall (vtable 0x865564 (bejeweled3), 0x678 bytes, made by 0x78ab50): the main menu's buttons, each a glass
// ball with a label. The menu (MainMenu::Update, 0x6a3530) puts a ball where its 3D point lands on the screen and sets the
// curves and values it draws from; the ball itself updates its highlight (0x78c730) and draws (0x78ae40).
// The ball is a ButtonWidget (0x49c490): the menu is its listener, and its id is 0 (OPTIONS), 1 (RECORDS) or 2 (PLAY).
// port: the 3D drawing of the ball (the refraction mesh of 10 rings of 30 points over the picture behind it, drawn as
// triangle strips under the exe's Graphics3D, and the 3D forms of the transforms) is not translated: the ball is always
// drawn on the exe's 2D path. The fields only that part reads (+0x488 to +0x5f8 and the curves' 3D set-up) are left out.
#pragma once

#include "ButtonWidget.h"
#include "Font.h"
#include "CurvedVal.h"
#include "PIEffect.h"
#include "SharedImage.h"
#include "Widget.h"

#include <string>

namespace Sexy {

// 0x58f620 and 0x58f920 (bejeweled3): the colour of a font's layer, set by its tag and put back
bool SetLayerColor(Font* theFont, const char* theName, const Color& theColor, Color* theOldColor);
void RestoreLayerColor(Font* theFont, const char* theName, const Color& theOldColor);

class CrystalBall : public ButtonWidget {
public:
	std::string mText;                  // +0xc0: the label, from the properties' string ids 0x11d to 0x11f
	SharedImageRef mCover;              // +0x168: a picture the ball shows over its rectangle while mBlendCurve is not 0
	PIEffect* mBallEffect = nullptr;    // +0x184: a copy of PIEFFECT_CRYSTALBALL, looping, drawn over the ball
	PIEffect* mRaysEffect = nullptr;    // +0x188: a copy of PIEFFECT_CRYSTALRAYS, looping, drawn under it
	CurvedVal mBlendCurve;              // +0x190: 0 at rest, 1 when the ball has grown to cover the screen
	CurvedVal mScaleCurve;              // +0x200: the ball's size (the constant 0.17 from the constructor)
	double mSubpixelX = 0.0;            // +0x270: how far the place the menu worked out is from the whole pixel it chose
	double mSubpixelY = 0.0;            // +0x278: (only the 3D drawing moves the ball by them)
	float mDepth = 0.0f;                // +0x280: the depth of the ball's point in the camera
	CurvedVal mOffsetXCurve;            // +0x288: the ball's move across (3D set-up only: still 0 on the 2D path)
	CurvedVal mOffsetYCurve;            // +0x2f8: the ball's move down (the same)
	float mFade = 1.0f;                 // +0x368: a curve's value that fades the label (the menu sets it)
	CurvedVal mArrowRightCurve;         // +0x370: while above 0 the menu arrow shows on the right, mirrored
	CurvedVal mArrowLeftCurve;          // +0x3e0: while above 0 the menu arrow shows on the left
	float mHighlight = 0.0f;            // +0x450: 0 to 1, up 0.05 an update while the mouse is over the ball, else down
	Color mColor = Color(255, 255, 255, 255);   // +0x458: the colour the ball's pictures are drawn in
	Color mTextColor = Color(128, 0, 64, 160);  // +0x468: the colour of the label font's MAIN layer
	float mTextScaleAdd = 0.0f;         // +0x478: added to the label's scale before the ball's own
	int mUpdateCount = 0;               // +0x47c: updates since it was made (the ball's frame counts them)
	bool mFlat = false;                 // +0x484: drawn flat: no moves, no effect under it, the first frame, a half alpha
	bool mMoves = true;                 // +0x485: the offset curves move the ball
	bool mDrawPicture = false;          // +0x486: the question mark in place of the label (a locked mode; the PlayMenu sets it)
	bool mArrowShown = false;           // +0x487: the menu arrow's hint (set elsewhere; its drawing is not translated)

	CrystalBall(int theId, ButtonListener* theListener, const std::string& theText);   // 0x78ab50 (bejeweled3)
	~CrystalBall() override;

	void Draw(Graphics* g) override;       // 0x78ae40 (bejeweled3), slot 21
	void Update() override;                // 0x78c730 (bejeweled3), slot 22
};

}  // namespace Sexy
