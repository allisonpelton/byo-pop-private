// Sexy::Credits (0x288 bytes, a TwistWidget and ButtonListener; 0x6e7f36-0x6e95f1): the credits, the Mandala's hidden
// easter egg (WinTwistApp::ShowCredits). The screen darkens, a tiled box grows from the middle between two bars, and
// the lines of a fixed list (0x85c668) rise through it, fading in from the bottom and out near the top, with a
// tumbling fruit for each "GEM" line and sparks at the box's edges while it grows. Continue, or the list running out,
// fades it all away; the app then deletes it (UpdateFrames).
#pragma once

#include "ButtonListener.h"
#include "Color.h"
#include "CurvedVal.h"
#include "Rect.h"
#include "TwistWidget.h"

#include <vector>

namespace Sexy {

class EffectsManager;
class Font;
class Image;
class TwistButton;

class Credits : public TwistWidget, public ButtonListener {
public:
	// One line on its way up (0x3c bytes; 0x6e84fe makes one)
	struct Line {
		SexyString mText;          // +0x00
		Image* mImage = nullptr;   // +0x1c: a fruit instead of the text
		int mCel = 0;              // +0x20: the fruit's cel
		float mY = 0;              // +0x24
		Font* mFont = nullptr;     // +0x28
		Color mColor;              // +0x2c: its alpha (+0x38) fades in and out by 5 every other update
	};

	int mState = 0;                    // +0x158: 1 darkens, 2 the box grows, 3 the box and bars appear, 4 the lines
	                                   // run, 5 it all fades away
	float mContentAlpha = 0;           // +0x15c: of the box, the bars and the lines
	float mFade = 0;                   // +0x160: how dark the screen is, 0 to 1
	EffectsManager* mFXManager;        // +0x164: the sparks, a child widget
	TwistButton* mButton = nullptr;    // +0x168: Continue
	bool mPressed = false;             // +0x16c: Continue was pressed (never read)
	float mGrow = 0;                   // +0x170: the bars' width, 0 to 1
	Rect mBox;                         // +0x174: three quarters of the screen, in the middle (set by every update)
	std::vector<Line> mLines;          // +0x184
	float mLineDelay = 0;              // +0x194: the next line comes when it falls to 0
	int mNextLine = 0;                 // +0x198: in the list
	CurvedVal mBob;                    // +0x1a0: the fruits rise and fall by 25 times it
	CurvedVal mTilt;                   // +0x210: the fruits turn by a quarter of pi times it
	int mFruitCel = 0;                 // +0x280: the next fruit's cel, 0 to 6

	Credits();             // 0x6e8c0e
	~Credits() override;   // 0x6e8d00

	void Resize(int theX, int theY, int theWidth, int theHeight) override;  // 0x6e7f36: always the whole screen
	using Widget::Resize;
	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x6e8760
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x6e7f7a
	void Update() override;                                             // 0x6e903c
	void Draw(Graphics* g) override;                                    // 0x6e817f
	void DrawAll(ModalFlags* theFlags, Graphics* g) override;           // 0x6e7fce
	void ButtonDepress(int theId) override;                             // 0x6e8403

	void Close();  // 0x6e8035: on to state 5
};

}  // namespace Sexy
