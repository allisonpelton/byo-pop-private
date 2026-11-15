#ifndef __HAMMERHEADS_USERDIALOG_H__
#define __HAMMERHEADS_USERDIALOG_H__

// "Who are you?": the list of players, to pick one, rename one or delete one (the first line makes a new one).
// Original: class Sexy::UserDialog : public NcMovingDialog, public ListListener (0x180 bytes; the constructor is 0x4622b0, the
// destructor 0x461ec0).

#include <string>

#include "ListListener.h"
#include "NcMovingDialog.h"

namespace Sexy
{

class ListWidgetEx;
class NcButton;
class ScrollbarWidgetEx;

class UserDialog : public NcMovingDialog, public ListListener
{
public:
	ListWidgetEx*		mList;			// +0x16c
	ScrollbarWidgetEx*	mScrollbar;		// +0x170
	NcButton*			mRenameButton;	// +0x174: id 1
	NcButton*			mDeleteButton;	// +0x178: id 2

public:
	UserDialog();															// 0x4622b0
	virtual ~UserDialog();													// 0x461ec0

	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x461f60
	virtual int		GetPreferredHeight(int theWidth) override;				// 0x462040
	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;	// 0x462050
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x4620a0
	virtual void	Draw(Graphics* g) override;								// 0x462230
	virtual void	ButtonDepress(int theId) override;						// 0x462b80
	virtual void	Closed() override;										// 0x4621d0
	virtual void	ListClicked(int theId, int theIdx, int theClickCount) override;	// 0x462180

	std::string		GetSelectedName();										// 0x462950: nothing when "(Create a New User)" is the line chosen
	void			RenameSelected(const std::string& theNewName);			// 0x4620f0: the chosen line reads the new name
	void			AddUser(const std::string& theName);					// 0x462120: a line for the new player, chosen and in view
	void			RemoveSelected(const std::string& theNameToSelect);		// 0x4629d0: the chosen line goes; the line of this player (the first one for none) is chosen
};

// 0x462c50: the list is made, sized and added as dialog 4
UserDialog* MakeUserDialog();

}

#endif
