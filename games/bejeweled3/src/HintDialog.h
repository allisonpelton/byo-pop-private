// Sexy::HintDialog (vtable 0x86e654, made by 0x682bf0), Sexy::HelpDialog (vtable 0x86e0bc, made by 0x696100) with its pages (Sexy::HelpWindow,
// 0x86e244) and Sexy::QuestHelpDialog (vtable 0x8697d4, made by 0x6ccfd0): the game's tutorial boxes and help. A HintDialog (id 0x12) is the box that
// points at the first special gem of each kind; a HelpDialog (id 0x13) is a mode's How To Play, shown before the mode's first game and by the options'
// HOW TO PLAY button; a QuestHelpDialog (id 0x28) is a quest's own text, shown before its first game. All are Bej3Dialogs. Which was seen is kept in
// the profile's mHintFlags (Board::HINT_* and the help ids); see Board.h and BoardHints.cpp for when they open.
// port: the HintDialog has no REPLAY button (the exe's, for a box with a copy of the move, is not made), and the HelpDialog shows its pages as
// columns of text with no picture: the pictures the exe's HelpWindow draws for each page (PopAnims) are not made.
#pragma once

#include "Bej3Dialog.h"
#include "Checkbox.h"
#include "CheckboxListener.h"

#include <string>
#include <vector>

namespace Sexy {

class Board;

class HintDialog : public Bej3Dialog, public CheckboxListener {
public:
	Checkbox* mCheckbox = nullptr;   // +0x22c: "Disable Hints", ticked to turn every box off

	// 0x682bf0 (bejeweled3): the box of theHintId (Board::HINT_*): header and text from default.xml (see the table in HintDialog.cpp), the OK button
	// (id 1000) and the box to tick
	explicit HintDialog(int theHintId);
	~HintDialog() override;

	void Resize(int theX, int theY, int theWidth, int theHeight) override;
	void Draw(Graphics* g) override;   // 0x6829c0 (bejeweled3)
	int GetPreferredHeight(int theWidth) override;
};

class HelpDialog : public Bej3Dialog, public CheckboxListener {
public:
	int mHelpId;                       // +0x22c: which help (the mode's start asks for it: 0x15 Classic, 0 Zen, 10 Lightning, 0x11 Butterflies, 0x12 Poker, 0x14 Ice Storm, 0x16 Diamond Mine)
	Checkbox* mCheckbox = nullptr;     // "Don't show hints": ticked when boxes are off
	std::string mTitle;                // the help's title text, if it has one
	std::vector<std::string> mPages;   // the texts of its pages

	// 0x696100 (bejeweled3): theQueued is how many requests were in the board's queue (this one among them): the button reads START GAME when it is the
	// only one (the game starts when it is closed), else CLOSE; with no game waiting (the options' HOW TO PLAY, board's hold 0) it reads CLOSE
	HelpDialog(int theHelpId, int theQueued);
	~HelpDialog() override;

	void Resize(int theX, int theY, int theWidth, int theHeight) override;
	void Draw(Graphics* g) override;
	// the exe marks the help seen in the profile when the dialog is destroyed (0x6770c0: the bit of the help id; Classic's 0x15 also sets bit 0 and Zen's 0
	// also sets bit 0x15); the port does it when the button is pressed, which is the only way the dialog closes
	void ButtonDepress(int theId) override;
	void CheckboxChecked(int theId, bool theChecked) override;   // 0x6746e0 (bejeweled3)
};

class QuestHelpDialog : public Bej3Dialog {
public:
	// 0x6ccfd0 (bejeweled3): the board's quest says what to show (TitleText and HelpText); the dialog is added and put over the board
	explicit QuestHelpDialog(Board* theBoard);
};

}  // namespace Sexy
