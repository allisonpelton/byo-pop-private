#ifndef __RECORDSCOREDIALOG_H__
#define __RECORDSCOREDIALOG_H__

// RecordScoreDialog.cpp: the modeless "High Score" name entry (dialog 3, OK / CANCEL) opened by
// HighScoreWidget::DoCheckForHighScore. WinBMApp::ButtonDepress(2003) reads the name and calls
// HighScoreWidget::AddHighScore.
// Original: class Sexy::RecordScoreDialog : public MoneyDialog, public EditListener, sizeof 0x168
// (0x164 is alignment padding), vftable 0x4fbb7c, ButtonListener 0x4fbb58 (Dialog's), EditListener
// 0x4fbb44 (subobject +0x158). Offsets are absolute.
// Behaviour spec: re/bigmoney/analysis/dialogs.md (section 4).

#include "MoneyDialog.h"
#include "EditListener.h"

namespace Sexy
{

class WinBMApp;
class Graphics;
class WidgetManager;
class EditWidget;

// EditWidget colors (int[5][4] RGBA: bkg, outline, text, hilite, hilite text) @0x528448
inline int gRecordScoreEditColors[5][4] =
{
	{0, 0, 0, 0}, {0, 0, 0, 0}, {240, 240, 255, 255}, {255, 255, 255, 255}, {0, 0, 0, 255}
};

class RecordScoreDialog : public MoneyDialog, public EditListener
{
public:
	enum
	{
		NAME_EDIT_ID = 0,
		NAME_MAX_PIXELS = 140,
		EXTRA_HEIGHT = 56
	};

	// +0x158 EditListener vptr
	WinBMApp*		mApp;                   // +0x15c
	EditWidget*		mNameWidget;            // +0x160 id 0, text = app->mLastScoreName; WinBMApp reads ->mString on 2003

public:
	virtual ~RecordScoreDialog();                                   // 0x417730 (scalar deleting dtor 0x417890 slot 0)
	virtual int		GetPreferredHeight(int theWidth) override;               // 0x4177a0 slot 57 (Dialog's + 56)
	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;  // 0x4177b0 slot 7
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override; // 0x4177e0 slot 8
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override; // 0x417810 slot 25
	virtual void	EditWidgetText(int theId, const std::string& theString) override; // 0x417860 EditListener slot 0 (Enter = OK)
	// EditListener slots 1-3 (AllowKey / AllowChar / AllowText) are the shared 'return true' 0x417880: the defaults
	void			DrawEditBox(Graphics* g, const Rect& theRect);  // 0x4178b0
	RecordScoreDialog(WinBMApp* theApp, const std::string& theLines); // 0x417a30
	virtual void	Draw(Graphics* g) override;                              // 0x417d00 slot 13

	// port: 1.2's EditWidget only takes characters 0x20..0x7e; the port's framework allows Latin-1
	// above that (mbAllowExtendedChars). This keeps the original's range for the name.
	virtual bool	AllowChar(int theId, SexyChar theChar) override;

	using Widget::Resize;
};

}

#endif
