// JeffLib::UserSelectionDialog (0x1bc bytes, constructor 0x6c5400, vtables 0x7c5a44, 0x7c5b7c (its ButtonListener
// part at +0x88) and 0x7c5b9c (its ListListener part at +0x150); code 0x6c5400-0x6c6210): the players' dialog, a
// ListBoxDialog of the players' names. The player now playing comes first and is chosen to begin with, the others
// follow in order of their names (a number counting as itself: "Anna2" before "Anna10"). Beside OK and CANCEL it
// has NEW and DELETE, which the dialog listener hears as buttons 10 and 11 (the dialog itself does nothing about
// them); NEW is dimmed once there are mMaxProfiles players. The dialog cannot be dragged unless mAllowDrag.
#pragma once

#include <string>
#include <vector>

#include "DialogButton.h"
#include "ListBoxDialog.h"

namespace JeffLib {

class UserSelectionDialog : public ListBoxDialog {
public:
	// the dialog's buttons for the listener (as the dialog's own, 1000 and 1001, they are OK and CANCEL)
	enum {
		BUTTON_NEW = 10,
		BUTTON_DELETE = 11,
	};

	// port: the original took these from the game's globals; the library is free of game code, so its user gives
	// them (IMAGE_DIALOG, IMAGE_DIALOG_BUTTON, SOUND_CLICK1, FONT_MEDIUM and the texts at 0x7c5a20, 0x7c5a30 and
	// 0x7c5a34)
	struct Resources {
		Sexy::Image* mDialogImage;
		Sexy::Image* mButtonImage;
		int mClickSound;
		Sexy::Font* mFont;
		std::string mHeader;
		std::string mNewLabel;
		std::string mDeleteLabel;
	};

	std::vector<std::string> mUsers;   // +0x180: every player's name
	std::string mCurrentUser;          // +0x190: the player now playing (first in the list), or empty
	bool mAllowDrag;                   // +0x1ac
	unsigned int mMaxProfiles;         // +0x1b0: the most players there may be (the NEW button is dimmed at that many)
	Sexy::DialogButton* mNewButton;    // +0x1b4 (id 10)
	Sexy::DialogButton* mDeleteButton; // +0x1b8 (id 11)

	explicit UserSelectionDialog(const Resources& theResources);  // 0x6c5400
	~UserSelectionDialog() override;                               // 0x6c5690

	int GetUserCount() const { return (int)mUsers.size(); }  // 0x477f00
	std::string GetUser(size_t theIdx) const;                // 0x477f30: "" past the last
	void ClearUsers();                                       // 0x477ea0: and no one plays
	// 0x6c6030: another player (unless there are mMaxProfiles already); NEW is dimmed at that many, and the list
	// is moved over a little (12 pixels) when the scrollbar is not showing
	void AddUser(const std::string& theName);
	// 0x6c5e10: the player of that name, if listed, is chosen
	void SelectUser(const std::string& theName);

	// slot 76, 0x6c58f0: the list is the current player, then the others in order
	void RefreshList() override;

	// Widget
	void AddedToManager(Sexy::WidgetManager* theWidgetManager) override;      // 0x6c5d70: NEW and DELETE too
	void RemovedFromManager(Sexy::WidgetManager* theWidgetManager) override;  // 0x6c5db0
	void Resize(const Sexy::Rect& theRect) override;                          // 0x6c53d0
	// 0x6c5c80: NEW and DELETE above OK and CANCEL
	void Resize(int theX, int theY, int theWidth, int theHeight) override;
	void SetColor(int theIdx, const Sexy::Color& theColor) override;          // 0x6c5f30: the buttons' text
	// 0x6c5fc0: a dialog that cannot be dragged keeps the pointer
	using ListBoxDialog::MouseDown;
	void MouseDown(int x, int y, int theClickCount) override;
	void MouseDrag(int x, int y) override;                                    // 0x6c5fa0

	// Dialog
	int GetPreferredHeight(int theWidth) override;     // 0x6c5d40: and room for NEW and DELETE
	void SetButtonFont(Sexy::Font* theFont) override;  // 0x6c5790

	// ButtonListener: NEW and DELETE go to the dialog listener
	using ListBoxDialog::ButtonPress;
	void ButtonDepress(int theId) override;  // 0x6c57d0
};

}  // namespace JeffLib
