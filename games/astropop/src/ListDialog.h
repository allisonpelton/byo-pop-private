#ifndef __ASTROPOP_LISTDIALOG_H__
#define __ASTROPOP_LISTDIALOG_H__

// ListDialog: a box with a list of the players' names and a scroll bar (original class of 0x140 bytes, constructor
// 0x486570, destructor body 0x484610, vftable 0x56adc0, with a ListListener at +0x110, vftable 0x56af88). Two boxes are
// made of it: UserSelectDialog, "who are you" (id 0x6d, 0x486b00), and DeleteUserDialog, "delete who" (id 0x6e,
// 0x486f20). The box's OK button (or a double click on a name) takes the name that is selected and tells the application.
// Offsets in the comments are the original's.

#include "Text2004.h"
#include "Dialog.h"
#include "ListListener.h"
#include <string>

namespace Sexy
{

class AstroPopApp;
class ListWidget;
class ScrollbarWidget;
class DialogButton;

class ListDialog : public Text2004<Dialog>, public ListListener
{
public:
	AstroPopApp*		mApp;           // +0x114
	ScrollbarWidget*	mScrollbar;     // +0x118
	int					m11c;           // +0x11c where the list starts under the heading (15, the delete box: seven lines)
	ListWidget*			mList;          // +0x120 the names
	std::string			mSelectedName;  // +0x124 the name chosen (the entries "create" and "delete" for the two buttons; empty: none)

public:
	ListDialog(AstroPopApp* theApp, int theId, const std::string& theHeader, const std::string& theLines);   // 0x486570
	virtual ~ListDialog();                                                  // 0x484b20 (body 0x484610)

	virtual void	AddedToManager(WidgetManager* theManager) override;     // 0x482f50
	virtual void	RemovedFromManager(WidgetManager* theManager) override; // 0x482fa0
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;   // 0x482dd0
	virtual int		GetPreferredHeight(int theWidth) override;              // 0x482d90
	virtual void	Draw(Graphics* g) override;                             // 0x483e50
	virtual void	ButtonPress(int theId) override;                        // 0x482fe0 a button going down clicks
	virtual void	ButtonDepress(int theId) override;                      // 0x4868c0 the OK takes the selected name

	virtual void	ListClicked(int theId, int theIdx, int theClickCount) override;     // 0x486a00
	virtual void	RebuildList();                                          // 0x48a810 (slot 52): the names of the players
	virtual void	SelectByName(const std::string& theName);              // 0x484760 (slot 53): the line with this name is selected

protected:
	void			MakeButtonImages();     // the picture of the box and of its buttons (also done by the new player box)
};

// "Who are you?": the players, and the buttons NEW and DELETE under them
class UserSelectDialog : public ListDialog
{
public:
	DialogButton*	mNewButton;     // +0x140
	DialogButton*	mDeleteButton;  // +0x144

public:
	UserSelectDialog(AstroPopApp* theApp);                                  // 0x486b00
	virtual ~UserSelectDialog();                                            // 0x484b50 (body 0x4846e0)

	virtual void	AddedToManager(WidgetManager* theManager) override;     // 0x4830f0
	virtual void	RemovedFromManager(WidgetManager* theManager) override; // 0x483130
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;   // 0x483000
	virtual int		GetPreferredHeight(int theWidth) override;              // 0x4830c0
	virtual void	ButtonDepress(int theId) override;                      // 0x486ea0 NEW and DELETE answer at once

private:
	void			StyleButton(DialogButton* theButton);
};

// "Delete who?": the same list, taller, with a line of text over it
class DeleteUserDialog : public ListDialog
{
public:
	DeleteUserDialog(AstroPopApp* theApp);                                  // 0x486f20
	virtual ~DeleteUserDialog();                                            // 0x484b80 (body 0x484830)
};

}

#endif
