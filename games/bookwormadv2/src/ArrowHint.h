// The tutorial's arrows: Sexy::ArrowHint (0x118 bytes, constructor 0x402b90, vtables 0x7a9aa0 and 0x7a9ac8; code
// 0x4029e0-0x404ff0), Sexy::ArrowHintManager (0x100 bytes, constructor 0x40b2c0, vtable 0x7aa354; code
// 0x40b230-0x40c200) and the manager's Sexy::MouseWidget (0x9c bytes, constructor 0x481f60, vtable 0x7af5fc; code
// 0x481f60-0x482540).
//
// An ArrowHint is a bubble of text with an arrow that points at something of the screen. Its short text ("mouse
// over for details...") grows into its lines when the mouse comes over it; its arrow glows with the pulse all the
// hints share. Clicks on its hot spots, a time running out or a click anywhere tell the scripts (the callback set
// with SetCallback) what happened, and it fades out. A hint can also block the mouse over its bubble and arrow, so
// that the screen below does not hear the click (BlockMouseEvents: the MouseWidget does).
//
// The ArrowHintManager is a LuaWidget over the whole screen (the app's mArrowHintManager, made by
// LoadingThreadCompleted) that keeps the hints, updates them, draws them over everything (DrawOverlay, which the
// board calls itself while its overlays are up), and passes the mouse to them. It is hidden and disabled while it
// has none. The board tells it of clicks and zooms (Click, DontDrawArrowHints).
//
// port: the hint's JeffLib::StateMachine (not ported; four states with the transitions 0 to 1, 1 to 2, 1 to 3 and 2
// to 3, each state with an update, a draw and an enter function) is a state number and a switch here.
#pragma once

#include <string>
#include <vector>

#include "LuaLinkedClass.h"
#include "LuaWidgets.h"
#include "Rect.h"

namespace Sexy {

class ArrowHintManager;
class Font;
class GameApp;
class Graphics;
class Image;
class LuaFont;

class ArrowHint : public LuaLib::LuaLinkedClass {
public:
	// The directions the arrow points (the scripts' DirLeft ... DirDown) and why the callback is called
	// (TimeExpired ... ClickedScreen)
	enum { DIR_LEFT, DIR_RIGHT, DIR_UP, DIR_DOWN };
	enum { CALLBACK_TIME_EXPIRED, CALLBACK_CLICKED_HINT, CALLBACK_CLICKED_HOT_SPOT, CALLBACK_CLICKED_SCREEN };
	// The state machine's states. A hint starts in STATE_SHOWING; STATE_FADE_IN and STATE_PULSE_IN (a fade in and
	// two flashes of the bubble) are never entered by anything.
	enum { STATE_FADE_IN, STATE_PULSE_IN, STATE_SHOWING, STATE_FADE_OUT };

	std::vector<std::string> mLines;     // +0x4c (vector at +0x48): the text, one entry per AddText, with ^colour^ tags
	std::vector<Rect> mHotSpots;         // +0x5c (+0x58): where a click means something (SetHotSpot)
	std::vector<Rect> mOverSpots;        // +0x6c (+0x68): where the mouse keeps the lines up (AddOverSpot)
	std::vector<Rect> mBlockedRects;     // +0x7c (+0x78): the rectangles BlockMouseEvents gave the mouse widget
	std::string mShortText;              // +0x88: shown until the mouse comes over (SetShortText)
	ArrowHintManager* mManager;          // +0xa4
	Rect mRectShort;                     // +0xa8: the bubble with the short text
	Rect mRectLong;                      // +0xb8: the bubble with the lines
	int mExpiration = 0;                 // +0xc8: updates until the callback hears TimeExpired (0: never)
	int mId;                             // +0xcc: the scripts' number for it
	int mDir = -1;                       // +0xd0: DIR_*
	int mX = 0;                          // +0xd4: where the arrow's picture is
	int mY = 0;                          // +0xd8
	int mPulseCount = 0;                 // +0xdc: state 1 turns once it has run up and down twice
	int mAlpha = 255;                    // +0xe0: the bubble's alpha while it comes in (255: not coming in)
	int mAlphaDelta = 0;                 // +0xe4
	int mFadeAlpha = 255;                // +0xe8: the alpha while it goes out
	int mFadeDelta = 0;                  // +0xec
	bool mActive = false;                // +0xf0: set by every state's entry; the manager draws only active hints
	bool mShowLong = false;              // +0xf1: the lines are up (the mouse is over the bubble)
	bool mDrawWhenDialogs = false;       // +0xf2: also drawn while a dialog is up
	bool mBlockMouse = false;            // +0xf3: BlockMouseEvents was called
	std::string mCallback;               // +0xf4: the scripts' function (SetCallback)
	Font* mFont = nullptr;               // +0x110: the lines' font (the LuaFont's)
	LuaFont* mLuaFont = nullptr;         // +0x114
	int mState = STATE_SHOWING;          // the state machine's current state (+0x44 points at its State)

	ArrowHint(ArrowHintManager* theManager, int theId);  // 0x402b90
	~ArrowHint() override;                                // 0x403200

	const char* LuaGetRegisteredName() override { return "ArrowHint"; }  // 0x402a20
	// The original registered the class in the constructor, every time
	static void RegisterLua();

	// ---- the manager's calls ----
	// 0x4035f0: the current state's update, then the time running out; true when the hint is done (the manager
	// deletes it)
	bool Update();
	void Draw(Graphics* g);  // 0x4036c0 (every state's draw)
	// 0x404300: a click at (x, y), theLeft for the left button. In STATE_SHOWING, the first hot spot it is in (the
	// bubble with the lines only while they are up) calls the callback and sends the hint to STATE_FADE_OUT.
	// True when it did.
	bool Click(int x, int y, bool theLeft);
	// 0x404a20: the mouse is at (x, y): the lines come up while it is over the bubble (or an over spot) and go
	// again when it leaves. Returns mShowLong.
	bool UpdateHover(int x, int y);
	// 0x4044a0: (x, y) is over the bubble or its arrow (the manager's GetArrowHint)
	bool HitTest(int x, int y);
	// 0x4049a0: (x, y) is in one of the over spots
	bool InOverSpot(int x, int y);
	// 0x404b90: where the arrow's picture is (mX, mY, and a cel's size)
	Rect GetArrowRect();
	// 0x6acc40: to another state, when the machine allows that (0 to 1, 1 to 2, 1 to 3, 2 to 3); false when not
	bool ChangeState(int theState);

	// ---- the scripts' methods ----
	void AddText(std::string theText);         // 0x403c60
	void SetFont(LuaFont* theFont);            // 0x4033b0
	void SetCallback(std::string theFunction)  { mCallback = theFunction; }  // 0x402a70
	// 0x402a40: for every hint: the glow's brightness runs between theMin and theMax, theDelta each update, from theStart
	void SetPulseValues(int theDelta, int theMin, int theMax, int theStart);
	void SetDir(int theDir);                   // 0x404980: the arrow points that way
	void SetHotSpot(int x, int y, int w, int h);   // 0x404250: (-1,-1,-1,-1): the bubble; (0,0,0,0): the screen
	void AddOverSpot(int x, int y, int w, int h);  // 0x404d50
	void SetExpirationTime(int theUpdates) { mExpiration = theUpdates; }  // 0x402ae0
	void SetShortText(std::string theText);    // 0x403b80
	int GetId() { return mId; }                // 0x402b60
	void ClearText();                          // 0x404ad0: the lines and the short text
	void DrawWhenDialogs(bool on) { mDrawWhenDialogs = on; }  // 0x402af0
	void BlockMouseEvents();                   // 0x404c00
	// 0x404580: theAnchorToTip false: the lines' bubble at (x, y); true: the arrow's tip at (x, y), the bubbles
	// placed beside it
	void SetXY(int x, int y, bool theAnchorToTip);

	// The pulse all the hints share (statics at 0x82590c-0x825920 and 0x8186b4)
	static int sPulse;       // 0x82590c: the glow's brightness now
	static int sPulseDelta;  // 0x825910
	static int sPulseMin;    // 0x825914
	static int sPulseMax;    // 0x825918
	static int sPulseUpdate; // 0x82591c: the app update the pulse last moved at
	static int sFlash;       // 0x825920: a second value that runs between 0 and 255 (nothing reads it)
	static int sFlashDelta;  // 0x8186b4

private:
	void SetState(int theState);  // 0x6acbb0: to a state, whatever the machine allows; calls its entry
	void EnterState(int theState);
	bool UpdateState();           // the current state's update (true: finished)
	void FitToScreen();           // 0x4047d0: both bubbles kept on the 800 by 600 screen
	// 0x408fc0: callback(theReason, id, hint, theLeft, theSpot)
	void CallCallback(int theReason, bool theLeft, int theSpot);
};

// The manager's widget over the whole screen that takes the mouse where hints blocked it: IsPointVisible is true
// only in the rectangles (and the cursor is then the pointer), and a click there goes to the manager's Click.
class MouseWidget : public Widget {
public:
	std::vector<Rect> mRects;    // +0x8c (vector at +0x88): the blocked rectangles
	ArrowHintManager* mManager;  // +0x98

	explicit MouseWidget(ArrowHintManager* theManager);  // 0x481f60 (no update, no draw, 800 by 600 at the manager's call)
	// 0x482080: adds a rectangle unless it is there; true when it added it
	bool AddRect(const Rect& theRect);
	// 0x482100: takes the first rectangle equal to theRect away
	void RemoveRect(const Rect& theRect);
	// 0x4823a0: theRect overlaps one of the rectangles
	bool IntersectsRects(const Rect& theRect) const;

	using Widget::MouseDown;
	void MouseDown(int x, int y, int theClickCount) override;  // 0x482200
	void MouseMove(int x, int y) override;                     // 0x482260
	bool IsPointVisible(int x, int y) override;                // 0x4822b0
};

class ArrowHintManager : public LuaWidget {
public:
	std::vector<ArrowHint*> mHints;    // +0xd0 (vector at +0xcc): the hints, in the order they were added
	std::vector<ArrowHint*> mPending;  // +0xe0 (+0xdc): added while the hints are being walked
	GameApp* mApp;                     // +0xec
	int mLastEventUpdate = 0;          // +0xf0: the widget's update count at the last mouse event it took
	int mDrawPriority = 9998;          // +0xf4: the overlay priority it is drawn at (SetDrawPriority)
	bool mDontDraw = false;            // +0xf8: DontDrawArrowHints (the board sets it while it zooms)
	bool mHintsDisabled = false;       // +0xf9: DisableArrowHints: no updates, clicks or mouse moves
	MouseWidget* mMouseWidget;         // +0xfc

	ArrowHintManager(GameApp* theApp);  // 0x40b2c0
	~ArrowHintManager() override;       // 0x40b4c0

	const char* LuaGetRegisteredName() override { return "ArrowHintManager"; }  // 0x40b200
	// The original registered the class in the constructor, every time
	static void RegisterLua();

	// The board's call (0x40ba40; its +0xf8 is mDontDraw): a click at (x, y) with theClickCount (1 and 2 are the left
	// button), told to every hint
	void Click(int x, int y, int theClickCount);

	// ---- Widget ----
	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x40b630: the mouse widget goes in too
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x40b600
	void Update() override;                // 0x40b880: updates the hints, deletes those that are done
	void Draw(Graphics* g) override;       // 0x40b7d0: asks for DrawOverlay, at mDrawPriority (not over the conversation panel)
	void DrawOverlay(Graphics* g) override;  // 0x40b6f0: draws the hints (also called by the board)
	void MouseMove(int x, int y) override;   // 0x40c070: the hints hear where the mouse is
	using LuaWidget::MouseDown;
	void MouseDown(int x, int y, int theClickCount) override;  // 0x40c030
	bool IsPointVisible(int x, int y) override { return false; }  // 0x40b220

	// ---- the scripts' methods ----
	// 0x40c110: a new hint with the number theId, unless there is one (null then); added to the list at the end
	// of a walk when it is made during one. theUnused is a boolean the scripts pass and the original ignores.
	ArrowHint* AddArrowHint(int theId, bool theUnused);
	void DeleteArrowHint(int theId);           // 0x40bc90 (an error message during a walk)
	int GetArrowHint(int x, int y);            // 0x40bbe0: the number of the first hint (x, y) is over, -32767 for none
	ArrowHint* GetArrowHintById(int theId);    // 0x40b660
	void DeleteAllArrowHints();                // 0x40beb0
	void DontDrawArrowHints(bool on) { mDontDraw = on; }         // 0x40b240
	void DisableArrowHints(bool on) { mHintsDisabled = on; }     // 0x40b250
	void SetDrawPriority(int thePriority) { mDrawPriority = thePriority; }  // 0x40b230

	// True while the manager walks its hints (0x82592c): hints are then added to mPending and not deleted
	static bool sWalking;

private:
	void EndWalk();  // the hints added during the walk join the list; with none left the widget goes to sleep
};

}  // namespace Sexy
