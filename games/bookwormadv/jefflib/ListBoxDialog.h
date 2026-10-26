// JeffLib::ListBoxDialog (0x180 bytes, constructor 0x6c8170, vtables 0x7c5ddc, 0x7c5f14 (its ButtonListener part at
// +0x88) and 0x7c5f34 (its ListListener part at +0x150); code 0x6c8130-0x6c89a0): a Dialog (id 6, modal, with OK
// and CANCEL) holding a list of lines, one of them chosen, with JeffLib's scrollbar beside it. The list is eight
// lines high, centred; a click chooses a line (with a click's sound) and a double click is an OK. The dialog
// listener hears OK or CANCEL like any dialog's; the chosen line is in mSelected (empty for CANCEL). What fills
// the list is up to the kind of dialog (RefreshList).
#pragma once

#include <string>

#include "Dialog.h"
#include "ListListener.h"
#include "ListWidget.h"
#include "SexyScrollbarWidget.h"

namespace JeffLib {

class ListBoxDialog : public Sexy::Dialog, public Sexy::ListListener {
public:
	SexyScrollbarWidget* mScrollbar;  // +0x154
	int mListGap;                     // +0x158: below the header, 15
	int mClickSound;                  // +0x15c: a click on a line or a button
	Sexy::ListWidget* mList;          // +0x160 (id 0)
	std::string mSelected;            // +0x164: the chosen line

	// 0x6c8170: theColors are the list's (background, outline, text, hilite, select, select text); the
	// scrollbar goes by itself when all the lines fit
	ListBoxDialog(Sexy::Font* theFont, const std::string& theHeader, const std::string& theLines,
	              Sexy::Image* theDialogImage, Sexy::Image* theButtonImage, int theClickSound, int theColors[][3]);
	~ListBoxDialog() override;  // 0x6c8480

	// slot 76, 0x6c8130: puts the lines in the list (nothing here). Resize and GetPreferredHeight call it.
	virtual void RefreshList() {}

	// Widget
	// 0x6c8720: the list and the scrollbar come too, with the first line chosen
	void AddedToManager(Sexy::WidgetManager* theWidgetManager) override;
	void RemovedFromManager(Sexy::WidgetManager* theWidgetManager) override;  // 0x6c8780
	void Resize(const Sexy::Rect& theRect) override;                          // 0x6c8140
	void Resize(int theX, int theY, int theWidth, int theHeight) override;    // 0x6c85a0
	void MouseWheel(int theDelta) override;                                   // 0x6c8980: a line up or down

	// Dialog: room for the list's eight lines
	int GetPreferredHeight(int theWidth) override;  // 0x6c8550

	// ButtonListener: a button's press is a click; OK (or a double click) takes the chosen line, CANCEL clears it
	using Sexy::ButtonListener::ButtonPress;
	void ButtonPress(int theId) override;    // 0x6c87c0
	void ButtonDepress(int theId) override;  // 0x6c87e0

	// ListListener: a click chooses the line, a double click is OK
	void ListClicked(int theId, int theIdx, int theClickCount) override;  // 0x6c88c0
};

}  // namespace JeffLib
