#ifndef __BOOKWORM_JOURNALWIDGET_H__
#define __BOOKWORM_JOURNALWIDGET_H__

// port(mod): the word journal's page (mod "word-journal", WordJournal.h), opened by the game-over page's WORD
// JOURNAL button (LevelUpWidget): the level-up book, centred over the darkened screen. Left page: the words,
// games and points so far, the best and the longest word, the average word, and a heatmap of the tiles played,
// each letter's share of them against its share of the letters dealt (gLetterWeights): warm for more, cool for
// less. Right page: the best word of each level, 14 levels at a time. A click on the right page shows the next
// levels (when there are more); any other click closes the journal (mClosed; LevelUpWidget deletes it).

#include "Widget.h"
#include "WordJournal.h"

namespace Sexy
{
class BookWormApp;
class Graphics;

class JournalWidget : public Widget
{
public:
	BookWormApp*	mApp;
	JournalStats	mStats;
	bool			mHasJournal;
	int				mFirstRow;      // the right page's first row (an index into mStats.mLevelBests)
	bool			mClosed;

public:
	JournalWidget(BookWormApp* theApp);

	virtual void	Draw(Graphics* g) override;
	virtual void	MouseDown(int x, int y, int theClickCount) override;

	using Widget::MouseDown;

private:
	void			DrawLeftPage(Graphics* g, int thePanelX, int thePanelY);
	void			DrawRightPage(Graphics* g, int thePanelX, int thePanelY);
};

}

#endif
