#ifndef __HAMMERHEADS_CREATEUSERDIALOG_H__
#define __HAMMERHEADS_CREATEUSERDIALOG_H__

// The box that asks for a player's name: to make a new player (dialog 5), or to rename the one chosen in the player list (dialog 6).
// Original: class Sexy::CreateUserDialog : public NcMovingDialog, public EditListener, public CheckboxListener (0x178 bytes; the
// constructor is 0x44e440, the destructor 0x44dff0). The check box of the original (an "expert mode" box that is never made) and
// what answers it are left out.

#include <string>

#include "EditListener.h"
#include "NcMovingDialog.h"

namespace Sexy
{

class EditWidget;

class CreateUserDialog : public NcMovingDialog, public EditListener
{
public:
	EditWidget*		mEditWidget;		// +0x170: the name

public:
	// theRename: the box renames a player (dialog 6, "Rename user"); otherwise it makes one (dialog 5, "New user")
	explicit CreateUserDialog(bool theRename);								// 0x44e440
	virtual ~CreateUserDialog();											// 0x44dff0

	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x44e080
	virtual int		GetPreferredHeight(int theWidth) override;				// 0x44e110
	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;	// 0x44e120
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x44e170
	virtual void	Update() override;										// 0x44e240
	virtual void	Draw(Graphics* g) override;								// 0x44e970
	virtual void	ButtonDepress(int theId) override;						// 0x44ea80

	virtual void	EditWidgetText(int theId, const SexyString& theString) override;	// 0x44e1b0: enter answers the box's first button
	virtual bool	AllowChar(int theId, SexyChar theChar) override;		// 0x44e1d0: letters, digits and spaces only

	std::string		GetName();												// 0x44e300: the text without leading, doubled or trailing spaces
	void			SetName(const std::string& theName);					// 0x44e2c0: the text, all of it chosen
};

// 0x44e750: the box is made, sized and added as dialog 5 (6 when it renames)
CreateUserDialog* MakeCreateUserDialog(bool theRename);

}

#endif
