#ifndef __REPLAYBROWSERDIALOG_H__
#define __REPLAYBROWSERDIALOG_H__

// port(variant): the replay browser (id 31; games/bejeweled/docs/CODE_MODS_DESIGN.md 4.11, 8). Not part of the
// original game. Extras opens it over itself with features.replays. A list of the replay folder's replays
// (gGameConfig.mReplayDir; the names ReplayRecorder::IsReplayName takes), newest first, one row each:
//   "date time  mode  score  seed n  rules title", a "*" in front of a kept one
// (the date, time, mode and score from the name; the seed and the title, the mods that made the rules, from the
// file: Replay::Peek). Watch (or a double click) closes Options, Extras and this dialog and watches it
// (WinDMApp::WatchReplay, which asks to leave a game in progress); Keep pins it ("-keep", so pruning never removes
// it) and Unkeep takes the pin off; Delete asks (DIALOG_CONFIRM_DELETE), then removes the file; Close. Mods can
// rename the texts: REPLAYSDLG_REPLAYS, REPLAYSDLG_NONE, REPLAYSDLG_WATCH, REPLAYSDLG_KEEP, REPLAYSDLG_UNKEEP,
// REPLAYSDLG_DELETE, REPLAYSDLG_CLOSE, REPLAYSDLG_NORMAL, REPLAYSDLG_TIMED, REPLAYSDLG_SEED.

#include "Dialog.h"
#include "ListListener.h"

#include <string>
#include <vector>

namespace Sexy
{

class WinDMApp;
class ListWidget;
class ScrollbarWidget;
class DialogButton;
class Graphics;
class WidgetManager;

class ReplayBrowserDialog : public Dialog, public ListListener
{
public:
	enum
	{
		WATCH_BUTTON_ID = 0,
		KEEP_BUTTON_ID = 1,
		DELETE_BUTTON_ID = 2,
		LIST_ID = 0,
		SCROLLBAR_ID = 0,
		WIDTH = 560,
		VISIBLE_ROWS = 10
	};

	ReplayBrowserDialog(WinDMApp* theApp);
	virtual ~ReplayBrowserDialog();

	virtual int		GetPreferredHeight(int theWidth) override;
	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;
	virtual void	Draw(Graphics* g) override;
	virtual void	ButtonDepress(int theId) override;
	virtual void	ListClicked(int theId, int theIdx, int theClickCount) override;

	void			DeleteSelected();   // DIALOG_CONFIRM_DELETE's Yes
	std::string		SelectedPath();     // "" for none

	using Dialog::ButtonDepress;

private:
	struct Row
	{
		std::string	mName;
		bool		mIsPinned = false;
	};

	WinDMApp*		mApp;
	ListWidget*		mList;
	ScrollbarWidget* mScrollbar;
	DialogButton*	mWatchButton;
	DialogButton*	mKeepButton;
	DialogButton*	mDeleteButton;
	std::vector<Row> mRows;             // mList's lines, in order

	void			Fill(const std::string& theSelect); // the folder again; theSelect stays selected
	void			UpdateButtons();
	void			ToggleKeep();
	int				ListHeight();
};

}

#endif
