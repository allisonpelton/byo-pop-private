#ifndef __PIXELUS_USERDIALOGS_H__
#define __PIXELUS_USERDIALOGS_H__

// UserDialogs.cpp: the two dialogs for the players (Pixelus.exe 0x401af0 NewUserDialog, vftable 0x50b3d0; 0x41d710
// SelectUserDialog, vftable 0x50ee50). Both are GameDialogs with a widget or two in their content area; the
// answers go to the app, which reads the dialogs' state (PixelusApp::NewUserOK, SelectUserOK, ...).
//
// port: the select dialog's list uses the framework's ListWidget with its standard scroll bar; the original has its
// own scroll bar drawn with the four slider pictures (only seen with more than five players).

#include "GameDialog.h"
#include "EditListener.h"
#include "CheckboxListener.h"
#include "ListListener.h"
#include <string>

namespace Sexy
{

class EditWidget;
class Checkbox;
class ListWidget;
class DialogButton;

// 0x401af0: the name of a new player, with the expert-mode box; the same dialog (id 0x15) renames a player
class NewUserDialog : public GameDialog, public EditListener, public CheckboxListener
{
public:
	EditWidget*		mEdit;			// +0x124
	Checkbox*		mExpertBox;		// +0x128 only when a new player is made
	bool			mRename;

public:
	NewUserDialog(PixelusApp* theApp, bool theRename);	// 0x401af0
	virtual ~NewUserDialog();

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;		// 0x4011a0 slot 1
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x4011f0 slot 2
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x4010c0 slot 13
	virtual void	Draw(Graphics* g) override;										// 0x401f20 slot 16
	virtual int		GetPreferredHeight(int theWidth) override;						// 0x401170 slot 50

	virtual void	EditWidgetText(int theId, const SexyString& theString) override;	// 0x401230
	virtual bool	AllowChar(int theId, SexyChar theChar) override;				// 0x401250
	virtual void	CheckboxChecked(int theId, bool theChecked) override;			// 0x401df0

	std::string		GetName() const;			// 0x401980: the text without leading, trailing and doubled spaces
	void			SetName(const std::string& theName);	// 0x401300
	bool			IsExpert() const;			// 0x401290

	using Widget::Resize;
};

// 0x41d710: "Who are you?": the players in a list, with the line that makes a new one and the buttons to rename or
// delete the selected one
class SelectUserDialog : public GameDialog, public ListListener
{
public:
	enum
	{
		LIST_ID = 0,
		ID_RENAME = 1,
		ID_DELETE = 2,
		VISIBLE_LINES = 6,
		LINE_HEIGHT = 0x14
	};

	ListWidget*		mList;			// +0x120
	DialogButton*	mRenameButton;	// +0x128
	DialogButton*	mDeleteButton;	// +0x12c

public:
	SelectUserDialog(PixelusApp* theApp);	// 0x41d710
	virtual ~SelectUserDialog();

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;		// 0x41d560 slot 1
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x41d5b0 slot 2
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x41d460 slot 13
	virtual void	Draw(Graphics* g) override;										// 0x41d690 slot 16
	virtual int		GetPreferredHeight(int theWidth) override;						// 0x41d540 slot 50
	virtual void	ButtonDepress(int theId) override;								// 0x41de10
	virtual void	ListClicked(int theId, int theIdx, int theClickCount) override;	// 0x41d630

	std::string		GetSelectedName() const;	// 0x41dbe0: "" for the line that makes a new player
	void			FillList(const std::string& theSelect);	// 0x41dc60 and the end of the constructor
	void			RenameSelected(const std::string& theName);	// 0x41d600

	using Widget::Resize;
};

}

#endif
