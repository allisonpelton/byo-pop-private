// Sexy::ConversationPanel (0x218 bytes, constructor 0x42f740, vtable 0x7abc5c; code 0x42f370-0x433900): the
// story's conversations. The scripts build one through the convpanel.* functions (registered by 0x4c01b0): a
// panel a speaker, each with a portrait (a JeffLib::Animator) that slides in from its side, a font, colours and
// pages of text, then Activate it. A beam of light opens into the panel, the pages type themselves out letter by
// letter (a click shows the page at once, the next click turns it), the speakers' portraits slide past each
// other between panels, and after the last page the panel closes into the beam again. The scripts' callbacks
// hear of each step. The app keeps one panel (mConversationPanel), above everything but the curtain.
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "Color.h"
#include "Rect.h"
#include "Widget.h"

namespace JeffLib {
class Animator;
}

namespace Sexy {

class Font;
class GameApp;
class Graphics;
class Image;

class ConversationPanel : public Widget {
public:
	enum State {
		STATE_OFF,          // hidden
		STATE_BEAM_IN,      // the screen darkens, the beam grows from the sides
		STATE_OPENING,      // the panel opens up and down from the beam
		STATE_TEXT,         // the pages
		STATE_CLOSING,
		STATE_BEAM_OUT,
		STATE_NEXT_PANEL,   // the portraits slide past each other, the inner colour fades to the next panel's
	};

	// a page of text (0x28 bytes, constructor 0x42f4f0)
	struct TextPage {
		std::string mText;     // +0x00: word wrapped
		int mLetters = 1;      // +0x1c: shown so far
		int mY = 0;            // +0x20
		int mMaxWidth = 99999; // +0x24: the widest line (the text is centred in the text rect when narrower)
	};

	// a speaker's part (100 bytes, constructor 0x42f580, freed by 0x42f610)
	struct Panel {
		Font* mFont = nullptr;                  // +0x00
		JeffLib::Animator* mAnim = nullptr;     // +0x04: the portrait
		std::vector<TextPage*> mPages;          // +0x08
		Color mTextColor;                       // +0x18 (white)
		Color mInnerColor;                      // +0x28
		Color mContinueColor;                   // +0x38: of the "click to continue" line
		Rect mTextRect;                         // +0x48
		int mTargetX = -1;                      // +0x58: where the portrait slides to
		unsigned int mPageIndex = 0;            // +0x5c
		bool mTyping = true;                    // +0x60: the page is still typing itself out
		bool mArrived = true;                   // +0x61: the portrait is where it slides to
		bool mTriggersNewUserDialog = false;    // +0x62: its last page asks a new player's name

		Panel();
		~Panel();
		TextPage* CurrentPage() const;  // the page index's, or the last
	};

	GameApp* mApp;
	std::vector<Panel*> mPanels;            // +0x88
	Color mScreenColor;                     // +0x98: darkens the screen behind
	Color mBeamColor;                       // +0xa8
	Color mOuterColor;                      // +0xb8: the panel's outer band
	Color mMiddleColor;                     // +0xc8
	Color mCurrentInner;                    // +0xd8: the inner colour while it fades to the next panel's
	Widget* mRestoreFocus = nullptr;        // +0xe8: the focus before Activate
	std::string mUpdateCallback;            // +0xec: each update, no arguments; a true result redraws
	std::string mDrawCallback;              // +0x108: with the Graphics, before or after the panel draws
	std::string mLetterCallback;            // +0x124: each letter, with the letter
	std::string mFinishedCallback;          // +0x140: a page typed itself out
	std::string mStartedCallback;           // +0x15c: a page or panel started (kept when the panel goes off)
	std::string mDisappearingCallback;      // +0x178: the last page was clicked away
	std::string mInactiveCallback;          // +0x194: the panel is off
	std::function<void()> mOnInactive;      // +0x1b0: the app's own, then forgotten
	std::string mGenericCompletionCallback; // +0x1b8: after mInactiveCallback
	unsigned int mCurPanel = 0;             // +0x1d4
	int mState = STATE_OFF;                 // +0x1d8
	int mAlpha = 0;                         // +0x1dc: the darkening's, then the beam's while it fades
	int mBeamX = 0;                         // +0x1e0
	int mBeamY = 0;                         // +0x1e4
	int mBeamTarget = 0;                    // +0x1e8
	int mPanelX = 0;                        // +0x1ec
	int mPanelY = 0;                        // +0x1f0
	int mPanelW = 0;                        // +0x1f4
	int mPanelH = 0;                        // +0x1f8
	int mOpenHalf = 0;                      // +0x1fc: half the open height
	int mBeamWidth = 0;                     // +0x200
	int mLetterDelay = 0;                   // +0x204: updates a letter, 0 for whole pages
	int mUpdateCnt = 0;                     // +0x208
	int mLastClick = 0;                     // +0x20c
	int mOldPriority;                       // +0x210
	bool mDrawCallbackUnder = false;        // +0x214
	bool mPausesBoard = true;               // +0x215
	bool mSkipLetterCallback = false;       // +0x216: once, for a page replaced while typing

	explicit ConversationPanel(GameApp* theApp);
	~ConversationPanel() override;  // 0x42f9c0

	void SetDrawPriority(int thePriority);  // vf 0x128 (0x42fc10): the old one comes back when it goes off
	// vf 0x118 (0x431760): a third of the screen high in its middle, a half-dark screen, the portrait on theSide
	// (-1 left), five updates a letter, white text in theFont
	void SetDefaults(int theSide, const Color& theInner, Image* thePortrait, Font* theFont, int theAnimDelay);
	void SlidePortrait(int theSide);       // vf 0x11c (0x431950): the last panel's portrait slides in from theSide
	void DontSlidePortrait(int theSide);   // vf 0x120 (0x431bc0): it stands there already
	void SetTextRect(int theX, int theY, int theW, int theH);  // vf 0x124 (0x42fc30): the last panel's
	void SetTextRectBeside(int theSide);   // vf 0x12c (0x431fe0): beside the portrait on theSide
	void SetScreenColor(const Color& theColor) { mScreenColor = theColor; }  // vf 0x130 (0x42f370)
	void SetBeamColor(const Color& theColor) { mBeamColor = theColor; }      // vf 0x134
	void SetOuterColor(const Color& theColor) { mOuterColor = theColor; }    // vf 0x138
	void SetMiddleColor(const Color& theColor) { mMiddleColor = theColor; }  // vf 0x13c
	void SetInnerColor(const Color& theColor);            // vf 0x140 (0x431e00): the last panel's
	void SetContinueTextColor(const Color& theColor);     // vf 0x144 (0x432c40): the last panel's
	void SetAllContinueTextColors(const Color& theColor); // vf 0x148 (0x432cb0)
	void SetDimensions(int theX, int theY, int theW, int theH);  // vf 0x14c (0x4300e0)
	void SetAnim(Image* theImage, int theDelay);          // vf 0x150 (0x42fca0): the last panel's portrait
	void SetAnimXY(int theX, int theY);                   // vf 0x154 (0x42fec0)
	// vf 0x158 (0x42ff20): the portrait in the middle of the panel's height and, when theAlignX, 20 pixels in
	// from the panel's side theSide
	void AlignAnim(int theSide, bool theAlignX);
	void AddTextPage(const std::string& theText);         // vf 0x15c (0x430120): to the last panel, word wrapped
	void SetLetterDelay(int theDelay);                    // vf 0x160 (0x431f80)
	void SetFont(Font* theFont, const Color& theColor);   // vf 0x164 (0x431d50): the last panel's
	void AddNewPanel();                                   // vf 0x168 (0x431e70): like the last one, no pages
	void OffsetPanelY(int theOffset);                     // vf 0x16c (0x4334d0)
	bool Active() const { return mState != STATE_OFF; }   // vf 0x170 (0x42f430)
	// vf 0x174 (0x4316c0): shows it, takes the focus, starts the beam; thePauseBoard pauses a battle's board
	void Activate(bool thePauseBoard);
	void TriggersNewUserDialog();         // 0x4321b0: the last panel's last page asks a new player's name
	void Deactivate();                    // 0x4320b0: off at once, without the callbacks
	void ReplaceCurrentText(const std::string& theText);  // 0x432d20 (convpanel.DebugReplaceCurrentText)

	void SetUpdateCallback(const std::string& theFunc) { mUpdateCallback = theFunc; }   // 0x4bfea0
	void SetDrawCallback(const std::string& theFunc, bool theUnder)                     // 0x4c0060
	{
		mDrawCallback = theFunc;
		mDrawCallbackUnder = theUnder;
	}

	// Widget
	using Widget::MouseDown;
	void Update() override;                                       // 0x430890
	void Draw(Graphics* g) override;                              // 0x430e30
	void KeyDown(KeyCode theKey) override;                        // 0x432090: keys do nothing
	void MouseMove(int x, int y) override;                        // 0x430870
	void MouseDown(int x, int y, int theClickCount) override;     // 0x430730

private:
	Panel* LastPanel() const { return mPanels.back(); }
	Panel* CurPanel() const { return mPanels[mCurPanel]; }
	bool SlideToTarget(Panel* thePanel);    // 0x430da0: 10 pixels; true when there
	void NextPanel();                       // 0x432210: the next panel, or the closing
	void RewrapPage();                      // 0x432530: the current page again, with the player's name
	bool NextLetter();                      // 0x4331a0: true when the page is complete
	void LayOutPage(Panel* thePanel, TextPage* thePage, const std::string& theText);
	void CallDrawCallback(Graphics* g);
	void ForgetCallbacks();
	static std::string WordWrap(Font* theFont, std::string theText, int theWidth, bool theJoinLines);  // 0x432920
};

// convpanel.* (0x4c01b0): the scripts' side, on the app's panel
void RegisterConvPanelFunctions();

}  // namespace Sexy
