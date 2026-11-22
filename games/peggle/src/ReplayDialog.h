// ReplayDialog.h: the strip at the side of the board that holds the instant-replay button and, while a replay is shown, the
// Next, Prev, Save and Done buttons (the original's ReplayDialog.cpp). It is a dialog of id 0x16 (made modal, but the
// board takes the mouse itself while it is shown).
//
// The drawing, and the board's replay functions its buttons call, are not translated yet.

#ifndef __REPLAYDIALOG_H__
#define __REPLAYDIALOG_H__

#include "ThunderDialog.h"

namespace Sexy
{

class Board;
class Graphics;
class ThunderButton;

class ReplayDialog : public ThunderDialog
{
public:
	explicit ReplayDialog(Board* theBoard); // 0x4a9960
	~ReplayDialog() override = default;

	// the mode: whether a replay is being shown (theShowing), whether the buttons that move between the replays are on
	// (theMultiple), and whether to start at the first replay (theResetIndex)
	void SetMode(bool theShowing, bool theMultiple, bool theResetIndex); // 0x49ab70
	void SetMouseVisibility(bool theVisible);							 // 0x495b70: shows or hides the five buttons
	bool IsFast() const;												 // 0x495d20: the right button is held while a replay plays
	void StopBlinkingButtons();											 // 0x495d50: the Done button stops blinking
	void BlinkDoneButton();												 // 0x49ad00

	int GetPreferredHeight(int theWidth) override;						   // 0x495c90
	void Resize(int theX, int theY, int theWidth, int theHeight) override; // 0x495bb0
	void Update() override;												   // 0x49cc30
	void MouseDown(int x, int y, int theClickCount) override;			   // 0x49acc0
	void Draw(Graphics* g) override;									   // 0x495d70
	void DrawOverlay(Graphics* g) override;								   // 0x49e5d0
	void ButtonPress(int theId) override;								   // 0x49ad30
	void ButtonDepress(int theId) override;								   // 0x49ad60

	using ThunderDialog::MouseDown;

	Board* mBoard;						// +0x178
	ThunderButton* mReplayButton;		// +0x17c: the instant-replay button
	ThunderButton* mNextButton;			// +0x180: id 1
	ThunderButton* mPrevButton;			// +0x184: id 2
	ThunderButton* mSaveButton;			// +0x188: id 3
	ThunderButton* mDoneButton;			// +0x18c: id 4
	int mReplayIndex;					// +0x190: the replay being shown (-1 for none)
	bool mSlowMoHeld;					// +0x194: the left button is held and the slow motion is on for it
	bool mMultiple;						// +0x195: there is more than one replay: the Next and Prev buttons are on
	bool mShowing;						// +0x196: a replay is being shown
};

} // namespace Sexy

#endif
