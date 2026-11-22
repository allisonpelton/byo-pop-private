// Sexy::Bej3Dialog (made by 0x76a2c0), Sexy::NewUserDialog (made by 0x6824b0, vtable 0x86ec14) and Sexy::WelcomeDialog
// (made by 0x683600): the game's dialogs. A Bej3Dialog is the framework's Dialog (its Draw, 0x584320, is the framework's)
// with the game's fonts, colours and sizes, that grows in on a curve and fades
// out when it is closed. The NewUserDialog asks for a player's name; the WelcomeDialog is the same asked of a game that
// has no player yet.
// port: the dialog's buttons are the framework's DialogButtons (the exe makes its own Bej3Buttons with 0x76a190), the
// scaling the exe applies to the picture while the dialog grows (only when a 3D graphics device exists) is left out, and
// the exe's dialog fields at +0x20c and +0x220 (a set of widgets that keep their mouse state) and +0x204 and +0x208 (a
// centre to grow from) are not made.
#pragma once

#include "CurvedVal.h"
#include "Dialog.h"
#include "EditListener.h"
#include "EditWidget.h"

namespace Sexy {

class Bej3Dialog : public Dialog {
public:
	CurvedVal mFadeCurve;                // +0x190: how far the dialog has grown (0 hidden, 1 whole)
	bool mClosing = false;               // +0x200: the fade out has begun
	bool mEscapeCloses = false;          // +0x201: Escape presses the No (else the Yes) button
	bool mMouseDownAllowed = true;       // +0x202

	// 0x76a2c0 (bejeweled3)
	Bej3Dialog(int theId, bool isModal, const std::string& theHeader, const std::string& theLines,
		const std::string& theFooter, int theButtonMode);

	void Update() override;                              // 0x76a8a0 (bejeweled3)
	void KeyDown(KeyCode theKey) override;               // 0x769f00 (bejeweled3)
	using Dialog::MouseDown;
	void MouseDown(int x, int y, int theBtnNum, int theClickCount) override;   // 0x76a020 (bejeweled3)
	void ButtonPress(int theId) override;                // 0x769fd0 (bejeweled3)
	void ButtonDepress(int theId) override;              // 0x769f60 (bejeweled3)
	int GetPreferredHeight(int theWidth) override;       // 0x76a280 (bejeweled3)

	// 0x76a670 (bejeweled3): starts the fade out, from the size now
	void StartClose();
	// 0x76a060 (bejeweled3): every widget on the dialog takes theOn as its mouse visibility
	void SetChildrenMouseVisible(bool theOn);
};

class NewUserDialog : public Bej3Dialog, public EditListener {
public:
	EditWidget* mEdit = nullptr;         // +0x22c: the name's box
	bool mCapitalised = false;           // +0x24c: the first letter has been dealt with (AllowText)

	// 0x6824b0 (bejeweled3): the player's name is asked (id 1) or changed (id 2, theIsRename); theWithButtons gives it
	// OK and Cancel buttons
	NewUserDialog(bool theIsRename, bool theWithButtons);
	~NewUserDialog() override;

	void Draw(Graphics* g) override;                                  // 0x682200 (bejeweled3)
	void Resize(int theX, int theY, int theWidth, int theHeight) override;   // 0x67b890 (bejeweled3)
	int GetPreferredHeight(int theWidth) override;                    // 0x674110 (bejeweled3)

	void EditWidgetText(int theId, const SexyString& theString) override;   // 0x673b80 (bejeweled3): Enter presses OK
	bool AllowText(int theId, const SexyString& theText) override;    // 0x675db0 (bejeweled3)
};

// 0x683600 (bejeweled3)
class WelcomeDialog : public NewUserDialog {
public:
	WelcomeDialog();
};

}  // namespace Sexy
