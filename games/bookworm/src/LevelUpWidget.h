#ifndef __BOOKWORM_LEVELUPWIDGET_H__
#define __BOOKWORM_LEVELUPWIDGET_H__

// LevelUpWidget.cpp (0x42b980-0x42d050; DeletePanel and CreatePanel sit at the end of LabelWidget.cpp's
// range): the level-complete / game-over summary. Board::ShowSummary creates it over the board. A 490x390
// panel (best word, longest word, level and total time, the rank picture, name and next rank) is painted
// once into an off-screen image, falls from the top with two independently bouncing corners (tilted while
// they differ), then a header bar (level up or game over, with the level number) slides down and a footer
// bar with a Continue button slides up. Continue resumes the game (BOARD_NEXT_LEVEL) or, after game over,
// deletes the saved game and opens the Hall of Fame.
// Original: class Sexy::LevelUpWidget : public Widget, public ButtonListener (+0x58), sizeof 0xe0,
// vftable 0x4fd894 (47 slots: no new virtuals), ButtonListener 0x4fd878. Offsets are absolute.
// Behaviour spec: re/bookworm/analysis/fame.md section 2.

#include "Widget.h"
#include "ButtonListener.h"
#include "GameText.h"

namespace Sexy
{

class BookWormApp;
class Graphics;
class MemoryImage;
class ColorizedButton;
class WidgetManager;
class WordBalloon;
class DialogButton;
class JournalWidget;

// 0x52f368 rank names shown in the summary, indexed by min(rank, NUM_RANK_NAMES - 1)
enum
{
	NUM_RANK_NAMES = 32
};
extern const GameTextId gRankNames[NUM_RANK_NAMES];

class LevelUpWidget : public Widget, public ButtonListener
{
public:
	enum
	{
		LEVELUP_ID_CONTINUE = 6,            // mContinueButton
		LEVELUP_ID_JOURNAL = 7              // port(mod): mJournalButton
	};

	// +0x58 ButtonListener vptr
	BookWormApp*	mApp;                   // +0x5c
	ColorizedButton* mContinueButton;       // +0x60 id 6, bar_footer_continue, Verdana12Bold (AddedToManager); owned
	// +0x64 padding (double alignment)
	double			mPanelY;                // +0x68 -150.0; top of the panel = (mLeftY + mRightY) * 0.5
	double			mPanelAngle;            // +0x70 0.0; atan2(mRightY - mLeftY, panel width) while falling
	// +0x78 (a double the ctor sets to 0.03, never read): not ported
	bool			mFalling;               // +0x80 true: the fall, the corners fall and bounce
	// +0x81..+0x87 padding
	double			mLeftY;                 // +0x88 -150.0 left corner y
	double			mLeftVel;               // +0x90 0.0; += 0.15 per update, bounce at 75 with * -0.4, rests above -0.5
	double			mRightY;                // +0x98 -174.0 right corner y
	double			mRightVel;              // +0xa0 0.0
	// +0xa8 (a double the ctor sets to 189.0, never read): not ported
	bool			mFrozen;              // +0xb0 false and never set; true would stop Update after the fall and Draw would draw nothing
	// +0xb1..+0xb7 padding
	double			mSlide;                 // +0xb8 header/footer slide 0..1 = sin(mSlidePhase), 1.0 once the phase reaches 1.570795
	double			mSlidePhase;            // +0xc0 += 0.03 per update
	int				mHeaderY;               // +0xc8 y of the header bar
	int				mFooterY;               // +0xcc y of the footer bar
	MemoryImage*	mPanelImage;            // +0xd0 off-screen panel, size of dialog_levelup (owned)
	Graphics*		mPanelGraphics;         // +0xd4 Graphics on mPanelImage (owned)
	bool			mPanelDirty;            // +0xd8 repaint the panel at the next Draw (CreatePanel)
	bool			mLexArrived;            // +0xd9 Lex was idle in BOARD_LEVEL_COMPLETE
	// port(mod): the mods' results at game over (the greedy bots', the daily board's), in a balloon above Lex;
	// owned, NULL until shown. mModsText: its text blocks as last laid out
	WordBalloon*	mModsBalloon;
	SexyString		mModsText;
	// port(mod): the word journal's button at game over, in the left column under Lex, and its page while open
	// (mod "word-journal"; JournalWidget.h); owned, NULL until made
	DialogButton*	mJournalButton;
	JournalWidget*	mJournal;
	// +0xda..+0xdf padding; sizeof 0xe0

public:
	LevelUpWidget(BookWormApp* theApp);                                     // 0x42bd50
	virtual ~LevelUpWidget();                                               // 0x42be40 (scalar deleting 0x42beb0) slot 0

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;        // 0x42bb10 slot 1
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;    // 0x42bc20 slot 2
	virtual void	SetVisible(bool isVisible) override;                    // 0x42baf0 slot 4
	virtual void	SetDisabled(bool isDisabled) override;                  // 0x42bad0 slot 10
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;// 0x42baa0 slot 13
	virtual void	Draw(Graphics* g) override;                             // 0x42cdd0 slot 16
	virtual void	Update() override;                                      // 0x42bee0 slot 17
	virtual void	KeyDown(KeyCode theKey) override;                       // port(mod): KeyContinue, Replay.h
	virtual void	KeyChar(SexyChar theChar) override;                     // port(mod): Replay.h

	// ButtonListener (+0x58)
	virtual void	ButtonPress(int theId) override;                        // 0x42bc80 (+0x58) slot 3
	virtual void	ButtonDepress(int theId) override;                      // 0x42bc90 (+0x58) slot 4

	void			DeletePanel();                                          // 0x42b980
	void			CreatePanel();                                          // 0x42b9c0
	void			DrawPanel(Graphics* g);                                 // 0x42c210
	void			UpdateModsBalloon();                                    // port(mod)
	// port(mod): Enter is Continue on a level's summary once the footer is in (mod "type-word"); false: not taken
	bool			KeyContinue(KeyCode theKey);
	void			UpdateJournal();                                        // port(mod)

	using Widget::Resize;
	using ButtonListener::ButtonPress;
};

}

#endif
