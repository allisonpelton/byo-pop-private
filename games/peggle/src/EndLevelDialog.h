// EndLevelDialog.h: the dialog that comes up when a level ends: the statistics of the level, the tip, the high score entry
// and the buttons that go on (the original's EndLevelDialog.cpp). It is a dialog of id 0xc that is not modal.
//
// Only the making of it is translated so far; every other function that has work to do stops at NOT_TRANSLATED.

#ifndef __ENDLEVELDIALOG_H__
#define __ENDLEVELDIALOG_H__

#include "ThunderDialog.h"

#include <string>

namespace Sexy
{

class Board;
class Graphics;
class ThunderButton;
class ThunderballApp;

class EndLevelDialog : public ThunderDialog
{
public:
	explicit EndLevelDialog(Board* theBoard); // 0x4a1a10
	~EndLevelDialog() override = default;

	void Resize(int theX, int theY, int theWidth, int theHeight) override; // 0x492c90
	void SetVisible(bool isVisible) override;							   // 0x492e30
	int GetPreferredHeight(int theWidth) override;						   // 0x49c850
	void Update() override;												   // 0x49cab0
	void Draw(Graphics* g) override;									   // not found yet
	void MouseDown(int x, int y, int theBtnNum, int theClickCount) override; // 0x499760
	void MouseMove(int x, int y) override;								   // 0x4931a0
	void MouseLeave() override;											   // 0x4931f0
	void ButtonDepress(int theId) override;								   // 0x49dc20

	using ThunderDialog::MouseDown;

	ThunderballApp* mApp;			// +0x178
	Board* mBoard;					// +0x17c
	ThunderButton* mButtons[2];		// +0x184, +0x188: ids 0 and 1 (their labels are set when the dialog is filled in)
	bool mUnknown18C;				// +0x18c
	int mUnknown190;				// +0x190: 0
	std::string mUnknown1C4;		// +0x1c4: empty
	std::string mUnknown1E0;		// +0x1e0: empty
	int mUnknown1F8;				// +0x1f8: 0
	bool mUnknown254;				// +0x254: false
	int mTipNumber;					// +0x258: the tip shown (counted as seen by Board::CheckIncTip)
};

} // namespace Sexy

#endif
