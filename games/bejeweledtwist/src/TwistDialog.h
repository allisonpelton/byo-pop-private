// Sexy::TwistDialog (0x218 bytes, vtables 0x7f230c and 0x7f22ec; 0x754822-0x7558fe) and Sexy::TwistDialogButton
// (0x160 bytes, vtable 0x7f24c4; 0x75591f-0x755b11): the game's dialogs. A TwistDialog pops up from 0.4 of its size
// with a bubble sound, makes its own buttons (up to three, from mTwistButtonMode's bits) and tells its own listener
// (mTwistListener, not the framework's mDialogListener) which one was pressed. Killing it (the app's KillDialog)
// shrinks it away first; it takes itself out of the app's dialogs once it is gone. The app makes every dialog one
// (WinTwistApp::NewDialog) and places it (WinTwistApp::DoDialog).
#pragma once

#include "Dialog.h"
#include "DialogButton.h"

namespace Sexy {

class TwistDialogListener;
class WidgetManager;

class TwistDialogButton : public DialogButton {
public:
	Image* mImage2;               // +0x140: the second image it was made with (never read)
	Widget* mOwned = nullptr;     // +0x144: deleted with the button (never set)
	float mGlow = 0;              // +0x148: follows the mouse (never drawn)
	float mAlpha = 1.0f;          // +0x14c: the box's alpha
	int mLabelOffsetX;            // +0x150: 0 * the art scale
	int mLabelOffsetY;            // +0x154: 1 * the art scale (never read)
	bool mSilent = true;          // +0x158: no sound when the mouse comes over it

	TwistDialogButton(Image* theComponentImage, Image* theImage2, int theId, ButtonListener* theListener);  // 0x755a4a
	~TwistDialogButton() override;  // 0x75591f

	void Update() override;         // 0x755997
	void Draw(Graphics* g) override;  // 0x755b11
	void MouseEnter() override;     // 0x75595b
};

class TwistDialog : public Dialog {
public:
	enum {
		BUTTON_MODE_YES = 1,      // mTwistButtonMode's bits: the buttons it makes
		BUTTON_MODE_NO = 2,
		BUTTON_MODE_THIRD = 4,
		ID_THIRD = 1011,
	};

	TwistDialogListener* mTwistListener = nullptr;  // +0x150: hears the buttons
	Widget* mOwned = nullptr;     // +0x154: deleted with the dialog (never set)
	bool mEscaped = false;        // +0x158: closed with Escape
	int mFixedWidth = 0;          // +0x15c: read only by an unused function (0x754f1f)
	int mArrowX = -500;           // +0x160: the tutorial arrow's point on the screen, drawn while mArrowX > 0
	int mArrowY = -500;           // +0x164
	float mArrowRot = 0;          // +0x168: the arrow's angle
	std::string mName;            // +0x16c: (never read)
	float mTime = 0;              // +0x188: the arrow's bobbing
	bool mIsActive = true;        // +0x18c: true, never read
	int mManagerLayer = 0;        // +0x190: the widget manager +0x88 when added (never read)
	bool mMouseDownOn = true;     // +0x194: mouse presses reach the dialog
	bool mShowClickText = false;  // +0x195: "click to continue" at the bottom
	bool mMouseUpThisUpdate = false;  // +0x196
	DialogButton* mThirdButton = nullptr;  // +0x198
	int mTwistButtonMode;         // +0x19c: BUTTON_MODE_ bits; the framework's mButtonMode stays 0
	int mButtonBottom;            // +0x1a0: 58 * the art scale between the buttons and the bottom
	SexyString mYesLabel;         // +0x1a4
	SexyString mNoLabel;          // +0x1c0
	SexyString mThirdLabel;       // +0x1dc
	int mButtonCount = 0;         // +0x1f8
	int mButtonWidth;             // +0x1fc: at least the button image's cel width
	int mButtonSpacing;           // +0x200: 32 * the art scale
	Image* mButtonImage;          // +0x208
	float mPopSpeed = 0;          // +0x20c
	float mPopScale = 1.0f;       // +0x210: the size it pops up from and shrinks away to (only Direct3D draws it)
	bool mClosing = false;        // +0x214

	TwistDialog(Image* theComponentImage, Image* theButtonComponentImage, int theId, bool isModal,
		const SexyString& theDialogHeader, const SexyString& theDialogLines, const SexyString& theDialogFooter,
		int theButtonMode);   // 0x75528d
	~TwistDialog() override;  // 0x755133

	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x7558fe
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x754b67
	void Update() override;   // 0x754c14
	void Draw(Graphics* g) override;  // 0x7555c3
	void DrawAll(ModalFlags* theFlags, Graphics* g) override;  // 0x754d02
	void Resize(int theX, int theY, int theWidth, int theHeight) override;  // 0x754b73
	void GotFocus() override;   // 0x75521c
	void LostFocus() override;  // 0x7551ca
	void KeyChar(SexyChar theChar) override;  // 0x754b96
	void KeyDown(KeyCode theKey) override;    // 0x754bc3
	void MouseDown(int x, int y, int theBtnNum, int theClickCount) override;  // 0x754ec8
	void MouseUp(int x, int y, int theBtnNum, int theClickCount) override;    // 0x754ea5
	int GetPreferredHeight(int theWidth) override;  // 0x754b1a
	virtual int GetPreferredWidth();  // 0x754acb (slot 76)

	void ButtonPress(int theId) override;    // 0x754e34
	void ButtonDepress(int theId) override;  // 0x754e54

	void PopUp();   // 0x754ede
	void Close();   // 0x754f0b
	void CreateButtons(int theButtonMode);  // 0x7557d5
	void DeleteButtons();  // 0x754822
	void LayoutButtons();  // 0x7548a7
};

}  // namespace Sexy
