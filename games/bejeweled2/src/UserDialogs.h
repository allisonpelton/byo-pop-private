#ifndef __USERDIALOGS_H__
#define __USERDIALOGS_H__

// UserDialogs.h: the dialogs of the profile list and of the puzzle packs, all PodDialogs: CreateUserDialog (0x5aea99,
// the name of a new or renamed player), UserDialog (0x5db36a, "Who are you?") and PuzzlePackSelector (0x5d0f70).

#include "PodDialog.h"
#include "EditListener.h"
#include "ListListener.h"

namespace Sexy
{

class EditWidget;
class ListWidget;
class ScrollbarWidget;

class CreateUserDialog : public PodDialog, public EditListener
{
public:
	EditWidget*		mEdit;              // +0x188

public:
	CreateUserDialog(bool isRename);                                // 0x5aed7c
	virtual ~CreateUserDialog();                                    // 0x5aea42

	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;   // 0x5aea99
	virtual int		GetPreferredHeight(int theWidth) override;      // 0x5aeb1b
	virtual void	AddedToManager(WidgetManager* theManager) override;     // 0x5aeb45
	virtual void	RemovedFromManager(WidgetManager* theManager) override; // 0x5aeb73
	virtual void	EditWidgetText(int theId, const SexyString& theString) override;    // 0x5aeb96
	virtual bool	AllowChar(int theId, SexyChar theChar) override;                    // 0x5aeba9
	virtual void	Draw(Graphics* g) override;                     // 0x5aec61

	SexyString		GetName();                                      // 0x5aeccc the text without a leading, trailing or double space
	void			SetName(const SexyString& theName);             // 0x5aebfc
};

class UserDialog : public PodDialog, public ListListener
{
public:
	ListWidget*		mList;              // +0x188 "(Create a New User)", then the names
	ScrollbarWidget* mScrollbar;        // +0x18c
	PodDialogButton* mRenameButton;     // +0x190
	PodDialogButton* mDeleteButton;     // +0x194

public:
	UserDialog();                                                   // 0x5db642
	virtual ~UserDialog();                                          // 0x5db2e3

	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;   // 0x5db36a
	virtual int		GetPreferredHeight(int theWidth) override;      // 0x5db47c
	virtual void	AddedToManager(WidgetManager* theManager) override;     // 0x5db4a9
	virtual void	RemovedFromManager(WidgetManager* theManager) override; // 0x5db4f3
	virtual void	ButtonDepress(int theId) override;              // 0x5dba24
	virtual void	ListClicked(int theId, int theIdx, int theClickCount) override;    // 0x5db5ae

	void			FinishDeleteUser();                             // 0x5db53d
	void			FinishRenameUser(const SexyString& theNewName); // 0x5db584
	SexyString		GetSelName();                                   // 0x5db9ca
};

class PuzzlePackSelector : public PodDialog, public ListListener
{
public:
	ListWidget*		mList;              // +0x188 the names of the puzzle packs
	ScrollbarWidget* mScrollbar;        // +0x18c

public:
	PuzzlePackSelector();                                           // 0x5d1115
	virtual ~PuzzlePackSelector();                                  // 0x5d0f09

	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;   // 0x5d0f70
	virtual int		GetPreferredHeight(int theWidth) override;      // 0x5d1029
	virtual void	AddedToManager(WidgetManager* theManager) override;     // 0x5d1053
	virtual void	RemovedFromManager(WidgetManager* theManager) override; // 0x5d1083
	virtual void	ListClicked(int theId, int theIdx, int theClickCount) override;    // 0x5d10b3

	SexyString		GetSelectedPack();                              // 0x5d13ae
};

}

#endif
