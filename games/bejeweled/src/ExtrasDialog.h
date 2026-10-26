#ifndef __EXTRASDIALOG_H__
#define __EXTRASDIALOG_H__

// port(variant): the Extras dialog (id 30; games/bejeweled/docs/CODE_MODS_DESIGN.md 4.10, 8). Not part of the
// original game. Options has an "Extras" button in its register slot while a feature mod adds something here
// (WinDMApp::HasExtras: seeds, replays, the leaderboard's replays; LiveSplit alone adds nothing). From the top:
//   with features.seeds.entry: "Seed for the next game", a digits field filled with this game's seed when it has
//     one, and Play this seed (WinDMApp::PlaySeed, which asks first like New Game); a seed outside 1..4294967295
//     gets a line saying so (MTRand::SRand(0) would play 4357);
//   "This game: SEED n" when the board owns an RNG with a seed;
//   with features.replays: Replays... (ReplayBrowserDialog, over this dialog);
//   Mods..., which leaves Options for this dialog, while an enabled mod has settings.
// Done closes it; Options stays open under it, as under Help. Mods can rename the texts: EXTRASDLG_EXTRAS,
// EXTRASDLG_SEED_LABEL, EXTRASDLG_PLAY_SEED, EXTRASDLG_BAD_SEED, EXTRASDLG_THIS_GAME, EXTRASDLG_REPLAYS,
// EXTRASDLG_MODS, EXTRASDLG_DONE.

#include "Dialog.h"
#include "EditListener.h"

#include <cstdint>

namespace Sexy
{

class WinDMApp;
class EditWidget;
class DialogButton;
class Graphics;
class WidgetManager;

class ExtrasDialog : public Dialog, public EditListener
{
public:
	enum
	{
		PLAY_SEED_BUTTON_ID = 0,
		REPLAYS_BUTTON_ID = 1,
		MODS_BUTTON_ID = 2,
		SEED_EDIT_ID = 0,
		TOP = 56,               // the first row, under the header (Options' sliders start at 60)
		SEED_ROWS_HEIGHT = 84,  // the label, the field with its button, the line for a bad seed
		SEED_LINE_HEIGHT = 26,
		BUTTON_ROW_HEIGHT = 38,
		BOTTOM = 72             // the Done button and the insets under the rows
	};

	ExtrasDialog(WinDMApp* theApp);
	virtual ~ExtrasDialog();

	virtual int		GetPreferredHeight(int theWidth) override;
	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;
	virtual void	Draw(Graphics* g) override;
	virtual void	ButtonDepress(int theId) override;

	// EditListener: digits only, at most 10; Enter plays the seed
	virtual void	EditWidgetText(int theId, const SexyString& theString) override;
	virtual bool	AllowChar(int theId, SexyChar theChar) override;

	using Dialog::ButtonDepress;

private:
	WinDMApp*		mApp;
	EditWidget*		mSeedWidget;        // NULL without features.seeds.entry
	DialogButton*	mPlaySeedButton;
	DialogButton*	mReplaysButton;     // NULL without features.replays
	DialogButton*	mModsButton;        // NULL unless an enabled mod has settings
	uint32_t		mGameSeed;          // the board's seed for "This game", 0 for none
	bool			mBadSeed;           // the field's text is no seed: the line under it says so

	void			PlaySeed();
	int				RowsHeight();
};

}

#endif
