// Sexy::NewUserDialog (0x174 bytes, constructor 0x482590, vtables 0x7af8ec, 0x7afa40 (its ButtonListener part
// at +0x88) and 0x7afa60 (its EditListener part at +0x168); code 0x482590-0x4833c0): asks the new player's
// name. A GameDialog (dialog id 7, GameApp::DIALOG_NEW_USER) with an edit box (a name made of letters, digits and a few
// marks, 189 pixels wide) under the banner, and OK (the footer button when it is the only one) and, from the
// players' list, CANCEL. OK is refused for a blank name or one of spaces; the answer goes to
// GameApp::NewUserDialogDone (0x4450a0).
#pragma once

#include "EditListener.h"
#include "GameDialog.h"

namespace Sexy {

class EditWidget;
class Widget;

class NewUserDialog : public GameDialog, public EditListener {
public:
	EditWidget* mEdit;       // +0x16c
	Widget* mPrevFocus;      // +0x170: the widget that had the focus (it gets it back)

	// theButtonMode: BUTTONS_FOOTER (OK) or BUTTONS_OK_CANCEL; theName: the name shown to begin with
	NewUserDialog(int theButtonMode, const std::string& theName);  // 0x482590
	~NewUserDialog() override;                                      // 0x482840

	// 0x482f00: the name typed
	std::string GetName() const;

	// Dialog
	int GetPreferredHeight(int theWidth) override { return 310; }       // 0x4828c0
	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x4828d0: the edit box goes in with the focus
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x482940: and goes, the focus back
	void Resize(int theX, int theY, int theWidth, int theHeight) override;  // 0x4829a0
	void Draw(Graphics* g) override;                                    // 0x482aa0
	// ButtonListener: OK needs a name, then the app goes on; CANCEL clears the name and the app goes on
	void ButtonDepress(int theId) override;                             // 0x482c00

	// EditListener
	void EditWidgetText(int theId, const SexyString& theString) override;  // 0x482ee0: Return presses OK
	bool AllowChar(int theId, SexyChar theChar) override;                  // 0x482f30
	bool AllowText(int theId, const SexyString& theText) override;         // 0x4833c0
};

}  // namespace Sexy
