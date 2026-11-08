// Sexy::Mandala (0x420 bytes, a TwistWidget; 0x72096f-0x724e91, constructor 0x7243b1): the mode select the menu's play
// button opens. Two panels slide in with four doors (ModeButton): Classic, Challenge, Blitz and Zen, each opening when
// its mode is unlocked; the portal of the mode under the mouse turns in the middle with the mode's description. Over a
// game it is the "zen jump" instead (mMode 1 or 2): the board's own mode continues or the game moves to the other one.
#pragma once

#include "ButtonListener.h"
#include "CurvedVal.h"
#include "TwistDialogListener.h"
#include "TwistWidget.h"

#include <vector>

namespace Sexy {

class EffectsManager;
class ModeButton;
class TwistButton;

class Mandala : public TwistWidget, public ButtonListener, public TwistDialogListener {
public:
	int mMode;                   // +0x15c: 0 from the menu; over a game 1 (classic) or 2 (zen), the zen jump
	int mSelected = 0;           // +0x160: the mode state 4 starts
	int mMouseX = 0;             // +0x16c
	int mMouseY = 0;             // +0x170
	float mFade = 1.0f;          // +0x178: the opacity of all it draws over the panels
	float mTextBright = 0;       // +0x17c: the description's brightness; 1 when the mouse enters a button
	float mGlow = 0;             // +0x180: the portal's opacity, up while a button is under the mouse
	int mPortal = -1;            // +0x184: the portal shown, the last button the mouse entered
	bool mShowPortal = true;     // +0x188
	int mOver = -1;              // +0x18c: the button under the mouse, -1 none
	float mIdle = 0;             // +0x190: grows while no button is under the mouse; the hint shows after 0.25
	std::vector<ModeButton*> mButtons;  // +0x194: Classic, Challenge, Blitz, Zen
	float mOffsetY = 0;          // +0x1a4: the zoom's centre below the screen's (only Direct3D zooms)
	float mUnknown1A8 = 0;       // +0x1a8: set by the challenge button, never read
	float mDelay = 0;            // +0x1ac: holds the states 0 and 2 while above 0 (never set above 0)
	bool mPlayVoice = true;      // +0x1b0: "choose your game" is still to be said
	int mSaveMode = -1;          // +0x1b4: the mode whose saved game the continue dialog asks about
	int mState = 0;              // +0x1b8: 0 opening, 1 open, 2 closing (then gone), 3 fading out (then gone), 4 the
	                             // zen jump's fade before its game starts
	SexyString mText;            // +0x1bc: the text of the button under the mouse
	TwistButton* mMainMenuButton;  // +0x1d8
	EffectsManager* mFXManager;  // +0x1dc: the classic portal's sparks
	CurvedVal mOpen;             // +0x1e0: the panels slide apart while it falls; the doors open when it ends
	CurvedVal mConnect;          // +0x250: the connectors, on mOpen's input
	CurvedVal mSeamGlow;         // +0x2c0: the glow of the panels' seam
	CurvedVal mZoom;             // +0x330: the whole's opacity (and with Direct3D its size), 1 at rest
	CurvedVal mDarken;           // +0x3a0: the screen goes dark before the zen jump's new game
	int mEggCount = 0;           // +0x410: the times the mouse went over the buttons from the last to the first
	int mEggNext = 0;            // +0x414: the button that counts next
	// (+0x418: the app's registration when it was made, registration code, left out)
	bool mEgg = false;           // +0x419: the next update starts the app's 0x75fd40

	explicit Mandala(int theMode);  // 0x7243b1
	~Mandala() override;            // 0x721d5c

	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x72197b
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x720951
	void Update() override;                                             // 0x72366c
	void Draw(Graphics* g) override;                                    // 0x720aea
	void DrawAll(ModalFlags* theFlags, Graphics* g) override;           // 0x72096f
	void DrawOverlay(Graphics* g) override;                             // 0x721eb0
	using TwistWidget::DrawOverlay;
	void MouseLeave() override {}                                       // 0x72102f
	void MouseMove(int x, int y) override;                              // 0x721016
	void MouseDown(int x, int y, int theClickCount) override {}         // 0x721030
	using TwistWidget::MouseDown;

	void ButtonPress(int theId) override;          // 0x721033
	void ButtonDepress(int theId) override;        // 0x723dc6
	void ButtonMouseEnter(int theId) override;     // 0x722c74
	void ButtonMouseLeave(int theId) override;     // 0x72104a
	void DialogButtonPress(int theDialogId, int theButtonId) override;    // 0x72105d
	void DialogButtonDepress(int theDialogId, int theButtonId) override;  // 0x7242b4
	void DialogThirdButtonDepress(int theDialogId) override;              // 0x721074

	void Layout();                                 // 0x721721: the buttons' places
	void Close();                                  // 0x722c28: the panels close and it goes
	void ShowContinueDialog(int theMode);          // 0x722ec1: continue the saved game or start a new one
	void StartMode(int theMode, bool theNewGame);  // 0x72313d
};

}  // namespace Sexy
