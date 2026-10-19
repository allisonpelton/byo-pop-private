#ifndef __BOOKWORM_HELPWIDGET_H__
#define __BOOKWORM_HELPWIDGET_H__

// HelpWidget.cpp: the full-screen help page (at game start unless switched off, and from the Options
// dialog). The backdrop with four pictures of play and their captions, five topic buttons on the left
// whose labels take turns turning brown until one is clicked, the chosen topic's text in a box, the
// hide-at-startup checkbox (GT_HELP_DONT_SHOW) and a continue button. It pauses the board while shown.
// Original: class Sexy::HelpWidget : public Widget, public ButtonListener, public CheckboxListener, sizeof
// 0xa0 (new in BookWormApp::ShowHelp 0x4101c0), vftable 0x4fcd24 (47 slots), ButtonListener vftable
// 0x4fcd04 (+0x58), CheckboxListener vftable 0x4fccfc (+0x5c). Offsets are absolute (the 2004 Widget is
// 0x58 bytes). Behaviour spec: re/bookworm/analysis/screens.md.

#include "Widget.h"
#include "ButtonListener.h"
#include "CheckboxListener.h"

namespace Sexy
{

class BookWormApp;
class ButtonWidget;
class Checkbox;
class DDImage;
class Graphics;
class WidgetManager;

class HelpWidget : public Widget, public ButtonListener, public CheckboxListener
{
public:
	enum
	{
		CONTINUE_BUTTON_ID = 5,
		SHOW_HELP_CHECKBOX_ID = 27,
		TOPIC_BUTTON_ID_FIRST = 28,         // 28..32: the five topics (gHelpTopicNames)
		NUM_TOPICS = 5
	};

	// +0x58 ButtonListener vfptr, +0x5c CheckboxListener vfptr
	BookWormApp*	mApp;                   // +0x60
	bool			mRefreshTopics;         // +0x64 a topic was clicked: Update swaps the topic button images
	int				mUnused68;              // +0x68 zeroed, never read
	int				mUnused6C;              // +0x6c zeroed, never read
	bool			mTopicChosen;           // +0x70 a topic was clicked (ends the label attention cycle)
	int				mCurTopic;              // +0x74 button id 28..32 of the shown topic, -1 = none
	Checkbox*		mShowHelpCheckbox;      // +0x78 id 27 at (215, 395), checked = help off at startup
	ButtonWidget*	mContinueButton;        // +0x7c id 5, button_help_continue, centred at the bottom
	DDImage*		mCheckedImage;          // +0x80 right half of checkboxes_help
	DDImage*		mUncheckedImage;        // +0x84 left half of checkboxes_help
	DDImage*		mFlippedArrow;          // +0x88 bonusarrow_right flipped vertically and palletized; never drawn
	ButtonWidget*	mTopicButtons[NUM_TOPICS]; // +0x8c ids 28..32 at (74, 284 + 18 i)

public:
	HelpWidget(BookWormApp* theApp);                                // 0x429cd0
	virtual ~HelpWidget();                                          // 0x4297e0 (scalar deleting dtor 0x429d60) slot 0
	virtual void	AddedToManager(WidgetManager* theWidgetManager);  // 0x429d90 slot 1
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager); // 0x4298d0 slot 2
	virtual void	OrderInManagerChanged();                        // 0x429940 slot 3
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight); // 0x429990 slot 13
	virtual void	Draw(Graphics* g);                              // 0x42a420 slot 16
	virtual void	Update();                                       // 0x429a50 slot 17

	void			DrawTopicText(Graphics* g);                     // 0x42a170

	virtual void	ButtonPress(int theId);                         // 0x4220c0 ButtonListener slot 3 (shared with FameWidget)
	virtual void	ButtonDepress(int theId);                       // 0x429c70 ButtonListener slot 4
	virtual void	CheckboxChecked(int theId, bool checked);       // 0x42a740 CheckboxListener slot 0

	using Widget::Resize;
	using ButtonListener::ButtonPress;
};

}

#endif
