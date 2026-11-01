// Sexy::Tutorial (0x220 bytes, vtables 0x7f1adc, 0x7f1ac8 (TwistDialogListener, at +0x148) and 0x7f1abc (PopAnimListener,
// at +0x14c); code 0x750a5a-0x754107): the tutorial a Board runs for a tip ("flamegem", "basics" and the rest), made by
// Board::ShowNextTip (0x6bbc58) and kept at the board's +0x2584. It is a script interpreter: the lines of
// properties/tutorial.txt (the board's Buffer at +0x254c, a line each) are run one command at a time until one of them
// has to wait (a state: for points, matches, the board to settle, a button and so on); every update checks whether the
// wait is over and runs on. Its commands put up a TwistDialog with a text, an animation and buttons, point an arrow or
// a hole at a gem or part of the tube, set up the board (gems, rows, the score, the level) and unlock things.
//
// Not ported: see the NotPorted calls (the main menu, the tube's fade, the fruit award and the restart from a saved
// board), and the title (+0x1f4..+0x20c: nothing ever makes the image, so mTitleState stays 0).
#pragma once

#include "Buffer.h"
#include "PopAnim.h"
#include "TwistDialogListener.h"
#include "TwistWidget.h"

#include <string>
#include <vector>

namespace Sexy {

class Board;
class Piece;
class TwistDialog;

class Tutorial : public TwistWidget, public TwistDialogListener, public PopAnimListener {
public:
	enum {
		STATE_NONE = 0,
		STATE_POINTS = 1,         // the score reaches mStateArg
		STATE_MATCHES = 2,        // the match count reaches mStateArg
		STATE_COUNTERGEMS = 3,    // the app's stat 71 reaches mStateArg
		STATE_READY = 4,          // the board is still, with no cursor move waiting
		STATE_MOVE = 5,           // a forced move is done (MoveDone)
		STATE_DIALOG = 6,         // the dialog is closed
		STATE_US = 7,             // the app's stat 21
		STATE_LIGHTNINGGEMS = 8,  // the app's stat 20
		STATE_MULTIPLIER = 9,     // the board's +0x21d0
		STATE_PRELOAD = 10,       // Preload runs the script up to the first wait
		STATE_SHARDS = 11,
		STATE_LEVELUP = 12,       // the level changes
		STATE_SECONDS = 13,       // mStateArg hundredths of a second
		STATE_MATCHES_DONE = 14,  // the board's +0x25a4
		STATE_TUBE = 15,          // the Tube art loads
		STATE_MENU = 16,          // the loading thread ends, then the main menu
	};

	int mLineCount = 0;               // +0x150: lines read (never read)
	float mArrowPhase = 0;            // +0x154: 0 to 2 pi, 0.04 an update (never read)
	int mPointX = -10000;             // +0x158: where the arrow points, on the screen
	int mPointY = -10000;             // +0x15c
	int mPointRow = -1;               // +0x160: the grid square the hole is cut at
	int mPointCol = -1;               // +0x164
	float mPointAngle = 0;            // +0x168: the arrow's angle (pi for the tube's)
	int mState = 0;                   // +0x16c
	int mStateArg = 0;                // +0x170
	TwistDialog* mDialog = nullptr;   // +0x174
	TwistDialog* mFadingDialog = nullptr;  // +0x178: closed, still shrinking away
	PopAnim* mAnim = nullptr;         // +0x17c: the dialog's animation
	std::string mAnimLabel;           // +0x180
	int mDialogX = -1;                // +0x19c: where the dialog glides to
	int mDialogY = -1;                // +0x1a0
	int mPointedPieceId = -1;         // +0x1a4: the gem the arrow and the hole follow
	std::string mHint;                // +0x1a8: the big text over the board
	int mUnknown1C4 = 0;              // +0x1c4: cleared when a wait ends (never read)
	bool mKeepDialog = false;         // +0x1c9: the last textbox had no buttons, so the dialog stays across waits
	std::string mGotoLabel;           // +0x1cc: set by the script's name, then by goto: lines skipped until its ":label"
	float mTimer = 0;                 // +0x1e8
	Buffer* mLines = nullptr;         // +0x1ec: the board's script lines
	bool mMarkSeen = true;            // +0x1f0: the tip is marked seen when the script ends
	int mTitleState = 0;              // +0x1f4: 0 to 3 (never set)
	Image* mTitleImage = nullptr;     // +0x1f8 (never made)
	float mTitleScale = 1.0f;         // +0x1fc
	float mTitleX = 0;                // +0x200
	float mTitleY = 0;                // +0x204
	float mTitleHold = 0;             // +0x208
	float mTitleAlpha = 0;            // +0x20c
	bool mUnknown210 = true;          // +0x210: read by the board: a textbox with buttons, or a tube or seconds wait, is up
	bool mFinished = false;           // +0x211: the script has ended
	int mKind = 0;                    // +0x214: the tip's kind (set by ShowNextTip)
	Board* mBoard = nullptr;          // +0x218
	bool mMoveDone = false;           // +0x21c
	bool mMovesAllowed = true;        // +0x21d

	Tutorial(Board* theBoard, Piece* thePiece, const char* theName, Buffer* theLines);  // 0x75122f
	~Tutorial() override;             // 0x750bce (slot 0 0x750ccc)

	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x75408f (slot 20)
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x750a5a (slot 21)
	void Update() override;           // 0x754006 (slot 22)
	void Draw(Graphics* g) override;  // 0x750d2c (slot 26)
	void DrawOverlay(Graphics* g) override;  // 0x751407 (slot 44): "LOADING..." while the tube loads

	void DialogButtonDepress(int theDialogId, int theButtonId) override;  // 0x750ad8 (TwistDialogListener slot 1)
	void PopAnimStopped(int theId) override;  // 0x750b7d (PopAnimListener slot 0)

	void Preload();                   // 0x7540ad: runs the script up to its first wait (the "preload" state)
	void MoveDone();                  // 0x750b8c: the forced move is done
	void PointAtNothing();            // 0x750b67
	void ClearDialog();               // 0x750a63

private:
	void CheckWait();                 // 0x753a4e: ends the wait if its condition holds, then runs on
	bool RunScript();                 // 0x751573: false when the script has ended
	bool ReadLine(std::string& theLine);  // 0x7513ad
};

}  // namespace Sexy
