#ifndef __RECORDSCOREDIALOG_H__
#define __RECORDSCOREDIALOG_H__

// RecordScoreDialog.cpp: modal "High Score" name-entry dialog (id 8, OK / CANCEL).
// Original: class Sexy::RecordScoreDialog : public Dialog, public EditListener, sizeof 0x15c.
// Vtables: primary 0x50729c, ButtonListener 0x50727c (Dialog defaults), EditListener 0x507268
// (subobject +0x150). Offsets are absolute; Ghidra's RecordScoreDialog_data.offset_N is absolute N+0x154.
// Behaviour spec: docs/GAME_NOTES.md (RecordScoreDialog section).

#include "Dialog.h"
#include "EditListener.h"

namespace Sexy
{

class WinDMApp;
class Graphics;
class WidgetManager;
class EditWidget;

// EditWidget colors (int[5][4] RGBA: bkg, outline, text, hilite, hilite text) @0x536108.
// RegisterDialog uses an identical copy @0x5361a8.
inline int gRecordScoreEditColors[5][4] =
{
	{0, 0, 0, 0}, {0, 0, 0, 0}, {240, 240, 255, 255}, {255, 255, 255, 255}, {0, 0, 0, 255}
};

class RecordScoreDialog : public Dialog, public EditListener
{
public:
	enum
	{
		NAME_EDIT_ID = 0,
		NAME_MAX_PIXELS = 104,
		EXTRA_HEIGHT = 56
	};

	// +0x150 EditListener vptr
	WinDMApp*		mApp;                   // +0x154
	EditWidget*		mNameWidget;            // +0x158 edit id 0, text = app->mLastScoreName; app reads ->mString on 2008

public:
	virtual ~RecordScoreDialog();                                   // 0x4d2728 (scalar deleting dtor 0x4d2838)
	virtual int		GetPreferredHeight(int theWidth);               // 0x4d2778 slot 73 (Dialog + 56)
	virtual void	AddedToManager(WidgetManager* theWidgetManager);  // 0x4d278a slot 20
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager); // 0x4d27b8 slot 21
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight); // 0x4d27db slot 40
	virtual void	EditWidgetText(int theId, const SexyString& theString); // 0x4d2822 EditListener slot 0 (Enter = OK)
	void			DrawEditBox(Graphics* g, const Rect& theRect);  // 0x4d2857 (RegisterDialog has an identical copy 0x4d2e04)
	RecordScoreDialog(WinDMApp* theApp, const SexyString& theLines); // 0x4d29b6
	virtual void	Draw(Graphics* g);                              // 0x4d2b34 slot 26

	using Widget::Resize;
};

}

#endif
