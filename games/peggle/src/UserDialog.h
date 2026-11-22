// UserDialog.h: the list of players, "WHO ARE YOU?" (the original's UserDialog.cpp). It shows the profile in use first, then the
// others; OK (or a double click) makes the one chosen the profile in use, and its buttons rename or delete the one chosen or
// make a new one. It hangs from a chain like the other dialogs of the main menu.

#ifndef __USERDIALOG_H__
#define __USERDIALOG_H__

#include "ListListener.h"
#include "ThunderDialog.h"

#include <string>

namespace Sexy
{

class ListWidget;
class ScrollbarWidget;
class ThunderButton;
class ThunderFrame;
class ThunderballApp;
class WidgetManager;

class UserDialog : public ThunderDialog, public ListListener
{
public:
	explicit UserDialog(ThunderballApp* theApp); // 0x4ac970
	~UserDialog() override;						 // 0x497dd0

	void Resize(int theX, int theY, int theWidth, int theHeight) override; // 0x497e50
	int GetPreferredHeight(int theWidth) override;						   // 0x4980a0
	void AddedToManager(WidgetManager* theWidgetManager) override;		   // 0x498120
	void ButtonDepress(int theId) override;								   // 0x4acec0
	void ListClicked(int theId, int theIdx, int theClickCount) override;   // 0x498200: selects; a double click is OK

	std::string GetSelName() const;				   // 0x4ace40: the name of the line chosen, empty when none is
	void CheckShowScrollbar();					   // 0x4980d0: lays the dialog out again when the scrollbar is needed or is not
	void FinishDeleteUser();					   // 0x498150: takes the chosen line off the list and chooses the one before it
	void FinishRenameUser(const std::string& theNewName); // 0x4981b0: the chosen line gets the new name

	// the ids of the buttons below the list (the framework's own OK and Cancel are Dialog::ID_YES and ID_NO)
	enum
	{
		BUTTON_RENAME = 0,
		BUTTON_DELETE = 1,
		BUTTON_CREATE = 2
	};

	ThunderballApp* mApp;			// +0x17c
	ListWidget* mList;				// +0x180: the names (the list's justify is centred)
	ScrollbarWidget* mScrollbar;	// +0x184: shown only when the names do not all fit
	ThunderButton* mRenameButton;	// +0x188
	ThunderButton* mDeleteButton;	// +0x18c
	ThunderButton* mCreateButton;	// +0x190
	ThunderFrame* mFrame;			// +0x194: the blue frame around the list and its scrollbar
};

} // namespace Sexy

#endif
